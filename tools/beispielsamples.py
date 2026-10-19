"""Generate original synthetic audio for first-run testing. No downloaded sample material."""
from pathlib import Path
import numpy as np
import soundfile as sf

def create(root):
    root=Path(root); root.mkdir(parents=True,exist_ok=True); sr=44100; rng=np.random.default_rng(42)
    def save(name,x):
        x=np.asarray(x); x=.8*x/(max(np.max(np.abs(x)),1e-8))
        sf.write(root/name,x,sr,subtype='PCM_24')
    t=np.arange(sr)/sr
    kick=np.sin(2*np.pi*(47*t+85*.025*(1-np.exp(-t/.025))))*np.exp(-t*13)
    snare=(rng.normal(0,.4,sr)+.4*np.sin(2*np.pi*180*t))*np.exp(-t*17)
    hat=np.diff(rng.normal(0,.3,sr+1))*np.exp(-t*40)
    save('Kick_Deep_01.wav',kick); save('Snare_Dry_01.wav',snare); save('HiHat_Closed_01.wav',hat)
    save('Kick_Deep_normalized.wav',kick*.5)
    # This gain variant intentionally differs after save normalization only by quantization;
    # write it directly so the robust detector can find a genuine level difference.
    sf.write(root/'Kick_Deep_normalized.wav',kick*.35,sr,subtype='PCM_16')
    for f,name in [(55,'SubBass_A1'),(110,'Bass_A2'),(220,'Pluck_A3')]:
        x=np.sin(2*np.pi*f*t)*np.exp(-t*(5 if f<200 else 14)); save(name+'.wav',x)
    long=np.arange(sr*4)/sr
    pad=sum(np.sin(2*np.pi*f*long) for f in [261.63,329.63,392])/3
    pad*=np.minimum(long*2,1)*np.minimum((4-long)*2,1)
    save('Pad_C_Dur_Stereo.wav',np.column_stack([pad,pad*.94+.03*np.sin(2*np.pi*393*long)]))
    for bpm in [120,174]:
        length=int(sr*16*60/bpm); loop=np.zeros(length)
        for beat in range(16):
            pos=int(sr*beat*60/bpm); sound=kick if beat%2==0 else snare
            count=min(len(sound),length-pos); loop[pos:pos+count]+=sound[:count]*.8
            for half in [0,.5]:
                pos2=int(sr*(beat+half)*60/bpm); count=min(len(hat),length-pos2)
                loop[pos2:pos2+count]+=hat[:count]*.15
        save(f'Drum_Loop_{bpm}BPM.wav',loop)
    save('Impact_Noise.wav',rng.normal(0,.4,sr)*np.exp(-t*6))
    # Same decoded content in another container.
    x,_=sf.read(root/'Snare_Dry_01.wav'); sf.write(root/'Snare_Container_Copy.aiff',x,sr,subtype='PCM_24')
    return root

if __name__=='__main__':
    root=create(Path(__file__).resolve().parent.parent/'Beispielsamples')
    print(f'Erzeugt: {root}\nDiesen Ordner in der Anwendung als Bibliothek hinzufuegen.')
