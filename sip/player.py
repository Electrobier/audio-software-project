"""Non-blocking output callback. Decoder runs outside the UI thread."""
import threading
import numpy as np
try:
    import sounddevice as sd
except OSError:
    sd=None
from PySide6.QtCore import QObject,Signal,QTimer
from .audio import decode,waveform
from .services import Job

class DecodeJob(Job):
    def __init__(self,path,token,parent=None): super().__init__(parent); self.path=path; self.token=token
    def run(self):
        try:
            data,sr=decode(self.path)
            if len(data)==0: raise ValueError('Die Audiodatei ist leer.')
            self.result.emit((self.token,data,sr,waveform(data)))
        except Exception as exc: self.failed.emit(str(exc))

class Player(QObject):
    changed=Signal(float,float,bool)
    ready=Signal(object,int,object)
    error=Signal(str)
    def __init__(self,parent=None):
        super().__init__(parent)
        self.data=None; self.sr=44100; self.position=0; self.playing=False; self.loop=False
        self.volume=.7; self.stream=None; self.device=None; self.token=0; self.jobs=[]; self.autoplay=False
        self.timer=QTimer(self); self.timer.timeout.connect(self.tick); self.timer.start(40)
    def load(self,path,autoplay=False):
        self.stop(); self.data=None; self.token+=1; self.autoplay=autoplay
        job=DecodeJob(path,self.token,self); self.jobs.append(job)
        job.result.connect(self.loaded); job.failed.connect(self.error)
        job.finished.connect(lambda:self.cleanup_job(job)); job.start()
    def cleanup_job(self,job):
        if job in self.jobs: self.jobs.remove(job)
        job.deleteLater()
    def loaded(self,result):
        token,data,sr,peaks=result
        if token!=self.token: return
        self.data=data; self.sr=sr; self.position=0
        self.ready.emit(data,sr,peaks)
        if self.autoplay: self.play()
    def callback(self,outdata,frames,time,status):
        outdata.fill(0)
        if not self.playing or self.data is None or len(self.data)==0: return
        at=0
        while at<frames and self.playing:
            count=min(frames-at,len(self.data)-self.position)
            if count>0:
                block=self.data[self.position:self.position+count]
                outdata[at:at+count]=block[:,:outdata.shape[1]]*self.volume
                self.position+=count; at+=count
            if self.position>=len(self.data):
                if self.loop: self.position=0
                else: self.playing=False
    def play(self):
        if self.data is None: return
        try:
            if sd is None: raise RuntimeError('PortAudio fehlt. Unter Linux: libportaudio2 installieren.')
            if self.stream is None:
                self.stream=sd.OutputStream(samplerate=self.sr,channels=min(2,self.data.shape[1]),
                    dtype='float32',callback=self.callback,device=self.device)
                self.stream.start()
            if self.position>=len(self.data): self.position=0
            self.playing=True
        except Exception as exc: self.error.emit(f'Audiogerät: {exc}'); self.stop()
    def toggle(self):
        if self.playing: self.playing=False
        else: self.play()
    def stop(self):
        self.playing=False; self.position=0
        if self.stream is not None:
            self.stream.stop(); self.stream.close(); self.stream=None
    def seek(self,seconds):
        if self.data is not None: self.position=max(0,min(len(self.data)-1,int(seconds*self.sr)))
    def tick(self):
        self.changed.emit(self.position/self.sr,0 if self.data is None else len(self.data)/self.sr,self.playing)
    def close(self):
        self.token+=1; self.stop()
        for job in list(self.jobs): job.wait()

class RecordJob(Job):
    def __init__(self,path,seconds,device=None,parent=None):
        super().__init__(parent); self.path=path; self.seconds=seconds; self.device=device
    def run(self):
        try:
            import soundfile as sf
            if sd is None: raise RuntimeError('PortAudio fehlt.')
            # This stream belongs to this worker only; recording never interrupts playback globally.
            blocks=[]; sr=44100; total=int(sr*self.seconds); recorded=0
            with sd.InputStream(samplerate=sr,channels=1,dtype='float32',device=self.device) as stream:
                while recorded<total and not self.cancelled.is_set():
                    block,overflow=stream.read(min(4096,total-recorded))
                    blocks.append(block.copy()); recorded+=len(block)
                    self.progress.emit(recorded,total)
            if self.cancelled.is_set(): return
            sf.write(self.path,np.concatenate(blocks),sr,subtype='PCM_24')
            self.result.emit(self.path)
        except Exception as exc: self.failed.emit(f'Aufnahme nicht möglich: {exc}')
