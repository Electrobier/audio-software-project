import hashlib
import json
import os
from pathlib import Path
import numpy as np
import pytest
import soundfile as sf
from sip.audio import analyze,full_stats,LocalClassifier
from sip.storage import Store
from sip.services import SimilarJob,ScanJob,DuplicateJob,export_samples

@pytest.fixture(scope='module')
def samples(tmp_path_factory):
    root=tmp_path_factory.mktemp('audio')
    sr=22050; t=np.arange(sr*2)/sr
    tone=(.35*np.sin(2*np.pi*220*t)*np.exp(-t)).astype('float32')
    sf.write(root/'a.wav',tone,sr,subtype='PCM_16')
    x,_=sf.read(root/'a.wav',dtype='float32')
    sf.write(root/'same.aiff',x,sr,subtype='PCM_16')
    sf.write(root/'24bit.wav',x,sr,subtype='PCM_24')
    sf.write(root/'normalized.wav',x*.5,sr,subtype='PCM_24')
    rng=np.random.default_rng(41)
    sf.write(root/'noise.wav',rng.uniform(-.2,.2,sr),sr)
    sf.write(root/'silence.wav',np.zeros(sr),sr)
    (root/'copy.wav').write_bytes((root/'a.wav').read_bytes())
    return root

@pytest.fixture(scope='module')
def analyzed(samples): return {p.name:analyze(p) for p in samples.iterdir()}

def test_content_identity(samples):
    a=full_stats(samples/'a.wav'); b=full_stats(samples/'same.aiff'); c=full_stats(samples/'24bit.wav')
    assert a['sha']!=b['sha']
    assert a['pcm']==b['pcm']==c['pcm']
    assert a['sha']==full_stats(samples/'copy.wav')['sha']

def test_analysis_values(analyzed):
    a=analyzed['a.wav']; noise=analyzed['noise.wav']; silent=analyzed['silence.wav']
    assert a['duration']==2
    assert -12<a['peak']<-8
    assert abs(a['centroid']-220)<100
    assert noise['brightness']>a['brightness']
    assert len(a['vectors']['neural'])==384
    assert len(a['audio_events'])==12
    assert silent['bpm'] is None and silent['musical_key'] is None
    assert silent['lufs'] is None
    assert len(a['vectors']['timbre'])==38
    assert a['pcm']==analyzed['same.aiff']['pcm']
    json.dumps(analyzed,allow_nan=False)

def populate(path,samples,analyzed):
    store=Store(path); lib=store.add_library(samples)
    for name,data in analyzed.items(): store.upsert(lib,samples/name,data)
    return store

def test_combined_filters_and_manual_labels(tmp_path,samples,analyzed):
    store=populate(tmp_path/'lib.db',samples,analyzed)
    a=store.rows("SELECT * FROM samples WHERE name='a.wav'")[0]
    store.set_categories([a['id']],['Synth Bass','Bass Loop'])
    store.execute('UPDATE samples SET favorite=1,tags=? WHERE id=?',('warm, deep',a['id']))
    f={'category':'Synth Bass','duration_max':3,'favorite':True,'text':'deep'}
    assert store.query(f,count=True)==1
    assert store.query(f)[0]['id']==a['id']
    assert store.query({'category':'Bass Loop'},count=True)==1
    store.upsert(a['library_id'],a['path'],analyzed['a.wav'])
    assert json.loads(store.get(a['id'])['manual_categories'])==['Synth Bass','Bass Loop']
    assert store.get(a['id'])['favorite']==1
    assert store.query({'text':'% OR 1=1'},count=True)==0
    store.close()

def test_similarity_audio_not_names(tmp_path,samples,analyzed):
    store=populate(tmp_path/'lib.db',samples,analyzed)
    ref=store.rows("SELECT * FROM samples WHERE name='a.wav'")[0]
    results=[]; errors=[]
    job=SimilarJob(store.path,ref,'Klangfarbe',0)
    job.result.connect(results.append); job.failed.connect(errors.append); job.run()
    assert not errors
    ids=[sid for sid,score in results[0]]
    assert ref['id'] not in ids
    top=store.get(ids[0]); assert top['name'] in ['same.aiff','copy.wav','24bit.wav','normalized.wav']
    score=dict(results[0]); noise=store.rows("SELECT id FROM samples WHERE name='noise.wav'")[0]['id']
    assert score[ids[0]]>score[noise]
    store.close()

def test_export_never_overwrites(tmp_path,samples,analyzed):
    store=populate(tmp_path/'lib.db',samples,analyzed); rows=store.query({})
    dest=tmp_path/'export'
    before={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in samples.iterdir()}
    copied,conflicts=export_samples(rows,store.libraries(),dest,prefix='Kit_',number=True,structure=True)
    assert len(copied)==len(rows) and not conflicts
    assert before=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in samples.iterdir()}
    copied,conflicts=export_samples(rows,store.libraries(),dest,prefix='Kit_',number=True,structure=True)
    assert not copied and len(conflicts)==len(rows)
    with pytest.raises(ValueError): export_samples(rows,store.libraries(),dest,prefix='../')
    store.close()

def test_classifier_from_audio_examples(tmp_path,samples,analyzed):
    store=populate(tmp_path/'lib.db',samples,analyzed)
    a=store.rows("SELECT * FROM samples WHERE name='a.wav'")[0]
    store.set_categories([a['id']],['Synth Bass']); model=LocalClassifier(store.labels())
    labels,confidence=model.predict(analyzed['same.aiff'])
    assert 'Synth Bass' in labels and confidence>.99
    assert LocalClassifier([]).predict(analyzed['a.wav'])==([],None)
    store.close()

def test_duplicates_exact_and_pcm(tmp_path,samples,analyzed):
    store=populate(tmp_path/'lib.db',samples,analyzed)
    output=[]; errors=[]; job=DuplicateJob(store.path)
    job.result.connect(output.append); job.failed.connect(errors.append); job.run()
    assert not errors
    types=[g['type'] for g in output[0]]
    assert 'Byte-identisch' in types and 'Dekodierter Inhalt identisch' in types
    assert all(len(g['samples'])>=2 for g in output[0]); store.close()

def test_scan_incremental_and_missing(tmp_path,samples,analyzed):
    folder=tmp_path/'library'; folder.mkdir(); file=folder/'a.wav'; file.write_bytes((samples/'a.wav').read_bytes())
    store=Store(tmp_path/'lib.db'); lib=store.add_library(folder)
    store.upsert(lib,file,analyzed['a.wav']); output=[]
    job=ScanJob(store.path,store.libraries()); job.result.connect(output.append); job.run()
    assert output[0]['skipped']==1 and output[0]['updated']==0
    file.unlink(); job=ScanJob(store.path,store.libraries()); job.run()
    assert store.query({},count=True)==0
    file.write_bytes((samples/'a.wav').read_bytes()); job=ScanJob(store.path,store.libraries()); job.run()
    assert store.query({},count=True)==1
    store.close()

def test_move_preserves_tags_project_and_identity(tmp_path,samples,analyzed):
    folder=tmp_path/'move'; folder.mkdir(); source=folder/'old.wav'; source.write_bytes((samples/'a.wav').read_bytes())
    store=Store(tmp_path/'lib.db'); lib=store.add_library(folder); store.upsert(lib,source,analyzed['a.wav'])
    row=store.query({})[0]; pid=store.execute('INSERT INTO projects(name) VALUES(?)',('Song',))
    store.execute('INSERT INTO project_samples VALUES(?,?)',(pid,row['id']))
    store.execute('UPDATE samples SET tags=?,favorite=1,missing=1 WHERE id=?',('warm',row['id']))
    target=folder/'renamed.wav'; source.rename(target); store.upsert(lib,target,analyzed['a.wav'])
    updated=store.get(row['id'])
    assert updated['path']==str(target) and updated['tags']=='warm' and updated['favorite']==1
    assert store.query({'project_id':pid},count=True)==1
    assert store.query({},count=True)==1
    store.close()

def test_full_precision_pcm_identity(tmp_path):
    # Two FLOAT64 files may differ audibly by far less than float32 precision; never call them exact.
    x=np.full(100,.1,dtype='float64'); y=x.copy(); y[0]+=1e-12
    sf.write(tmp_path/'x.wav',x,44100,subtype='DOUBLE'); sf.write(tmp_path/'y.wav',y,44100,subtype='DOUBLE')
    assert full_stats(tmp_path/'x.wav')['pcm']!=full_stats(tmp_path/'y.wav')['pcm']

def test_similarity_obeys_combined_filters(tmp_path,samples,analyzed):
    store=populate(tmp_path/'lib.db',samples,analyzed)
    ref=store.rows("SELECT * FROM samples WHERE name='a.wav'")[0]
    target=store.rows("SELECT * FROM samples WHERE name='same.aiff'")[0]
    store.execute('UPDATE samples SET favorite=1 WHERE id=?',(target['id'],))
    result=[]; job=SimilarJob(store.path,ref,'KI-Embedding',.8,filters={'favorite':True,'duration_max':3})
    job.result.connect(result.append); job.run()
    assert [sid for sid,score in result[0]]==[target['id']]
    store.close()
