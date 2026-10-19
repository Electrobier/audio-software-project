"""Bundled EfficientAT mn04 AudioSet model, CPU-only ONNX, no model downloads at runtime."""
import json
import threading
from pathlib import Path
import numpy as np
from scipy.special import expit

MODEL_DIR=Path(__file__).resolve().parent.parent/'models'
_lock=threading.Lock()
_session=None
_pre=None
_labels=None

# AudioSet labels map to supported broad instrument/event categories. Fine synth subtypes
# need user examples; the model is never presented as knowing labels it wasn't trained on.
MAPPING={
 'Bass drum':'Kick','Snare drum':'Snare','Clapping':'Clap','Hi-hat':'Hi-Hat',
 'Cymbal':'Cymbal','Crash cymbal':'Cymbal','Ride cymbal':'Cymbal',
 'Tom-tom drum':'Tom','Percussion':'Percussion','Drum':'Percussion','Drum kit':'Percussion',
 'Tabla':'Percussion','Tambourine':'Percussion','Cowbell':'Percussion','Maraca':'Percussion','Gong':'Percussion',
 'Double bass':'Acoustic Bass',
 'Piano':'Piano','Electric piano':'Piano','Keyboard (musical)':'Piano',
 'Guitar':'Guitar','Acoustic guitar':'Guitar','Electric guitar':'Guitar',
 'Violin, fiddle':'Strings','Cello':'Strings','Bowed string instrument':'Strings',
 'String section':'Strings','Orchestra':'Strings',
 'Brass instrument':'Brass','Trumpet':'Brass','Trombone':'Brass','French horn':'Brass',
 'Singing':'Vocals','Speech':'Vocals','Choir':'Vocals','Rapping':'Vocals','Humming':'Vocals',
 'Sound effect':'FX','Explosion':'Impacts','Bang':'Impacts','Whoosh, swoosh, swish':'FX',
 'Silence':'','Inside, small room':'Ambiences','Outside, rural or natural':'Ambiences',
 'Outside, urban or manmade':'Ambiences','Rain':'Ambiences','Wind':'Ambiences',
 'Footsteps':'Foley','Walk, footsteps':'Foley','Door':'Foley','Keys jangling':'Foley',
}

def load():
    global _session,_pre,_labels
    if _session is not None: return
    with _lock:
        if _session is not None: return
        import onnxruntime as ort
        ort.disable_telemetry_events()
        options=ort.SessionOptions(); options.intra_op_num_threads=2; options.inter_op_num_threads=1
        options.log_severity_level=3
        session=ort.InferenceSession(str(MODEL_DIR/'efficientat.onnx'),sess_options=options,providers=['CPUExecutionProvider'])
        _pre=np.load(MODEL_DIR/'preprocess.npz'); _labels=json.loads((MODEL_DIR/'labels.json').read_text())
        _session=session

def preprocess(wave):
    load()
    wave=np.asarray(wave,dtype=np.float32)
    if len(wave)<32000: wave=np.pad(wave,(0,32000-len(wave)))
    wave=wave[1:]-.97*wave[:-1]
    framed=np.lib.stride_tricks.sliding_window_view(np.pad(wave,(512,512),mode='reflect'),1024)[::320]
    window=np.pad(_pre['window'],(112,112))
    spectrum=np.fft.rfft(framed*window,axis=-1)
    power=(spectrum.real**2+spectrum.imag**2).T
    mel=_pre['mel']@power
    return ((np.log(mel+1e-5)+4.5)/5).astype('float32')[None,None,:,:]

def infer(wave,sr):
    import librosa
    load()
    wave=np.asarray(wave,dtype=np.float32)
    if sr!=32000: wave=librosa.resample(wave,orig_sr=sr,target_sr=32000)
    size=32000*10
    starts=sorted(set([0,max(0,(len(wave)-size)//2),max(0,len(wave)-size)]))
    scores=[]; embeddings=[]
    for start in starts:
        mel=preprocess(wave[start:start+size])
        logits,embedding=_session.run(None,{'mel':mel})
        scores.append(expit(logits[0])); embeddings.append(embedding.reshape(-1))
    probability=np.mean(scores,axis=0); embedding=np.mean(embeddings,axis=0)
    embedding=embedding/(np.linalg.norm(embedding)+1e-12)
    events=sorted([(_labels[i],float(value)) for i,value in enumerate(probability)],key=lambda x:x[1],reverse=True)
    mapped={}
    if np.max(np.abs(wave))>1e-5:
        for label,score in events:
            category=MAPPING.get(label)
            if category and score>=.35: mapped[category]=max(mapped.get(category,0),score)
    categories=sorted(mapped,key=mapped.get,reverse=True)[:3]
    return {'categories':categories,'confidence':mapped[categories[0]] if categories else None,
            'embedding':embedding.tolist(),'events':events[:12],'model':'EfficientAT mn04_as / AudioSet'}
