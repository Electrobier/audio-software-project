"""Synthetic 100,001-row database benchmark, NOT a hardware/audio library certification."""
import json,sys,time,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
from sip.storage import Store
from sip.services import SimilarJob
import numpy as np

def main():
    with tempfile.TemporaryDirectory() as folder:
        store=Store(Path(folder)/'benchmark.db'); lib=store.add_library(folder)
        n=100001; start=time.perf_counter()
        store.db.executemany('INSERT INTO samples(id,library_id,path,name,duration,kind,data) VALUES(?,?,?,?,?,?,?)',
             ((i,lib,f'{folder}/{i:06d}.wav',f'{i:06d}.wav',1.,'One-Shot','{}') for i in range(1,n+1)))
        vector={'spectral':[.1,.2,.3,.4,.5,.6,.7,.8,.9]}
        payload=json.dumps(vector)
        store.db.executemany('INSERT INTO features VALUES(?,?)',((i,payload) for i in range(1,n+1)))
        store.db.commit(); insert=time.perf_counter()-start
        start=time.perf_counter(); count=store.query({'duration_max':2},count=True); rows=store.query({'text':'099'},limit=500); query=time.perf_counter()-start
        output=[]; errors=[]; job=SimilarJob(store.path,{'vectors':vector},'Frequenzcharakteristik',.9)
        job.result.connect(output.append); job.failed.connect(errors.append)
        start=time.perf_counter(); job.run(); search=time.perf_counter()-start
        assert count==n and not errors and len(output[0])==1000
        print(json.dumps({'samples':n,'insert_seconds':round(insert,3),'filter_count_and_page_seconds':round(query,3),
                          'full_spectral_search_seconds':round(search,3),'result_limit':len(output[0]),
                          'note':'synthetic rows and 9-dimensional vectors; no 100k real-audio timing'},indent=2))
        store.close()
if __name__=='__main__': main()
