from pathlib import Path
import numpy as np
from scipy.io import wavfile
import json,sys
root=Path(sys.argv[1]);rows={}
for f in root.glob('*.wav'):
 sr,a=wavfile.read(f);power=abs(np.fft.rfft(a.astype(float)))**2
 wanted=np.zeros(len(power),bool)
 for freq in (5000,10000,15000,20000):wanted[max(0,freq-1):freq+2]=True
 power[0]=0
 rows[f.stem]=float(power[~wanted].sum()/max(1e-30,power.sum()))
for wave in ['saw','square']:
 assert rows[wave+'_blep']<rows[wave+'_naive']*.5
print(json.dumps(rows,indent=2))
print('Coherent 5 kHz oscillator: BLEP alias-bin power is at least 3 dB below naive.')
print('Filter aliases include output tanh; no hardware-emulation accuracy claim.')
