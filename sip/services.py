import json
import os
import shutil
import threading
from pathlib import Path
import numpy as np
from PySide6.QtCore import QThread, Signal
from .storage import Store
from .audio import analyze, EXTENSIONS, LocalClassifier, ANALYSIS_VERSION

class Job(QThread):
    message=Signal(str)
    progress=Signal(int,int)
    result=Signal(object)
    failed=Signal(str)
    def __init__(self,parent=None):
        super().__init__(parent); self.cancelled=threading.Event()
    def cancel(self): self.cancelled.set()

class ScanJob(Job):
    def __init__(self,dbpath,libraries,force=False,parent=None):
        super().__init__(parent); self.dbpath=dbpath; self.libraries=libraries; self.force=force
    def run(self):
        store=None
        try:
            store=Store(self.dbpath); classifier=LocalClassifier(store.labels())
            done=0; updated=0; errors=[]; skipped=0
            for lib in self.libraries:
                root=Path(lib['path']); self.message.emit(f'Ordner erfassen: {root}')
                if not root.is_dir():
                    errors.append(f'Bibliothek nicht erreichbar: {root}'); continue
                known={r['path']:r for r in store.rows("SELECT path,mtime,size,error,json_extract(data,'$.version') version FROM samples WHERE library_id=?",(lib['id'],))}
                found=set(); files=[]; enumeration_errors=[]
                for folder,dirs,names in os.walk(root,onerror=lambda e:enumeration_errors.append(str(e)),followlinks=False):
                    dirs[:]=[d for d in dirs if not Path(folder,d).is_symlink()]
                    if self.cancelled.is_set(): break
                    for name in names:
                        p=Path(folder,name)
                        if p.suffix.lower() in EXTENSIONS:
                            p=p.resolve(); files.append(p); found.add(str(p))
                if not self.cancelled.is_set() and not enumeration_errors:
                    # An interrupted traversal must never mark unseen files as missing.
                    for missing in known.keys()-found:
                        store.db.execute('UPDATE samples SET missing=1 WHERE path=?',(missing,))
                    store.db.commit()
                errors.extend(enumeration_errors)
                total=len(files)
                for i,path in enumerate(files):
                    if self.cancelled.is_set(): break
                    try:
                        st=path.stat(); prev=known.get(str(path))
                        if not self.force and prev and prev['mtime']==st.st_mtime_ns and prev['size']==st.st_size and not prev['error'] and prev['version']==ANALYSIS_VERSION:
                            skipped+=1
                            store.db.execute('UPDATE samples SET missing=0 WHERE path=?',(str(path),))
                            if i%100==0: store.db.commit()
                        else:
                            self.message.emit(f'Analysiere: {path.name}')
                            data=analyze(path,self.cancelled.is_set)
                            labels,conf=classifier.predict(data)
                            if labels:
                                data.update(categories=labels,category=labels[0],confidence=conf,classifier='Lokaler Beispiel-Klassifikator')
                            after=path.stat()
                            if after.st_mtime_ns!=st.st_mtime_ns or after.st_size!=st.st_size: raise RuntimeError('Während der Analyse geändert; wird beim nächsten Scan erneut verarbeitet.')
                            store.upsert(lib['id'],path,data); updated+=1
                    except InterruptedError: break
                    except Exception as exc:
                        errors.append(f'{path.name}: {exc}')
                        if path.exists(): store.error(lib['id'],path,exc)
                    done+=1; self.progress.emit(i+1,total)
                store.db.commit()
                if self.cancelled.is_set(): break
            self.result.emit({'updated':updated,'skipped':skipped,'errors':errors,'cancelled':self.cancelled.is_set()})
        except Exception as exc: self.failed.emit(str(exc))
        finally:
            if store: store.close()

MODES={'Allgemeiner Klang':{'neural':1.,'timbre':1.,'spectral':1.,'rhythm':.5,'dynamics':.3},
       'KI-Embedding':{'neural':1.},'Rhythmus':{'rhythm':1.},'Klangfarbe':{'timbre':1.},
       'Dynamik':{'dynamics':1.},'Frequenzcharakteristik':{'spectral':1.}}

def unit(v):
    v=np.asarray(v,dtype=np.float32); return v/(np.linalg.norm(v,axis=-1,keepdims=True)+1e-12)

class SimilarJob(Job):
    def __init__(self,dbpath,reference,mode,threshold=0.,weights=None,parent=None,filters=None):
        super().__init__(parent); self.dbpath=dbpath; self.reference=reference; self.mode=mode
        self.threshold=threshold; self.weights=weights or MODES[mode]; self.filters=filters or {}
    def run(self):
        store=None
        try:
            store=Store(self.dbpath)
            data=self.reference if isinstance(self.reference,dict) else analyze(self.reference,self.cancelled.is_set)
            reference_id=data.get('id')
            if 'data' in data: data=json.loads(data['data'])
            if 'vectors' not in data: raise ValueError('Für diese Datei liegt noch keine Analyse vor.')
            eligible={r['id'] for r in store.query(self.filters,limit=1000000000,ids_only=True)} if self.filters else None
            results=[]; count=store.rows('SELECT count(*) n FROM samples WHERE missing=0 AND error IS NULL')[0]['n']; done=0
            cur=store.db.execute('SELECT s.id,f.vectors FROM samples s JOIN features f ON f.sample_id=s.id WHERE s.missing=0 AND s.error IS NULL')
            while batch:=cur.fetchmany(1024):
                if self.cancelled.is_set(): return
                entries=[(r['id'],{'vectors':json.loads(r['vectors'])}) for r in batch if r['id']!=reference_id and (eligible is None or r['id'] in eligible)]
                entries=[(sid,d) for sid,d in entries if d.get('vectors')]
                if entries:
                    scores=np.zeros(len(entries)); divisor=0.
                    for key,weight in self.weights.items():
                        if weight<=0: continue
                        if key not in data['vectors']: raise ValueError('Referenz benötigt eine neue Analyse (KI-Merkmale fehlen).')
                        matrix=unit([d['vectors'].get(key,[0]*len(data['vectors'][key])) for sid,d in entries]); ref=unit(data['vectors'][key])
                        scores+=weight*(matrix@ref); divisor+=weight
                    if divisor<=0: raise ValueError('Mindestens ein Merkmalsgewicht muss größer als null sein.')
                    scores/=divisor
                    results.extend((sid,float(score)) for (sid,d),score in zip(entries,scores) if score>=self.threshold)
                done+=len(batch); self.progress.emit(done,count)
            results.sort(key=lambda v:v[1],reverse=True)
            # A bounded result list prevents GUI memory growth. All library vectors were searched.
            self.result.emit(results[:1000])
        except InterruptedError: pass
        except Exception as exc: self.failed.emit(str(exc))
        finally:
            if store: store.close()

class AnalyzeJob(Job):
    def __init__(self,path,parent=None): super().__init__(parent); self.path=path
    def run(self):
        try: self.result.emit(analyze(self.path,self.cancelled.is_set))
        except InterruptedError: pass
        except Exception as exc: self.failed.emit(str(exc))

class DuplicateJob(Job):
    def __init__(self,dbpath,parent=None): super().__init__(parent); self.dbpath=dbpath
    def run(self):
        store=None
        try:
            store=Store(self.dbpath); groups=[]
            # Linear-time indexed grouping rather than quadratic all-pairs matching.
            used=set()
            for field,label in [('sha','Byte-identisch'),('pcm','Dekodierter Inhalt identisch'),('fingerprint','Ähnlicher Fingerprint – prüfen')]:
                for group in store.rows(f'SELECT {field} hash,COUNT(*) n FROM samples WHERE missing=0 AND {field} IS NOT NULL GROUP BY {field} HAVING n>1'):
                    if self.cancelled.is_set(): return
                    rows=store.rows(f'SELECT * FROM samples WHERE {field}=? AND missing=0',(group['hash'],))
                    ids={r['id'] for r in rows}
                    if ids.issubset(used): continue
                    used.update(ids); groups.append({'type':label,'samples':rows,'verified':field!='fingerprint'})
            # Near matches use quantized spectral signatures to build bounded candidate buckets.
            buckets={}
            for row in store.db.execute('SELECT s.id,s.duration,f.vectors FROM samples s JOIN features f ON f.sample_id=s.id WHERE s.missing=0 AND s.error IS NULL'):
                if self.cancelled.is_set(): return
                d={'vectors':json.loads(row['vectors'])}
                if not d.get('vectors'): continue
                vector=unit(np.r_[d['vectors']['timbre'],d['vectors']['spectral']])
                bands=np.asarray(d['vectors']['spectral'][:9])
                # Broad duration/spectrum buckets; candidates only, never exact duplicates.
                key=(round(float(row['duration'])*10),int(np.argmax(bands)))
                bucket=buckets.setdefault(key,[])
                for sid,v in bucket[-64:]:
                    score=float(vector@v)
                    pair={sid,row['id']}
                    if score>.997 and not pair.issubset(used):
                        samples=[store.get(sid),store.get(row['id'])]
                        groups.append({'type':f'Ähnlicher Klang {score:.1%} – manuell prüfen','samples':samples,'verified':False})
                        if len(groups)>=2000: break
                bucket.append((row['id'],vector))
                if len(groups)>=2000: break
            self.result.emit(groups)
        except Exception as exc: self.failed.emit(str(exc))
        finally:
            if store: store.close()

def export_samples(rows,libraries,destination,prefix='',suffix='',number=False,structure=False,cancelled=lambda:False):
    """Copy only; never overwrite. Returns paths and skipped conflicts."""
    destination=Path(destination).resolve(); destination.mkdir(parents=True,exist_ok=True)
    if any(ch in prefix+suffix for ch in '/\\:*?"<>|'):
        raise ValueError('Präfix/Suffix darf keine Pfad- oder Windows-Sonderzeichen enthalten.')
    roots={lib['id']:Path(lib['path']).resolve() for lib in libraries}
    copied=[]; conflicts=[]
    for i,row in enumerate(rows,1):
        if cancelled(): break
        source=Path(row['path']); parent=destination
        if structure:
            root=roots.get(row['library_id'])
            if root:
                try: parent=destination/root.name/source.resolve().relative_to(root).parent
                except ValueError: pass
        name=f'{prefix}{i:04d}_' if number else prefix
        name+=source.stem+suffix+source.suffix
        target=parent/name
        if target.resolve()==source.resolve() or target.exists(): conflicts.append(str(target)); continue
        parent.mkdir(parents=True,exist_ok=True)
        # Exclusive creation avoids overwriting even when another process creates the destination.
        created=False
        try:
            with open(source,'rb') as src:
                with open(target,'xb') as dst:
                    created=True
                    shutil.copyfileobj(src,dst)
            shutil.copystat(source,target); copied.append(str(target))
        except FileExistsError: conflicts.append(str(target))
        except Exception:
            if created and target.exists(): target.unlink()
            raise
    return copied,conflicts

class ExportJob(Job):
    def __init__(self,rows,libraries,options,parent=None):
        super().__init__(parent); self.rows=rows; self.libraries=libraries; self.options=options
    def run(self):
        try: self.result.emit(export_samples(self.rows,self.libraries,cancelled=self.cancelled.is_set,**self.options))
        except Exception as exc: self.failed.emit(str(exc))
