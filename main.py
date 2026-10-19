"""Run locally with Python or build with PyInstaller on Windows."""
import os
# Limit BLAS oversubscription while the Qt UI and output callback share CPU resources.
os.environ.setdefault('OMP_NUM_THREADS','2')
os.environ.setdefault('OPENBLAS_NUM_THREADS','2')
import sys
from pathlib import Path
from PySide6.QtCore import QStandardPaths
from PySide6.QtWidgets import QApplication,QMessageBox

def main():
    app=QApplication(sys.argv); app.setApplicationName('Sample Intelligence Pro'); app.setOrganizationName('SampleIntelligence')
    if 'NUMBA_CACHE_DIR' not in os.environ:
        cache=Path(QStandardPaths.writableLocation(QStandardPaths.AppLocalDataLocation))/'numba-cache'
        try:
            cache.mkdir(parents=True,exist_ok=True)
        except OSError:
            import tempfile
            cache=Path(tempfile.gettempdir())/'sample-intelligence-numba'
            cache.mkdir(parents=True,exist_ok=True)
        os.environ['NUMBA_CACHE_DIR']=str(cache)
    from sip.window import Window
    if '--self-test' in sys.argv:
        from sip.selftest import run
        return run(app,sys.argv[sys.argv.index('--self-test')+1])
    db=Path(QStandardPaths.writableLocation(QStandardPaths.AppLocalDataLocation))/'library.sqlite3'
    if '--db' in sys.argv: db=Path(sys.argv[sys.argv.index('--db')+1])
    window=Window(db); window.show()
    return app.exec()

if __name__=='__main__': sys.exit(main())
