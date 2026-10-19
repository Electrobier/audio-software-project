import json
from pathlib import Path
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QDialog,QVBoxLayout,QHBoxLayout,QFormLayout,QLabel,QLineEdit,
 QComboBox,QDoubleSpinBox,QSpinBox,QCheckBox,QPushButton,QDialogButtonBox,QFileDialog,
 QListWidget,QListWidgetItem,QAbstractItemView,QTreeWidget,QTreeWidgetItem,QMessageBox,QTextEdit)
from .services import MODES
from .widgets import display

def buttons(dialog,layout):
    box=QDialogButtonBox(QDialogButtonBox.Ok|QDialogButtonBox.Cancel)
    box.accepted.connect(dialog.accept); box.rejected.connect(dialog.reject); layout.addWidget(box)

class FilterDialog(QDialog):
    def __init__(self,filters,parent=None):
        super().__init__(parent); self.setWindowTitle('Erweiterte Filter'); self.resize(470,440)
        self.filters=dict(filters); layout=QVBoxLayout(self); form=QFormLayout(); self.ranges={}
        for key,label,low,high in [('bpm','BPM',0,999),('duration','Dauer (s)',0,86400),
             ('brightness','Helligkeit (0–1)',0,1),('crest','Crest (dB)',0,240),
             ('dynamics','Dynamik (dB)',0,240),('lufs','Lautheit (LUFS)',-120,12)]:
            line=QHBoxLayout(); items=[]
            for suffix in ('_min','_max'):
                check=QCheckBox('von' if suffix=='_min' else 'bis'); spin=QDoubleSpinBox()
                spin.setRange(low,high); spin.setDecimals(2); spin.setValue(filters.get(key+suffix,low if suffix=='_min' else high))
                check.setChecked(filters.get(key+suffix) is not None); spin.setEnabled(check.isChecked())
                check.toggled.connect(spin.setEnabled); line.addWidget(check); line.addWidget(spin); items.append((check,spin))
            self.ranges[key]=items; form.addRow(label,line)
        self.key=QLineEdit(filters.get('musical_key','')); self.key.setPlaceholderText('z. B. C Moll · leer = alle')
        form.addRow('Tonart',self.key); layout.addLayout(form)
        layout.addWidget(QLabel('Alle aktiven Filter werden miteinander kombiniert.'))
        buttons(self,layout)
    def result_filters(self):
        f=dict(self.filters)
        for key,items in self.ranges.items():
            for suffix,(check,spin) in zip(('_min','_max'),items):
                f.pop(key+suffix,None)
                if check.isChecked(): f[key+suffix]=spin.value()
        f.pop('musical_key',None)
        if self.key.text().strip(): f['musical_key']=self.key.text().strip()
        return f

class SimilarDialog(QDialog):
    def __init__(self,parent=None):
        super().__init__(parent); self.setWindowTitle('Find Similar · Audioähnlichkeit'); layout=QVBoxLayout(self); form=QFormLayout()
        self.mode=QComboBox(); self.mode.addItems(MODES); form.addRow('Suchmodus',self.mode)
        self.threshold=QDoubleSpinBox(); self.threshold.setRange(0,100); self.threshold.setValue(65); self.threshold.setSuffix(' %'); form.addRow('Mindestähnlichkeit',self.threshold)
        self.weights={}
        for key,label in [('neural','KI-Embedding'),('timbre','Klangfarbe'),('spectral','Spektrum'),('rhythm','Rhythmus'),('dynamics','Dynamik')]:
            spin=QDoubleSpinBox(); spin.setRange(0,5); spin.setSingleStep(.1); self.weights[key]=spin; form.addRow(label,spin)
        self.mode.currentTextChanged.connect(self.apply_mode); self.apply_mode(self.mode.currentText()); layout.addLayout(form)
        layout.addWidget(QLabel('Die gesamte Bibliothek wird durchsucht. Anzeige: bis zu 1.000 Treffer.\nProzentwerte sind Merkmalsähnlichkeiten, keine Wahrscheinlichkeiten.'))
        buttons(self,layout)
    def apply_mode(self,text):
        for key,spin in self.weights.items(): spin.setValue(MODES[text].get(key,0))
    def accept(self):
        if sum(s.value() for s in self.weights.values())<=0:
            QMessageBox.information(self,'Gewichtung','Mindestens ein Gewicht muss größer als null sein.'); return
        super().accept()

class CategoryDialog(QDialog):
    def __init__(self,categories,selected,parent=None):
        super().__init__(parent); self.setWindowTitle('Kategorien zuordnen · lokal lernen'); self.resize(390,520)
        layout=QVBoxLayout(self); layout.addWidget(QLabel('Diese Zuordnung dient als lokales Trainingsbeispiel.'))
        self.list=QListWidget(); self.list.setSelectionMode(QAbstractItemView.MultiSelection)
        for name in categories:
            item=QListWidgetItem(name); self.list.addItem(item); item.setSelected(name in selected)
        layout.addWidget(self.list); buttons(self,layout)
    def labels(self): return [i.text() for i in self.list.selectedItems()]

class ExportDialog(QDialog):
    def __init__(self,parent=None):
        super().__init__(parent); self.setWindowTitle('Samples exportieren / Cubase-Projekt'); self.resize(500,290)
        layout=QVBoxLayout(self); form=QFormLayout(); line=QHBoxLayout(); self.destination=QLineEdit(); choose=QPushButton('Ordner …')
        choose.clicked.connect(self.choose); line.addWidget(self.destination); line.addWidget(choose); form.addRow('Zielordner',line)
        self.prefix=QLineEdit(); self.suffix=QLineEdit(); self.number=QCheckBox('Fortlaufend nummerieren'); self.structure=QCheckBox('Ordnerstruktur beibehalten')
        form.addRow('Präfix',self.prefix); form.addRow('Suffix',self.suffix); form.addRow(self.number); form.addRow(self.structure)
        layout.addLayout(form); layout.addWidget(QLabel('Originaldateien werden unverändert kopiert.\nVorhandene Zieldateien werden übersprungen.')); buttons(self,layout)
    def choose(self):
        path=QFileDialog.getExistingDirectory(self,'Exportordner')
        if path: self.destination.setText(path)
    def accept(self):
        if not self.destination.text().strip():
            QMessageBox.information(self,'Zielordner','Bitte einen Zielordner wählen.'); return
        super().accept()
    def options(self):
        return dict(destination=self.destination.text(),prefix=self.prefix.text(),suffix=self.suffix.text(),number=self.number.isChecked(),structure=self.structure.isChecked())

class DuplicateDialog(QDialog):
    def __init__(self,groups,parent=None):
        super().__init__(parent); self.setWindowTitle('Duplikatprüfung'); self.resize(1080,590)
        self.groups=groups; self.items={}; self.seen=set(); layout=QVBoxLayout(self)
        layout.addWidget(QLabel('Dateien einzeln markieren. Pro Gruppe bleibt mindestens eine Datei erhalten.\nÄhnliche Fingerprints und Klänge sind Vorschläge; sie müssen vor dem Entfernen angehört und geprüft werden.'))
        self.tree=QTreeWidget(); self.tree.setHeaderLabels(['Gruppe / Datei','Format','Größe','Speicherort']); self.tree.setColumnWidth(0,400)
        potential=0
        for n,group in enumerate(groups,1):
            top=QTreeWidgetItem([f'{n:03d} · {group["type"]}']); self.tree.addTopLevelItem(top)
            for row in group['samples']:
                item=QTreeWidgetItem([row['name'],row['format'] or '',display(row['size'],'size'),row['path']])
                item.setCheckState(0,Qt.Unchecked); item.setData(0,Qt.UserRole,row['id']); top.addChild(item)
                self.items[id(item)]=(row,item,group)
            top.setExpanded(True)
            if group['verified']: potential+=sum(r['size'] for r in group['samples'])-max(r['size'] for r in group['samples'])
        layout.addWidget(self.tree)
        layout.addWidget(QLabel(f'{len(groups)} Gruppen · Obergrenze bei bestätigten Duplikaten: {potential/1048576:.2f} MB (überlappende Gruppen möglich)'))
        row=QHBoxLayout(); self.trash=QPushButton('Markierte Dateien in den Papierkorb …'); close=QPushButton('Schließen')
        row.addWidget(self.trash); row.addStretch(); row.addWidget(close); close.clicked.connect(self.reject); layout.addLayout(row)
    def selected_rows(self):
        selected={row['id']:row for row,item,group in self.items.values() if item.checkState(0)==Qt.Checked}
        for group in self.groups:
            if all(row['id'] in selected for row in group['samples']):
                raise ValueError('In jeder Duplikatgruppe muss mindestens eine Datei erhalten bleiben.')
        return list(selected.values())
