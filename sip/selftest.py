"""End-to-end check available in Python and frozen builds; no hardware output or user files."""
import json,tempfile,traceback
from pathlib import Path

def run(app,report_path):
    report={}
    try:
        import numpy as np
        import soundfile as sf
        from .audio import analyze
        from .window import Window
        from .services import SimilarJob
        with tempfile.TemporaryDirectory(prefix='sip-selftest-') as directory:
            root=Path(directory); sr=22050; t=np.arange(sr)/sr
            sf.write(root/'tone.wav',.2*np.sin(2*np.pi*220*t),sr)
            data=analyze(root/'tone.wav')
            assert len(data['vectors']['neural'])==384
            window=Window(root/'test.db'); lib=window.store.add_library(root)
            window.store.upsert(lib,root/'tone.wav',data); window.refresh(); window.show(); app.processEvents()
            assert window.model.rowCount()==1
            result=[]; errors=[]
            job=SimilarJob(window.store.path,{'vectors':data['vectors']},'Allgemeiner Klang',.5)
            job.result.connect(result.append); job.failed.connect(errors.append); job.run()
            assert not errors and len(result[0])==1
            window.table.selectRow(0)
            # A decoded sample can be loaded without opening an audio output device.
            while window.player.jobs:
                app.processEvents()
            window.grab().save(str(Path(report_path).with_suffix('.png')))
            window.close(); app.processEvents()
            report={'ok':True,'checks':['DSP analysis','bundled ONNX inference (384-d embedding)','SQLite upsert',
                    'Qt window and sample table','cosine search','background decoding'],'hardware_audio_tested':False}
    except Exception:
        report={'ok':False,'error':traceback.format_exc()}
    Path(report_path).write_text(json.dumps(report,indent=2),encoding='utf-8')
    return 0 if report['ok'] else 1
