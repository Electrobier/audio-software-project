import json
from .services import Job
from .storage import Store
from .audio import LocalClassifier

class ClassifyJob(Job):
    def __init__(self,dbpath,parent=None): super().__init__(parent); self.dbpath=dbpath
    def run(self):
        store=None
        try:
            store=Store(self.dbpath); model=LocalClassifier(store.labels())
            if not model.examples: raise ValueError('Bitte zuerst einigen Samples manuell Kategorien zuweisen. Diese dienen als Trainingsbeispiele.')
            total=store.rows('SELECT count(*) n FROM samples WHERE missing=0 AND manual_categories IS NULL')[0]['n']; done=0
            cursor=store.db.execute('SELECT id,data FROM samples WHERE missing=0 AND manual_categories IS NULL')
            while batch:=cursor.fetchmany(256):
                for row in batch:
                    if self.cancelled.is_set(): break
                    data=json.loads(row['data'])
                    if 'vectors' not in data: continue
                    labels,confidence=model.predict(data,exclude_id=row['id'])
                    source='Lokaler Beispiel-Klassifikator'
                    if not labels:
                        from .neural import MAPPING
                        mapped={}
                        for label,score in data.get('audio_events',[]):
                            c=MAPPING.get(label)
                            if c and score>=.35: mapped[c]=max(mapped.get(c,0),score)
                        labels=sorted(mapped,key=mapped.get,reverse=True)[:3]
                        confidence=mapped[labels[0]] if labels else None
                        if data.get('kind')=='Loop' and any(c in labels for c in ('Kick','Snare','Hi-Hat','Percussion','Cymbal','Tom','Clap')): labels=['Drum Loop']+labels[:2]
                        source='EfficientAT mn04_as / AudioSet'
                    data.update(categories=labels,category=labels[0] if labels else 'Unbekannt',confidence=confidence,classifier=source)
                    store.db.execute('UPDATE samples SET category=?,confidence=?,data=? WHERE id=?',(data['category'],confidence,json.dumps(data),row['id']))
                    done+=1
                store.db.commit(); self.progress.emit(done,total)
                if self.cancelled.is_set(): break
            self.result.emit(done)
        except Exception as exc: self.failed.emit(str(exc))
        finally:
            if store: store.close()
