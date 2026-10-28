"""Independent RIFF/SMF and numerical checks, requires numpy and scipy."""
from pathlib import Path
import json,struct,sys
import numpy as np
from scipy.io import wavfile
root=Path(sys.argv[1] if len(sys.argv)>1 else 'Library')
rows=[]
for f in sorted(root.glob('*/*.wav')):
 sr,a=wavfile.read(f)
 if a.dtype==np.int32:a=a.astype(float)/2147483648
 elif a.dtype==np.int16:a=a.astype(float)/32768
 a=np.asarray(a,dtype=float)
 assert np.isfinite(a).all()
 peak=float(np.max(np.abs(a)));dc=float(np.mean(a));jump=float(np.max(np.abs(np.diff(a,axis=0))))
 rms=float(np.sqrt(np.mean(a*a)))
 assert peak<1 and abs(dc)<.005 and rms>.001,(f,peak,dc,rms)
 spectrum=np.abs(np.fft.rfft(a[:min(len(a),sr)]*np.hanning(min(len(a),sr))))**2
 high=float(spectrum[int(len(spectrum)*.9):].sum()/max(1e-30,spectrum.sum()))
 rows.append(dict(file=str(f.relative_to(root)),rate=sr,samples=len(a),peak=peak,dc=dc,rms=rms,max_delta=jump,top_10_percent_spectral_energy=high))
midis=[]
for f in root.glob('MIDI/*.mid'):
 b=f.read_bytes();assert b[:4]==b'MThd' and struct.unpack('>IHHH',b[4:14])==(6,0,1,960)
 assert b[14:18]==b'MTrk';assert len(b)==22+struct.unpack('>I',b[18:22])[0]
 i=22;t=0;events=[]
 def vlq():
  global i
  v=0
  for _ in range(4):
   c=b[i];i+=1;v=(v<<7)|(c&127)
   if not c&128:return v
  raise ValueError('VLQ overflow')
 while i<len(b):
  t+=vlq();status=b[i];i+=1
  if status==255:
   kind=b[i];i+=1;n=vlq();i+=n
   if kind==47:break
  else:
   assert status in (128,144,176);x,y=b[i:i+2];i+=2;assert x<128 and y<128;events.append((t,status,x,y))
 notes={};count=0
 for t,s,x,y in events:
  if s==144 and y>0:notes.setdefault(x,[]).append(t);count+=1
  elif s==128:assert x in notes and notes[x];assert t>notes[x].pop(0)
 assert all(not v for v in notes.values()) and count>0
 assert any(s==176 and x==65 for _,s,x,y in events)
 assert any(s==176 and x==80 for _,s,x,y in events)
 midis.append(dict(file=f.name,notes=count,end_tick=t))
assert len(rows)==42 and len(midis)==32
out=root.parent/'docs'/'audio-analysis.json';out.write_text(json.dumps(dict(wavs=rows,midi=midis),indent=2))
print(f'PASS: {len(rows)} WAVs, {len(midis)} independent SMF parses. Peak {max(r["peak"] for r in rows):.6f}, max abs DC {max(abs(r["dc"]) for r in rows):.8f}')
print('Spectral energy is diagnostic; it is not by itself a proof of low aliasing.')
