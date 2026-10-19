import json
import math
from pathlib import Path
from PySide6.QtCore import Qt,QAbstractTableModel,QModelIndex,QMimeData,QUrl,Signal
from PySide6.QtGui import QColor,QPainter,QPen,QPainterPath,QDrag
from PySide6.QtWidgets import QWidget,QTableView,QAbstractItemView,QHeaderView

COLUMNS=[('favorite','★'),('name','Dateiname'),('category','Kategorie'),('kind','Typ'),
 ('duration','Dauer'),('bpm','BPM'),('musical_key','Tonart'),('sr','Hz'),('bit_depth','Bit / Codec'),
 ('channels','Kanäle'),('peak','Peak dBFS'),('rms','RMS dBFS'),('lufs','LUFS I'),
 ('crest','Crest dB'),('dynamics','Dynamik dB'),('brightness','Helligkeit'),
 ('format','Format'),('size','Größe'),('confidence','Konfidenz'),
 ('true_peak','True Peak ≈'),('silence','Stilleanteil'),('centroid','Centroid Hz'),('bandwidth','Bandbreite Hz'),
 ('rolloff','Rolloff Hz'),('flatness','Flatness'),('zcr','ZCR'),('transient','Transienten'),
 ('harmonic','Harmonisch'),('percussive','Perkussiv'),('attack','Attack s'),('decay','Decay s'),('path','Speicherort')]

def display(value,key=''):
    if value is None: return '—'
    if key=='favorite': return '★' if value else '☆'
    if key=='duration': return f'{value:.3f} s'
    if key=='size': return f'{value/1048576:.2f} MB'
    if key in ('brightness','confidence'): return f'{value:.0%}'
    if isinstance(value,float): return f'{value:.1f}'
    return str(value)

class SampleModel(QAbstractTableModel):
    def __init__(self,parent=None): super().__init__(parent); self.rows=[]; self.scores={}
    def set_rows(self,rows,scores=None):
        self.beginResetModel()
        for row in rows:
            for key,value in json.loads(row['data']).items():
                if key not in row: row[key]=value
        self.rows=rows; self.scores=scores or {}; self.endResetModel()
    def rowCount(self,parent=QModelIndex()): return 0 if parent.isValid() else len(self.rows)
    def columnCount(self,parent=QModelIndex()): return len(COLUMNS)+bool(self.scores)
    def headerData(self,section,orientation,role=Qt.DisplayRole):
        if role==Qt.DisplayRole and orientation==Qt.Horizontal:
            return COLUMNS[section][1] if section<len(COLUMNS) else 'Ähnlichkeit'
        return None
    def data(self,index,role=Qt.DisplayRole):
        if not index.isValid(): return None
        row=self.rows[index.row()]; col=index.column()
        if col>=len(COLUMNS):
            return f'{self.scores.get(row["id"],0):.1%}' if role==Qt.DisplayRole else None
        key=COLUMNS[col][0]
        if role==Qt.DisplayRole:
            if key=='category' and row.get('manual_categories'):
                return ', '.join(json.loads(row['manual_categories'])) or 'Unbekannt'
            return display(row.get(key),key)
        if role==Qt.ToolTipRole: return row.get('error') or row['path']
        if role==Qt.ForegroundRole:
            if row.get('error'): return QColor('#f48779')
            if key=='favorite': return QColor('#dfb965' if row.get('favorite') else '#6d737b')
            if key=='category': return QColor('#65c5b5')
        if role==Qt.TextAlignmentRole:
            return int(Qt.AlignVCenter | (Qt.AlignLeft if key in ('name','category','path','kind') else Qt.AlignRight))
        return None
    def flags(self,index):
        return Qt.ItemIsEnabled|Qt.ItemIsSelectable|Qt.ItemIsDragEnabled if index.isValid() else Qt.NoItemFlags
    def mimeTypes(self): return ['text/uri-list','application/x-sip-ids']
    def mimeData(self,indexes):
        selected=sorted({i.row() for i in indexes}); mime=QMimeData()
        mime.setUrls([QUrl.fromLocalFile(self.rows[r]['path']) for r in selected])
        mime.setData('application/x-sip-ids',json.dumps([self.rows[r]['id'] for r in selected]).encode())
        return mime
    def supportedDragActions(self): return Qt.CopyAction

class SampleTable(QTableView):
    def __init__(self,parent=None):
        super().__init__(parent)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.setAlternatingRowColors(True); self.setShowGrid(False)
        self.setDragEnabled(True); self.setDragDropMode(QAbstractItemView.DragOnly)
        self.verticalHeader().hide(); self.verticalHeader().setDefaultSectionSize(27)
        self.horizontalHeader().setSectionsMovable(True)
        self.horizontalHeader().setContextMenuPolicy(Qt.CustomContextMenu)
        self.setSortingEnabled(False)

class Waveform(QWidget):
    seek=Signal(float)
    def __init__(self,parent=None):
        super().__init__(parent); self.waves=[]; self.duration=0.; self.position=0.; self.zoom=1.; self.center=0.
        self.setMinimumHeight(125); self.setMouseTracking(True)
    def set_audio(self,waves,duration):
        self.waves=waves; self.duration=duration; self.position=0; self.center=0; self.update()
    def set_position(self,pos):
        self.position=pos
        if self.zoom>1 and self.duration>0:
            self.center=pos/self.duration
        self.update()
    def visible(self):
        width=1/self.zoom; start=max(0,min(1-width,self.center-width/2))
        return start,start+width
    def paintEvent(self,event):
        painter=QPainter(self); painter.fillRect(self.rect(),QColor('#151719'))
        w=self.width(); h=self.height(); start,end=self.visible()
        if not self.waves:
            painter.setPen(QColor('#6b747b')); painter.drawText(self.rect(),Qt.AlignCenter,'Sample auswählen · Stereo-Wellenform und Vorhören')
            return
        channels=len(self.waves); area=(h-24)/channels
        painter.setPen(QColor('#292d31'))
        for i in range(11):
            x=w*i/10; painter.drawLine(int(x),0,int(x),h-20)
        for channel,peaks in enumerate(self.waves):
            middle=area*(channel+.5); amp=area*.43
            painter.setPen(QColor('#343b3e')); painter.drawLine(0,int(middle),w,int(middle))
            painter.setPen(QPen(QColor('#91b3ae'),1))
            n=len(peaks)
            for px in range(w):
                a=int((start+(end-start)*px/w)*n); b=max(a+1,int((start+(end-start)*(px+1)/w)*n))
                segment=peaks[a:min(b,n)]
                if segment:
                    lo=min(p[0] for p in segment); hi=max(p[1] for p in segment)
                    painter.drawLine(px,int(middle-hi*amp),px,int(middle-lo*amp))
            painter.setPen(QColor('#5a666b')); painter.drawText(8,int(area*channel+15),'L' if channel==0 else 'R')
        if self.duration:
            x=(self.position/self.duration-start)/(end-start)*w
            painter.setPen(QColor('#41d3b0')); painter.drawLine(int(x),0,int(x),h-20)
        painter.setPen(QColor('#68757b'))
        for i in range(6):
            seconds=(start+(end-start)*i/5)*self.duration
            painter.drawText(max(3,int(w*i/5)-25),h-5,f'{seconds:.2f}s')
    def mousePressEvent(self,event):
        if self.duration:
            start,end=self.visible(); self.seek.emit((start+(end-start)*event.position().x()/max(1,self.width()))*self.duration)
    def wheelEvent(self,event):
        self.zoom=max(1,min(32,self.zoom*(1.25 if event.angleDelta().y()>0 else .8))); self.update()
