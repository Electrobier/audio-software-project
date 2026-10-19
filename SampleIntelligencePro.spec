# Build on Windows x64. onedir keeps startup fast and native runtime libraries inspectable.
from PyInstaller.utils.hooks import collect_data_files,collect_dynamic_libs,collect_submodules
from pathlib import Path
root=Path(SPECPATH)
datas=[(str(root/'models'),'models')]
for package in ['librosa','soundfile','onnxruntime']:
    datas+=collect_data_files(package)
hiddenimports=collect_submodules('librosa')+['sip.window','sip.training','scipy.special._ufuncs','sounddevice','_sounddevice_data']
a=Analysis([str(root/'main.py')],pathex=[str(root)],datas=datas,
    binaries=collect_dynamic_libs('onnxruntime'),hiddenimports=hiddenimports,
    excludes=['torch','torchvision','torchaudio','tensorflow','matplotlib','PySide6.QtWebEngineCore','PySide6.QtWebEngineWidgets','pytest','IPython'],
    module_collection_mode={'librosa':'py','numba':'py'},
    noarchive=False)
pyz=PYZ(a.pure)
exe=EXE(pyz,a.scripts,[],exclude_binaries=True,name='SampleIntelligencePro',debug=False,
    bootloader_ignore_signals=False,strip=False,upx=False,console=False)
coll=COLLECT(exe,a.binaries,a.datas,strip=False,upx=False,name='SampleIntelligencePro')
