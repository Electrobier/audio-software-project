"""Analyse raw float32 interleaved stereo IRs emitted by white224_tests.
Requires numpy/scipy. T20 by Schroeder regression (-5 .. -25 dB), no original IR claim.
"""
import argparse,csv,json
from pathlib import Path
import numpy as np
from scipy.signal import butter,sosfilt

def t20(x,sr):
 e=np.cumsum(np.square(x)[::-1])[::-1]
 if not len(e) or e[0]<=1e-30:return None
 db=10*np.log10(np.maximum(e/e[0],1e-30));idx=np.where((db<=-5)&(db>=-25))[0]
 if len(idx)<100:return None
 slope,intercept=np.polyfit(idx/sr,db[idx],1)
 pred=intercept+slope*idx/sr
 r2=1-np.sum((db[idx]-pred)**2)/max(np.sum((db[idx]-np.mean(db[idx]))**2),1e-30)
 return {'rt60_s':float(-60/slope),'r_squared':float(r2)} if slope<0 else None

def main():
 parser=argparse.ArgumentParser();parser.add_argument('directory',type=Path);a=parser.parse_args();rows=[]
 for file in sorted(a.directory.glob('*.f32')):
  ir=np.fromfile(file,dtype='<f4').reshape(-1,2).astype(float);tail=ir[4800:]
  corr=float(np.corrcoef(tail.T)[0,1]) if np.std(tail[:,0])>1e-15 and np.std(tail[:,1])>1e-15 else None;row={'file':file.name,'sample_rate':48000,'peak':float(np.max(np.abs(ir))),'energy':float(np.sum(ir*ir)),'tail_waveform_correlation':corr,'bands':{}}
  for hz in (125,250,500,1000,2000,4000,8000):
   sos=butter(4,[hz/np.sqrt(2),hz*np.sqrt(2)],btype='bandpass',fs=48000,output='sos');filtered=sosfilt(sos,ir[:,0]);row['bands'][str(hz)]=t20(filtered,48000)
  rows.append(row)
 (a.directory/'metrics.json').write_text(json.dumps(rows,indent=2))
 with (a.directory/'octave_rt60.csv').open('w',newline='') as f:
  w=csv.writer(f);w.writerow(['IR','125 Hz','250 Hz','500 Hz','1 kHz','2 kHz','4 kHz','8 kHz','tail waveform correlation'])
  for r in rows:w.writerow([r['file']]+[round(v['rt60_s'],3) if v else '' for v in r['bands'].values()]+[round(r['tail_waveform_correlation'],4) if r['tail_waveform_correlation'] is not None else ''])
 print((a.directory/'octave_rt60.csv').read_text())
if __name__=='__main__':main()
