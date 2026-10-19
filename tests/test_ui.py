import json
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
import numpy as np
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import Qt,QItemSelectionModel
from PySide6.QtTest import QTest
from sip.window import Window
from sip.dialogs import DuplicateDialog,SimilarDialog,FilterDialog
from sip.player import Player
from sip.storage import Store

app=QApplication.instance() or QApplication([])

def test_window_and_filters(tmp_path):
    win=Window(tmp_path/'ui.db'); win.show(); app.processEvents()
    assert win.table.model().rowCount()==0
    assert win.wave.minimumHeight()>0
    assert win.inspector_dock.isVisible()
    assert len(win.store.categories())>=31
    win.filters={'duration_max':20,'bpm_min':170,'bpm_max':180,'category':'Drum Loop'}
    win.refresh(); assert win.total==0
    win.reset_filters(); assert not win.filters
    win.close(); app.processEvents()

def test_duplicate_dialog_never_preselects(tmp_path):
    rows=[{'id':i,'name':f'{i}.wav','format':'WAV','size':100,'path':f'/library/{i}.wav'} for i in [1,2]]
    dialog=DuplicateDialog([{'type':'Byte-identisch','samples':rows,'verified':True}])
    assert dialog.selected_rows()==[]
    for row,item,group in dialog.items.values(): item.setCheckState(0,Qt.Checked)
    try:
        dialog.selected_rows(); assert False,'all-copy deletion must be rejected'
    except ValueError: pass
    dialog.close()

def test_player_stereo_loop_pause_without_device():
    player=Player(); player.data=np.array([[1,.5],[.5,1],[.25,.25]],dtype='float32')
    player.sr=3; player.volume=.5; player.playing=True; player.loop=True
    out=np.zeros((5,2),dtype='float32'); player.callback(out,5,None,None)
    np.testing.assert_allclose(out,[[.5,.25],[.25,.5],[.125,.125],[.5,.25],[.25,.5]])
    player.playing=False; player.callback(out,5,None,None); assert np.all(out==0)
    player.playing=True; player.loop=False; player.position=0; player.callback(out,5,None,None)
    assert not player.playing and np.all(out[3:]==0)
    player.close()

def test_similarity_weight_validation():
    dialog=SimilarDialog(); assert 'neural' in dialog.weights
    dialog.mode.setCurrentText('KI-Embedding')
    assert dialog.weights['neural'].value()==1
    assert dialog.weights['rhythm'].value()==0
    dialog.close()
