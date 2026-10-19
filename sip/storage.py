"""SQLite metadata store. Each worker owns its own connection; WAL keeps reads responsive."""
import json
import os
import sqlite3
from pathlib import Path

CATEGORIES = ['Kick', 'Snare', 'Clap', 'Hi-Hat', 'Cymbal', 'Tom', 'Percussion',
 'Drum Loop', 'Breakbeat', 'Synth Bass', 'Acoustic Bass', 'Sub Bass', 'Bass Loop',
 'Synth Lead', 'Synth Pad', 'Synth Pluck', 'Synth Arpeggio', 'Synth Sequence',
 'Piano', 'Guitar', 'Strings', 'Brass', 'Vocals', 'Vocal Chops', 'FX', 'Risers',
 'Impacts', 'Atmospheres', 'Textures', 'Ambiences', 'Foley']

class Store:
    def __init__(self, path):
        self.path = str(path)
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(self.path, timeout=30)
        self.db.row_factory = sqlite3.Row
        self.db.execute('PRAGMA journal_mode=WAL')
        self.db.execute('PRAGMA foreign_keys=ON')
        self.db.executescript('''
        CREATE TABLE IF NOT EXISTS libraries(id INTEGER PRIMARY KEY, path TEXT UNIQUE NOT NULL);
        CREATE TABLE IF NOT EXISTS samples(
          id INTEGER PRIMARY KEY, library_id INTEGER REFERENCES libraries(id) ON DELETE CASCADE,
          path TEXT UNIQUE NOT NULL, name TEXT, size INTEGER, mtime INTEGER,
          format TEXT, duration REAL, sr INTEGER, channels INTEGER, bit_depth TEXT,
          bpm REAL, musical_key TEXT, kind TEXT, brightness REAL, crest REAL, dynamics REAL,
          lufs REAL, rms REAL, peak REAL, category TEXT, confidence REAL,
          favorite INTEGER DEFAULT 0, tags TEXT DEFAULT '', manual_categories TEXT,
          missing INTEGER DEFAULT 0, error TEXT, sha TEXT, pcm TEXT, fingerprint TEXT,
          data TEXT NOT NULL DEFAULT '{}');
        CREATE TABLE IF NOT EXISTS features(sample_id INTEGER PRIMARY KEY REFERENCES samples(id) ON DELETE CASCADE,vectors TEXT NOT NULL);
        CREATE INDEX IF NOT EXISTS sample_lib ON samples(library_id);
        CREATE INDEX IF NOT EXISTS sample_name ON samples(name COLLATE NOCASE);
        CREATE INDEX IF NOT EXISTS sample_sha ON samples(sha);
        CREATE INDEX IF NOT EXISTS sample_pcm ON samples(pcm);
        CREATE INDEX IF NOT EXISTS sample_category ON samples(category);
        CREATE TABLE IF NOT EXISTS projects(id INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL, kind TEXT DEFAULT 'Projekt');
        CREATE TABLE IF NOT EXISTS project_samples(project_id INTEGER REFERENCES projects(id) ON DELETE CASCADE,
          sample_id INTEGER REFERENCES samples(id) ON DELETE CASCADE, PRIMARY KEY(project_id,sample_id));
        CREATE TABLE IF NOT EXISTS searches(id INTEGER PRIMARY KEY, name TEXT UNIQUE, data TEXT);
        CREATE TABLE IF NOT EXISTS categories(name TEXT PRIMARY KEY);
        CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY, value TEXT);
        ''')
        self.db.executemany('INSERT OR IGNORE INTO categories VALUES(?)', [(c,) for c in CATEGORIES])
        self.db.commit()

    def close(self): self.db.close()
    def rows(self, sql, args=()): return [dict(x) for x in self.db.execute(sql, args)]
    def execute(self, sql, args=()):
        cur = self.db.execute(sql, args); self.db.commit(); return cur.lastrowid
    def libraries(self): return self.rows('SELECT * FROM libraries ORDER BY path')
    def add_library(self, path):
        path = str(Path(path).resolve())
        self.execute('INSERT OR IGNORE INTO libraries(path) VALUES(?)', (path,))
        return self.rows('SELECT id FROM libraries WHERE path=?', (path,))[0]['id']
    def get(self, sid):
        rows = self.rows('SELECT * FROM samples WHERE id=?', (sid,))
        return rows[0] if rows else None
    def upsert(self, lib, path, result):
        st = Path(path).stat()
        canonical=str(Path(path).resolve())
        if result.get('sha') and not self.rows('SELECT id FROM samples WHERE path=?',(canonical,)):
            moved=self.rows('SELECT id FROM samples WHERE missing=1 AND sha=? AND library_id=?',(result['sha'],lib))
            if len(moved)==1:
                self.db.execute('UPDATE samples SET path=? WHERE id=?',(canonical,moved[0]['id']))
        keys = ['format','duration','sr','channels','bit_depth','bpm','musical_key','kind',
                'brightness','crest','dynamics','lufs','rms','peak','category','confidence','sha','pcm','fingerprint']
        values = [result.get(k) for k in keys]
        cols = ['library_id','path','name','size','mtime'] + keys + ['data','missing','error']
        vals = [lib,str(Path(path).resolve()),Path(path).name,st.st_size,st.st_mtime_ns] + values + [json.dumps(result, allow_nan=False),0,None]
        updates = ','.join(f'{k}=excluded.{k}' for k in cols if k != 'path')
        self.execute(f'INSERT INTO samples({",".join(cols)}) VALUES({",".join("?" for _ in cols)}) ON CONFLICT(path) DO UPDATE SET {updates}', vals)
        if result.get('vectors'):
            sid=self.rows('SELECT id FROM samples WHERE path=?',(str(Path(path).resolve()),))[0]['id']
            self.execute('INSERT INTO features VALUES(?,?) ON CONFLICT(sample_id) DO UPDATE SET vectors=excluded.vectors',(sid,json.dumps(result['vectors'])))
    def error(self, lib, path, error):
        st = Path(path).stat()
        self.execute('''INSERT INTO samples(library_id,path,name,size,mtime,error)
        VALUES(?,?,?,?,?,?) ON CONFLICT(path) DO UPDATE SET error=excluded.error,mtime=excluded.mtime,missing=0''',
        (lib,str(Path(path).resolve()),Path(path).name,st.st_size,st.st_mtime_ns,str(error)))
    def categories(self): return [r['name'] for r in self.rows('SELECT name FROM categories ORDER BY name')]
    def query(self, filters, order='name', descending=False, limit=500, offset=0, count=False, ids_only=False):
        where, args = ['s.missing=0'], []
        text = filters.get('text','').strip()
        if text:
            where.append("(s.name LIKE ? ESCAPE '\\' OR s.tags LIKE ? ESCAPE '\\' OR s.path LIKE ? ESCAPE '\\')")
            escaped=text.replace('\\','\\\\').replace('%','\\%').replace('_','\\_')
            args.extend(['%'+escaped+'%']*3)
        for key in ('library_id','kind','format','musical_key'):
            if filters.get(key): where.append(f's.{key}=?'); args.append(filters[key])
        if filters.get('category'):
            where.append('EXISTS (SELECT 1 FROM json_each(COALESCE(s.manual_categories,json_extract(s.data,\'$.categories\'),\'[]\')) WHERE value=?)')
            args.append(filters['category'])
        if filters.get('favorite'): where.append('s.favorite=1')
        if filters.get('folder'):
            where.append("s.path LIKE ? ESCAPE '\\'")
            folder=filters['folder'].rstrip('/\\')+os.sep
            args.append(folder.replace('\\','\\\\').replace('%','\\%').replace('_','\\_')+'%')
        if filters.get('project_id'):
            where.append('EXISTS(SELECT 1 FROM project_samples ps WHERE ps.sample_id=s.id AND ps.project_id=?)'); args.append(filters['project_id'])
        for col in ('bpm','duration','brightness','crest','dynamics','lufs'):
            for suffix, op in (('_min','>='),('_max','<=')):
                if filters.get(col+suffix) is not None:
                    where.append(f's.{col}{op}?'); args.append(filters[col+suffix])
        allowed = {'name','size','format','duration','sr','channels','bit_depth','bpm','musical_key','kind','brightness','crest','dynamics','lufs','rms','peak','category','confidence','favorite','path','mtime','id'}
        extra={'true_peak','silence','centroid','bandwidth','rolloff','flatness','zcr','transient','harmonic','percussive','attack','decay','rhythm_confidence','key_confidence'}
        sort_expression=f"json_extract(s.data,'$.{order}')" if order in extra else f's.{order if order in allowed else "name"}'
        if order=='category': sort_expression="COALESCE(json_extract(s.manual_categories,'$[0]'),s.category)"
        select = 'COUNT(*) AS n' if count else 's.id' if ids_only else 's.*'
        sql = f'SELECT {select} FROM samples s WHERE '+ ' AND '.join(where)
        if not count: sql += f' ORDER BY {sort_expression} {"DESC" if descending else "ASC"},s.id LIMIT ? OFFSET ?'; args.extend([limit,offset])
        rows=self.rows(sql,args)
        return rows[0]['n'] if count else rows
    def set_categories(self, ids, categories):
        for sid in ids: self.db.execute('UPDATE samples SET manual_categories=? WHERE id=?',(json.dumps(categories),sid))
        self.db.commit()
    def labels(self):
        return self.rows("SELECT id,data,manual_categories FROM samples WHERE manual_categories IS NOT NULL AND missing=0")
