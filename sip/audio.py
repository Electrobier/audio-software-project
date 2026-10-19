"""Offline DSP. Content hashes cover the full file; heavy features use a bounded excerpt."""
import hashlib
import json
import math
import subprocess
from pathlib import Path
import numpy as np
import scipy.signal as sig
import soundfile as sf

EXTENSIONS={'.wav','.wave','.aif','.aiff','.flac','.ogg','.mp3','.m4a','.aac','.wma','.opus'}
ANALYSIS_VERSION=2
MAX_SECONDS=90

def decode(path, max_seconds=None):
    try:
        with sf.SoundFile(str(path)) as f:
            sr=f.samplerate
            data=f.read(-1 if max_seconds is None else int(sr*max_seconds), dtype='float32',always_2d=True)
        return data,sr
    except (sf.LibsndfileError, RuntimeError):
        command=['ffmpeg','-v','error','-i',str(path)]
        if max_seconds is not None: command += ['-t',str(max_seconds)]
        command += ['-f','wav','-acodec','pcm_f32le','-']
        import io
        p=subprocess.run(command,capture_output=True,timeout=180,check=True)
        data,sr=sf.read(io.BytesIO(p.stdout),dtype='float32',always_2d=True)
        return data,sr

def db(v): return float(20*np.log10(max(float(v),1e-12)))

def clean(value):
    if isinstance(value,dict): return {k:clean(v) for k,v in value.items()}
    if isinstance(value,(list,tuple,np.ndarray)): return [clean(v) for v in value]
    if isinstance(value,(float,np.floating)): return float(value) if np.isfinite(value) else None
    if isinstance(value,np.integer): return int(value)
    return value

def full_stats(path, cancelled=lambda:False):
    """Stream native PCM for strict content identity and level statistics."""
    sha=hashlib.sha256()
    with open(path,'rb') as f:
        while block:=f.read(1024*1024):
            if cancelled(): raise InterruptedError()
            sha.update(block)
    pcm=hashlib.sha256(); total=0; square=0.; peak=0.; silent=0
    try:
        with sf.SoundFile(path) as f:
            meta={'sr':f.samplerate,'channels':f.channels,'duration':len(f)/f.samplerate,
                  'format':f.format,'bit_depth':f.subtype,'frames':len(f)}
            pcm.update(f'{f.samplerate}:{f.channels}:'.encode())
            for x in f.blocks(blocksize=65536,dtype='float64',always_2d=True):
                if cancelled(): raise InterruptedError()
                pcm.update(np.ascontiguousarray(x,dtype='<f8').tobytes())
                square+=float(np.sum(x.astype('float64')**2)); total+=x.size
                peak=max(peak,float(np.max(np.abs(x))))
                silent+=int(np.sum(np.max(np.abs(x),axis=1)<0.001))
        meta.update(sha=sha.hexdigest(),pcm=pcm.hexdigest(),rms=db(math.sqrt(square/max(total,1))),
                    peak=db(peak),silence=silent/max(meta['frames'],1))
        return meta
    except (sf.LibsndfileError,RuntimeError):
        x,sr=decode(path)
        return {'sr':sr,'channels':x.shape[1],'duration':len(x)/sr,'format':Path(path).suffix[1:].upper(),
                'bit_depth':'unbekannt','sha':sha.hexdigest(),'pcm':hashlib.sha256(f'{sr}:{x.shape[1]}:'.encode()+x.astype('<f4').tobytes()).hexdigest(),
                'rms':db(np.sqrt(np.mean(x.astype('float64')**2))),'peak':db(np.max(np.abs(x))),
                'silence':float(np.mean(np.max(np.abs(x),axis=1)<0.001))}

def analyze(path,cancelled=lambda:False):
    import librosa
    import pyloudnorm as pyln
    result=full_stats(str(path),cancelled)
    if cancelled(): raise InterruptedError()
    x,sr=decode(path,MAX_SECONDS)
    if len(x)<32: raise ValueError('Audiodatei ist leer oder zu kurz (mindestens 32 Frames).')
    if not np.all(np.isfinite(x)): raise ValueError('Audiodatei enthält ungültige Werte (NaN/Inf).')
    mono=np.mean(x,axis=1)
    # Full-file LUFS streaming is separate below; spectral work is deliberately bounded.
    y=librosa.resample(mono,orig_sr=sr,target_sr=22050) if sr!=22050 else mono
    ar=22050; nfft=2048; hop=512
    padded=np.pad(y,(0,max(0,nfft-len(y))))
    S=np.abs(librosa.stft(padded,n_fft=nfft,hop_length=hop))
    power=S**2
    centroid=librosa.feature.spectral_centroid(S=S,sr=ar)[0]
    bandwidth=librosa.feature.spectral_bandwidth(S=S,sr=ar)[0]
    roll=librosa.feature.spectral_rolloff(S=S,sr=ar)[0]
    flat=librosa.feature.spectral_flatness(S=S)[0]
    zcr=librosa.feature.zero_crossing_rate(padded)[0]
    onset=librosa.onset.onset_strength(S=librosa.power_to_db(power+1e-12),sr=ar,hop_length=hop)
    rms=librosa.feature.rms(S=S)[0]
    h,p=librosa.decompose.hpss(S)
    harmonic=float(np.sum(h**2)/(np.sum(power)+1e-12))
    percussive=float(np.sum(p**2)/(np.sum(power)+1e-12))
    mel=librosa.feature.melspectrogram(S=power,sr=ar,n_mels=40)
    mfcc=librosa.feature.mfcc(S=librosa.power_to_db(mel+1e-12),n_mfcc=20)
    frequency=np.sum(power,axis=1)
    freq=librosa.fft_frequencies(sr=ar,n_fft=nfft)
    edges=[0,60,120,250,500,1000,2000,4000,8000,11026]
    bands=np.array([np.sum(frequency[(freq>=a)&(freq<b)]) for a,b in zip(edges,edges[1:])])
    bands=bands/(np.sum(bands)+1e-12)
    beats=[]; bpm=None; rhythm_confidence=0.
    if len(y)/ar>=2 and np.max(np.abs(y))>1e-5:
        tempo,frames=librosa.beat.beat_track(onset_envelope=onset,sr=ar,hop_length=hop,trim=False)
        detected=librosa.frames_to_time(frames,sr=ar,hop_length=hop)
        if len(detected)>=4:
            intervals=np.diff(detected); rhythm_confidence=float(max(0,1-np.std(intervals)/(np.mean(intervals)+1e-9)))
            # Reject unaccented/silent passages and irregular pulses.
            if rhythm_confidence>.72 and np.max(onset)>1:
                bpm=float(np.asarray(tempo).reshape(-1)[0]); beats=detected.tolist()
    key=None; key_conf=0.
    if harmonic>.45 and np.max(np.abs(y))>1e-5:
        chroma=librosa.feature.chroma_stft(S=power,sr=ar).mean(axis=1)
        major=np.array([6.35,2.23,3.48,2.33,4.38,4.09,2.52,5.19,2.39,3.66,2.29,2.88])
        minor=np.array([6.33,2.68,3.52,5.38,2.60,3.53,2.54,4.75,3.98,2.69,3.34,3.17])
        scores=[]
        for mode,profile in [('Dur',major),('Moll',minor)]:
            for i in range(12):
                c=float(np.corrcoef(chroma,np.roll(profile,i))[0,1]) if np.std(chroma)>1e-8 else 0.
                scores.append((c,i,mode))
        scores.sort(reverse=True); key_conf=scores[0][0]
        if key_conf>.72 and scores[0][0]-scores[1][0]>.08:
            key=f'{["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"][scores[0][1]]} {scores[0][2]}'
    envelope=np.abs(y)
    frame_env=np.sqrt(np.mean(np.pad(y,(0,(-len(y))%220)).reshape(-1,220)**2,axis=1))
    peakidx=int(np.argmax(frame_env)); active=np.flatnonzero(frame_env>max(float(np.max(frame_env))*.1,1e-5))
    attack=max(0,(peakidx-int(active[0]))*.01) if len(active) else None
    decay=max(0,(int(active[-1])-peakidx)*.01) if len(active) else None
    kind='Unbekannt'
    if result['duration']<=2.0 and attack is not None and attack<.15: kind='One-Shot'
    if bpm and len(beats)>=4:
        bars=result['duration']*bpm/60/4
        if abs(bars-round(bars))<.12 and round(bars)>=1: kind='Loop'
    # Resampled, mono, peak-normalized fingerprint; coarse quantization tolerates bit-depth changes.
    fp=librosa.resample(mono,orig_sr=sr,target_sr=8000)
    fp=fp/(np.max(np.abs(fp))+1e-12)
    fp_hash=hashlib.sha256(np.round(fp*127).astype('int8').tobytes()).hexdigest()
    temporal=np.interp(np.linspace(0,1,32),np.linspace(0,1,len(frame_env)),frame_env)
    temporal=temporal/(np.linalg.norm(temporal)+1e-12)
    timbre=np.r_[mfcc.mean(axis=1)[1:]/100,mfcc.std(axis=1)[1:]/100]
    spectral=np.r_[np.sqrt(bands),np.mean(centroid)/11025,np.mean(bandwidth)/11025,np.mean(roll)/11025,float(np.mean(flat)),float(np.mean(zcr))]
    dynamics=np.array([result['peak']/120,result['rms']/120,(result['peak']-result['rms'])/60,result['silence'],np.std(rms)/(np.mean(rms)+1e-9)])
    rhythm=np.r_[temporal,(bpm or 0)/240,np.mean(onset)/20,np.std(onset)/20,rhythm_confidence]
    vectors={k:v.tolist() for k,v in [('timbre',timbre),('spectral',spectral),('dynamics',dynamics),('rhythm',rhythm)]}
    # Oversampled sample peak estimate (4x); labeled estimate, not a certified BS.1770 true-peak meter.
    tp=float(np.max(np.abs(sig.resample_poly(x,4,1,axis=0))))
    lufs=None
    if result['duration']<=MAX_SECONDS and len(x)/sr>=.4 and x.shape[1]<=5:
        try:
            value=pyln.Meter(sr).integrated_loudness(x)
            lufs=float(value) if np.isfinite(value) else None
        except ValueError: pass
    result.update(version=ANALYSIS_VERSION,analysis_seconds=min(result['duration'],MAX_SECONDS),
        lufs=lufs,lufs_scope='vollständig' if result['duration']<=MAX_SECONDS else 'unbekannt: Datei über 90 s',
        true_peak=db(tp),true_peak_method='4× Oversampling-Schätzung',
        crest=result['peak']-result['rms'],dynamics=db(np.percentile(rms,95))-db(np.percentile(rms,10)),
        centroid=float(np.mean(centroid)),brightness=float(np.sum(bands[6:])),bandwidth=float(np.mean(bandwidth)),
        rolloff=float(np.mean(roll)),flatness=float(np.mean(flat)),zcr=float(np.mean(zcr)),bands=bands.tolist(),
        transient=float(np.max(onset)),harmonic=harmonic,percussive=percussive,bpm=bpm,beats=beats,
        rhythm_confidence=rhythm_confidence,musical_key=key,key_confidence=key_conf,kind=kind,
        attack=attack,decay=decay,fingerprint=fp_hash,vectors=vectors,
        categories=[],category='Unbekannt',confidence=None,classifier='Keine Trainingsbeispiele',
        waveform=waveform(x,800))
    from .neural import infer
    neural=infer(mono,sr)
    result['vectors']['neural']=neural['embedding']
    labels=neural['categories']
    if kind=='Loop' and any(c in labels for c in ('Kick','Snare','Hi-Hat','Percussion','Cymbal','Tom','Clap')):
        labels=['Drum Loop']+labels[:2]
    result.update(categories=labels,category=labels[0] if labels else 'Unbekannt',confidence=neural['confidence'],
                  classifier=neural['model'],audio_events=neural['events'])
    return clean(result)

def waveform(data, points=800):
    n=len(data); step=max(1,math.ceil(n/points)); out=[]
    for ch in range(min(data.shape[1],2)):
        samples=data[:,ch]
        out.append([[round(float(np.min(samples[i:i+step])),4),round(float(np.max(samples[i:i+step])),4)] for i in range(0,n,step)])
    return out

class LocalClassifier:
    """Supervised, incremental k-nearest-exemplar model. Audio only, no filename guesses."""
    def __init__(self, labelled):
        self.examples=[]
        for row in labelled:
            data=json.loads(row['data']); labels=json.loads(row['manual_categories'])
            if data.get('vectors') and labels:
                self.examples.append((row['id'],self.embedding(data),labels))
    @staticmethod
    def embedding(data):
        v=np.r_[data['vectors'].get('neural',[]),data['vectors']['timbre'],data['vectors']['spectral']]
        return v/(np.linalg.norm(v)+1e-12)
    def predict(self,data,exclude_id=None):
        emb=self.embedding(data)
        matches=sorted([(float(np.dot(emb,v)),labels) for sid,v,labels in self.examples if sid!=exclude_id and len(v)==len(emb)],key=lambda x:x[0],reverse=True)[:5]
        if not matches or matches[0][0]<.92: return [],None
        votes={}
        for score,labels in matches:
            if score>=.92:
                for label in labels: votes[label]=votes.get(label,0)+(score-.90)**2
        ranked=sorted(votes,key=votes.get,reverse=True)
        confidence=float(matches[0][0])
        return ranked[:3],confidence
