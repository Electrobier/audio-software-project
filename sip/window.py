import json
import os
import tempfile
from pathlib import Path
from PySide6.QtCore import Qt,QTimer,QSettings,QUrl,Signal,QStandardPaths
from PySide6.QtGui import QAction,QKeySequence,QDesktopServices,QShortcut
from PySide6.QtWidgets import (QMainWindow,QWidget,QVBoxLayout,QHBoxLayout,QLabel,QPushButton,
 QLineEdit,QComboBox,QSplitter,QTreeWidget,QTreeWidgetItem,QFrame,QScrollArea,QGridLayout,
 QDockWidget,QPlainTextEdit,QSlider,QCheckBox,QProgressBar,QFileDialog,QMessageBox,
 QInputDialog,QMenu,QDialog,QFormLayout,QDialogButtonBox,QSpinBox,QAbstractItemView)
from .storage import Store
from .widgets import SampleModel,SampleTable,Waveform,COLUMNS,display
from .services import ScanJob,SimilarJob,DuplicateJob,ExportJob,AnalyzeJob
from .player import Player,RecordJob
from .dialogs import FilterDialog,SimilarDialog,CategoryDialog,ExportDialog,DuplicateDialog
from .audio import waveform,LocalClassifier

STYLE='''
QWidget {background:#202326;color:#c7ced1;font-family:"Segoe UI","Inter",sans-serif;font-size:12px;}
QMainWindow,QDialog {background:#191c1f;}
QMenuBar,QMenuBar::item,QToolBar {background:#25292c;}
QMenuBar::item:selected,QMenu::item:selected {background:#354b48;color:#83e0cb;}
QMenu {border:1px solid #3b4448;padding:5px;}
QLabel#brand {font-size:15px;font-weight:700;color:#e0e9e6;letter-spacing:2px;}
QLabel#muted {color:#75858a;}
QPushButton {background:#2b3135;border:1px solid #3a4449;border-radius:4px;padding:6px 10px;}
QPushButton:hover {background:#37443f;border-color:#547e73;}
QPushButton:checked {background:#284b43;color:#8aefd7;border-color:#3c8976;}
QPushButton:disabled {color:#62696c;border-color:#2b3033;}
QPushButton#accent {background:#295a4f;color:#a8f3df;border-color:#3e8a77;}
QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox {background:#171b1e;border:1px solid #374248;border-radius:4px;padding:6px;selection-background-color:#366c5d;}
QLineEdit:focus {border-color:#5cac98;}
QTreeWidget,QListWidget {background:#1b1e21;border:0;outline:0;}
QTreeWidget::item {height:27px;}
QTreeWidget::item:selected,QListWidget::item:selected {background:#2c4540;color:#b9f2e1;}
QTableView {background:#202427;alternate-background-color:#23282b;selection-background-color:#344c46;selection-color:#e0f7ee;border:0;outline:0;}
QHeaderView::section {background:#2c3235;color:#91a4a9;padding:7px;border:0;border-right:1px solid #343d41;font-weight:600;}
QSplitter::handle {background:#30383c;width:3px;height:3px;}
QScrollBar:vertical {background:#1b1f22;width:10px;margin:0;}
QScrollBar::handle:vertical {background:#48565a;min-height:25px;border-radius:4px;}
QScrollBar:horizontal {background:#1b1f22;height:10px;}
QScrollBar::handle:horizontal {background:#48565a;min-width:30px;}
QScrollBar::add-line,QScrollBar::sub-line {width:0;height:0;}
QDockWidget::title {background:#293134;padding:7px;color:#9eb5b6;}
QPlainTextEdit {background:#1b1f22;border:0;padding:8px;color:#aab8bb;}
QStatusBar {background:#172c27;color:#8cc9b8;}
QProgressBar {background:#19221f;border:0;border-radius:3px;text-align:center;max-height:17px;}
QProgressBar::chunk {background:#367b65;border-radius:3px;}
QSlider::groove:horizontal {height:4px;background:#3d494b;}
QSlider::handle:horizontal {background:#70cbb2;width:10px;margin:-5px 0;border-radius:5px;}
QToolTip {background:#303b3d;color:#d4e7df;border:1px solid #55756b;}
'''

class LibraryTree(QTreeWidget):
    assign=Signal(object,object)
    def __init__(self,parent=None):
        super().__init__(parent); self.setAcceptDrops(True); self.setHeaderHidden(True)
        self.setDragDropMode(QAbstractItemView.DropOnly)
    def dragEnterEvent(self,event):
        if event.mimeData().hasFormat('application/x-sip-ids'): event.acceptProposedAction()
        else: event.ignore()
    def dragMoveEvent(self,event):
        item=self.itemAt(event.position().toPoint())
        data=item.data(0,Qt.UserRole) if item else None
        if data and data[0] in ('project','favorite') and event.mimeData().hasFormat('application/x-sip-ids'): event.acceptProposedAction()
        else: event.ignore()
    def dropEvent(self,event):
        item=self.itemAt(event.position().toPoint())
        data=item.data(0,Qt.UserRole) if item else None
        if data and data[0] in ('project','favorite'):
            ids=json.loads(bytes(event.mimeData().data('application/x-sip-ids')))
            self.assign.emit(ids,data); event.acceptProposedAction()

class Window(QMainWindow):
    def __init__(self,dbpath):
        super().__init__(); self.setWindowTitle('SAMPLE INTELLIGENCE PRO'); self.resize(1510,940); self.setMinimumSize(1060,680)
        self.store=Store(dbpath); self.settings=QSettings('SampleIntelligence','Pro')
        self.filters={}; self.page=0; self.page_size=500; self.sort_key='name'; self.desc=False
        self.jobs=[]; self.scan=None; self.results=None; self.current=None; self.recording=None
        self.temp=tempfile.TemporaryDirectory(prefix='sip-reference-'); self.player=Player(self)
        self.setStyleSheet(STYLE); self.build_ui(); self.build_menu(); self.rebuild_tree(); self.refresh()
        self.search_timer=QTimer(self); self.search_timer.setSingleShot(True); self.search_timer.setInterval(200)
        self.search_timer.timeout.connect(self.apply_search)
        self.search.textChanged.connect(lambda:self.search_timer.start())
        self.player.ready.connect(self.player_ready); self.player.changed.connect(self.player_changed)
        self.player.error.connect(self.error)
        self.wave.seek.connect(self.player.seek)
        self.settings_restore()
        QShortcut(QKeySequence('Space'),self,activated=self.player.toggle)
        QShortcut(QKeySequence('Ctrl+F'),self,activated=self.search.setFocus)
        QShortcut(QKeySequence('Ctrl+O'),self,activated=self.add_library)
        QShortcut(QKeySequence('F5'),self,activated=self.start_scan)
        QShortcut(QKeySequence('Escape'),self,activated=self.player.stop)
        self.rescan_timer=QTimer(self); self.rescan_timer.setInterval(60000)
        self.rescan_timer.timeout.connect(self.watch_scan)
        self.auto_scan.toggled.connect(lambda checked:self.rescan_timer.start() if checked else self.rescan_timer.stop())

    def button(self,text,callback,checkable=False):
        b=QPushButton(text); b.setCheckable(checkable); b.clicked.connect(callback); return b
    def build_ui(self):
        central=QWidget(); self.setCentralWidget(central); outer=QVBoxLayout(central); outer.setContentsMargins(0,0,0,0); outer.setSpacing(0)
        top=QHBoxLayout(); top.setContentsMargins(15,10,12,10)
        brand=QLabel('▥  SAMPLE INTELLIGENCE'); brand.setObjectName('brand'); top.addWidget(brand)
        pro=QLabel('PRO  /  LOCAL AUDIO WORKSPACE'); pro.setObjectName('muted'); top.addWidget(pro); top.addStretch()
        self.add_btn=self.button('+ Bibliothek',self.add_library); top.addWidget(self.add_btn)
        self.scan_btn=self.button('↻ Scannen',self.start_scan); top.addWidget(self.scan_btn)
        self.rec_btn=self.button('● Record Audio',self.record); top.addWidget(self.rec_btn)
        self.ref_btn=self.button('Referenzdatei',self.reference); top.addWidget(self.ref_btn)
        self.sim_btn=self.button('Find Similar',self.find_similar); self.sim_btn.setObjectName('accent'); top.addWidget(self.sim_btn)
        outer.addLayout(top)
        self.main_split=QSplitter(Qt.Horizontal); outer.addWidget(self.main_split,1)
        side=QWidget(); side.setMinimumWidth(180); sl=QVBoxLayout(side); sl.setContentsMargins(8,8,8,8)
        title=QLabel('BIBLIOTHEKEN & PROJEKTE'); title.setObjectName('muted'); sl.addWidget(title)
        self.tree=LibraryTree(); self.tree.itemClicked.connect(self.tree_clicked); self.tree.assign.connect(self.assign_drop)
        self.tree.setContextMenuPolicy(Qt.CustomContextMenu); self.tree.customContextMenuRequested.connect(self.tree_menu); sl.addWidget(self.tree,1)
        sl.addWidget(self.button('+ Projekt / Sammlung',self.create_project))
        self.auto_scan=QCheckBox('Änderungen jede Minute prüfen'); sl.addWidget(self.auto_scan)
        self.auto_play=QCheckBox('Automatisch vorhören'); sl.addWidget(self.auto_play)
        foot=QLabel('OFFLINE  •  SQLite + DSP'); foot.setObjectName('muted'); sl.addWidget(foot)
        self.main_split.addWidget(side)
        workspace=QWidget(); wl=QVBoxLayout(workspace); wl.setContentsMargins(0,0,0,0); wl.setSpacing(0)
        self.vertical_split=QSplitter(Qt.Vertical)
        player=QWidget(); pl=QVBoxLayout(player); pl.setContentsMargins(0,0,0,0); pl.setSpacing(0)
        self.now=QLabel('  Keine Datei ausgewählt'); self.now.setMinimumHeight(27); self.now.setObjectName('muted'); pl.addWidget(self.now)
        self.wave=Waveform(); pl.addWidget(self.wave,1)
        controls=QHBoxLayout(); controls.setContentsMargins(8,4,8,6)
        controls.addWidget(self.button('⏮',lambda:self.step_sample(-1)))
        self.play_btn=self.button('▶',self.player.toggle); controls.addWidget(self.play_btn)
        controls.addWidget(self.button('■',self.player.stop)); controls.addWidget(self.button('⏭',lambda:self.step_sample(1)))
        controls.addWidget(self.button('−5s',lambda:self.player.seek(self.player.position/self.player.sr-5)))
        controls.addWidget(self.button('+5s',lambda:self.player.seek(self.player.position/self.player.sr+5)))
        self.loop_btn=self.button('Loop',lambda checked:setattr(self.player,'loop',checked),True); controls.addWidget(self.loop_btn)
        self.time=QLabel('0.00 / 0.00 s'); controls.addWidget(self.time); controls.addStretch()
        controls.addWidget(QLabel('Zoom')); self.zoom=QSlider(Qt.Horizontal); self.zoom.setRange(1,32); self.zoom.setValue(1); self.zoom.setFixedWidth(80)
        self.zoom.valueChanged.connect(self.set_zoom); controls.addWidget(self.zoom)
        controls.addWidget(QLabel('VOL')); volume=QSlider(Qt.Horizontal); volume.setRange(0,100); volume.setValue(70); volume.setFixedWidth(90)
        volume.valueChanged.connect(lambda v:setattr(self.player,'volume',v/100)); controls.addWidget(volume); pl.addLayout(controls)
        self.vertical_split.addWidget(player)
        browser=QWidget(); bl=QVBoxLayout(browser); bl.setContentsMargins(0,0,0,0); bl.setSpacing(0)
        searchrow=QHBoxLayout(); searchrow.setContentsMargins(8,6,8,6)
        self.search=QLineEdit(); self.search.setPlaceholderText('Dateiname, Tags oder Pfad durchsuchen …  Ctrl+F'); searchrow.addWidget(self.search,1)
        self.kind=QComboBox(); self.kind.addItems(['Alle Typen','Loop','One-Shot','Unbekannt']); self.kind.currentIndexChanged.connect(self.quick_filter); searchrow.addWidget(self.kind)
        self.format=QComboBox(); self.format.addItems(['Alle Formate','WAV','AIFF','FLAC','OGG','MP3','M4A','AAC','WMA','OPUS']); self.format.currentIndexChanged.connect(self.quick_filter); searchrow.addWidget(self.format)
        searchrow.addWidget(self.button('Filter …',self.advanced_filters)); searchrow.addWidget(self.button('Zurücksetzen',self.reset_filters))
        bl.addLayout(searchrow)
        self.context_label=QLabel('  Alle Samples'); self.context_label.setMinimumHeight(26); self.context_label.setStyleSheet('background:#263d33;color:#a8ceb7;padding-left:6px;'); bl.addWidget(self.context_label)
        self.model=SampleModel(self); self.table=SampleTable(); self.table.setModel(self.model)
        self.table.selectionModel().currentRowChanged.connect(self.select_sample)
        self.table.doubleClicked.connect(lambda:self.player.play()); self.table.clicked.connect(self.table_clicked)
        self.table.setContextMenuPolicy(Qt.CustomContextMenu); self.table.customContextMenuRequested.connect(self.sample_menu)
        self.table.horizontalHeader().sectionClicked.connect(self.sort_table)
        self.table.horizontalHeader().customContextMenuRequested.connect(self.column_menu)
        for i,(key,label) in enumerate(COLUMNS):
            self.table.setColumnWidth(i,270 if key=='name' else 120 if key=='category' else 80)
        self.table.setColumnWidth(len(COLUMNS)-1,400)
        bl.addWidget(self.table,1)
        pager=QHBoxLayout(); pager.setContentsMargins(8,4,8,4)
        self.count_label=QLabel(); pager.addWidget(self.count_label); pager.addStretch()
        self.prev=self.button('‹ Zurück',lambda:self.change_page(-1)); self.next=self.button('Weiter ›',lambda:self.change_page(1))
        pager.addWidget(self.prev); pager.addWidget(self.next); bl.addLayout(pager)
        self.vertical_split.addWidget(browser)
        cats=QWidget(); cl=QVBoxLayout(cats); cl.setContentsMargins(8,6,8,6)
        catrow=QHBoxLayout(); catrow.addWidget(QLabel('KATEGORIEN')); catrow.addStretch(); catrow.addWidget(self.button('+ Kategorie',self.add_category)); cl.addLayout(catrow)
        self.cat_scroll=QScrollArea(); self.cat_scroll.setWidgetResizable(True); self.cat_scroll.setMinimumHeight(105); cl.addWidget(self.cat_scroll)
        self.vertical_split.addWidget(cats); self.vertical_split.setSizes([235,460,165]); wl.addWidget(self.vertical_split)
        self.main_split.addWidget(workspace); self.main_split.setSizes([220,1250]); self.main_split.setStretchFactor(1,1)
        self.inspector_dock=QDockWidget('INSPEKTOR / ANALYSE',self); self.inspector_dock.setObjectName('inspector'); self.inspector_dock.setAllowedAreas(Qt.LeftDockWidgetArea|Qt.RightDockWidgetArea)
        inspect=QWidget(); il=QVBoxLayout(inspect); self.inspect_title=QLabel('Kein Sample'); self.inspect_title.setWordWrap(True); il.addWidget(self.inspect_title)
        self.details=QPlainTextEdit(); self.details.setReadOnly(True); il.addWidget(self.details,1)
        il.addWidget(self.button('Kategorien bearbeiten …',self.edit_categories)); il.addWidget(self.button('Tags bearbeiten …',self.edit_tags))
        il.addWidget(self.button('★ Favorit umschalten',self.toggle_favorite)); il.addWidget(self.button('In Projekt aufnehmen …',self.add_to_project))
        self.inspector_dock.setWidget(inspect); self.addDockWidget(Qt.RightDockWidgetArea,self.inspector_dock); self.inspector_dock.setMinimumWidth(240)
        self.log_dock=QDockWidget('Analyseprotokoll',self); self.log_dock.setObjectName('log'); self.log=QPlainTextEdit(); self.log.setReadOnly(True); self.log_dock.setWidget(self.log)
        self.addDockWidget(Qt.BottomDockWidgetArea,self.log_dock); self.log_dock.hide()
        self.status=QLabel('Bereit'); self.statusBar().addWidget(self.status,1)
        self.progress=QProgressBar(); self.progress.setFixedWidth(190); self.progress.hide(); self.statusBar().addPermanentWidget(self.progress)
        self.cancel_btn=self.button('Abbrechen',self.cancel_jobs); self.cancel_btn.hide(); self.statusBar().addPermanentWidget(self.cancel_btn)
        self.rebuild_categories()

    def action(self,menu,title,callback,shortcut=None):
        action=QAction(title,self); action.triggered.connect(callback)
        if shortcut: action.setShortcut(shortcut)
        menu.addAction(action); return action
    def build_menu(self):
        file=self.menuBar().addMenu('Datei'); self.action(file,'Bibliothek hinzufügen …',self.add_library)
        self.action(file,'Bibliotheken scannen / fortsetzen',self.start_scan)
        self.action(file,'Alle Dateien neu analysieren …',self.force_scan)
        self.action(file,'Auswahl exportieren …',self.export_selected,'Ctrl+E'); file.addSeparator(); self.action(file,'Beenden',self.close,'Ctrl+Q')
        manage=self.menuBar().addMenu('Organisieren'); self.action(manage,'Projekt / Sammlung erstellen …',self.create_project)
        self.action(manage,'Auswahl zu Projekt …',self.add_to_project); self.action(manage,'Kategorien bearbeiten …',self.edit_categories)
        self.action(manage,'Tags bearbeiten …',self.edit_tags); self.action(manage,'Duplikate prüfen …',self.duplicates)
        self.action(manage,'Klassifikator auf Bibliothek anwenden',self.reclassify)
        search=self.menuBar().addMenu('Suche'); self.action(search,'Find Similar …',self.find_similar)
        self.action(search,'Externe Referenz …',self.reference); self.action(search,'Audio aufnehmen …',self.record)
        self.action(search,'Suche speichern …',self.save_search)
        view=self.menuBar().addMenu('Ansicht'); view.addAction(self.inspector_dock.toggleViewAction()); view.addAction(self.log_dock.toggleViewAction())
        self.action(view,'Audioausgabegerät …',self.choose_output)
        helpmenu=self.menuBar().addMenu('Hilfe'); self.action(helpmenu,'Bedienung / Grenzen',self.help)
        self.action(helpmenu,'Datenordner öffnen',lambda:QDesktopServices.openUrl(QUrl.fromLocalFile(str(Path(self.store.path).parent))))

    def settings_restore(self):
        geometry=self.settings.value('geometry'); state=self.settings.value('state'); header=self.settings.value('header')
        if geometry: self.restoreGeometry(geometry)
        if state: self.restoreState(state)
        if header: self.table.horizontalHeader().restoreState(header)
    def rebuild_categories(self):
        panel=QWidget(); grid=QGridLayout(panel); grid.setSpacing(3)
        all_btn=self.button('Alle Kategorien',lambda:self.category_filter(None)); grid.addWidget(all_btn,0,0)
        for n,category in enumerate(self.store.categories(),1):
            b=self.button(category,lambda checked=False,c=category:self.category_filter(c)); b.setStyleSheet('border-left:2px solid #548f7f;text-align:left;padding:5px 8px;')
            grid.addWidget(b,n//6,n%6)
        self.cat_scroll.setWidget(panel)
    def tree_item(self,parent,text,data=None):
        item=QTreeWidgetItem([text])
        if parent: parent.addChild(item)
        else: self.tree.addTopLevelItem(item)
        item.setData(0,Qt.UserRole,data); return item
    def rebuild_tree(self):
        self.tree.clear(); self.tree_item(None,'▤  Alle Samples',('all',None)); self.tree_item(None,'★  Favoriten',('favorite',None))
        libs=self.tree_item(None,'BIBLIOTHEKEN')
        for lib in self.store.libraries():
            item=self.tree_item(libs,Path(lib['path']).name,('library',lib['id'])); item.setToolTip(0,lib['path'])
            # Build folder tree from indexed paths; no recursive disk IO on the GUI thread.
            directories=self.store.rows('SELECT DISTINCT substr(path,1,length(path)-length(name)) folder FROM samples WHERE library_id=? LIMIT 1000',(lib['id'],))
            for folder in directories:
                p=folder['folder']; relative=os.path.relpath(p,lib['path'])
                if relative!='.': self.tree_item(item,relative,('folder',p))
        libs.setExpanded(True)
        projects=self.tree_item(None,'PROJEKTE & SAMMLUNGEN')
        for p in self.store.rows('SELECT * FROM projects ORDER BY name'):
            self.tree_item(projects,('▣ ' if p['kind']=='Projekt' else '▧ ')+p['name'],('project',p['id']))
        projects.setExpanded(True); saved=self.tree_item(None,'GESPEICHERTE SUCHEN')
        for s in self.store.rows('SELECT * FROM searches ORDER BY name'): self.tree_item(saved,s['name'],('search',s['id']))
        saved.setExpanded(True)
    def tree_clicked(self,item,column):
        data=item.data(0,Qt.UserRole)
        if not data: return
        kind,value=data
        for key in ('library_id','favorite','project_id','folder'): self.filters.pop(key,None)
        if kind=='library': self.filters['library_id']=value
        if kind=='favorite': self.filters['favorite']=True
        if kind=='project': self.filters['project_id']=value
        if kind=='folder': self.filters['folder']=value
        if kind=='search':
            row=self.store.rows('SELECT data FROM searches WHERE id=?',(value,))[0]; self.filters=json.loads(row['data']); self.sync_filter_controls()
        self.page=0; self.results=None; self.refresh()
    def tree_menu(self,pos):
        item=self.tree.itemAt(pos); data=item.data(0,Qt.UserRole) if item else None
        if not data: return
        menu=QMenu(self)
        if data[0]=='library':
            self.action(menu,'Ordner öffnen',lambda:self.open_library(data[1]))
            self.action(menu,'Bibliothek aus Index entfernen …',lambda:self.remove_library(data[1]))
        if data[0]=='project':
            self.action(menu,'Auswahl zuweisen',lambda:self.assign_drop([r['id'] for r in self.selected()],data))
            self.action(menu,'Projekt / Sammlung entfernen …',lambda:self.remove_entity('projects',data[1]))
        if data[0]=='search': self.action(menu,'Gespeicherte Suche entfernen',lambda:self.remove_entity('searches',data[1]))
        if not menu.isEmpty(): menu.exec(self.tree.viewport().mapToGlobal(pos))
    def remove_entity(self,table,id):
        if QMessageBox.question(self,'Entfernen','Eintrag entfernen? Originaldateien bleiben erhalten.')==QMessageBox.Yes:
            self.store.execute(f'DELETE FROM {table} WHERE id=?',(id,)); self.reset_filters(); self.rebuild_tree()
    def open_library(self,id):
        row=self.store.rows('SELECT path FROM libraries WHERE id=?',(id,))[0]; QDesktopServices.openUrl(QUrl.fromLocalFile(row['path']))
    def remove_library(self,id):
        if self.scan and self.scan.isRunning(): self.info('Bitte zuerst den laufenden Scan abbrechen.'); return
        if QMessageBox.question(self,'Bibliothek entfernen','Bibliothek und ihre Metadaten aus dem Index entfernen? Die Audiodateien bleiben erhalten.')==QMessageBox.Yes:
            self.store.execute('DELETE FROM libraries WHERE id=?',(id,)); self.rebuild_tree(); self.reset_filters()
    def add_library(self):
        path=QFileDialog.getExistingDirectory(self,'Sample-Bibliothek hinzufügen')
        if path:
            self.store.add_library(path); self.rebuild_tree(); self.start_scan()
    def start_scan(self,force=False):
        if self.scan and self.scan.isRunning(): self.info('Ein Scan läuft bereits. „Abbrechen“ pausiert ihn; erneutes Scannen setzt bei unverarbeiteten Dateien fort.'); return
        libraries=self.store.libraries()
        if not libraries: self.add_library(); return
        self.scan=ScanJob(self.store.path,libraries,force,self)
        self.scan.result.connect(self.scan_done); self.start_job(self.scan,'Bibliothek analysieren')
    def force_scan(self):
        if QMessageBox.question(self,'Neu analysieren','Alle Dateien erneut analysieren? Manuelle Kategorien und Tags bleiben erhalten.')==QMessageBox.Yes: self.start_scan(True)
    def watch_scan(self):
        if self.store.libraries() and not any(j.isRunning() for j in self.jobs): self.start_scan()
    def scan_done(self,result):
        self.rebuild_tree(); self.refresh()
        for err in result['errors']: self.log.appendPlainText(err)
        text=f'{result["updated"]} analysiert · {result["skipped"]} unverändert · {len(result["errors"])} Fehler'
        if result['cancelled']: text='Pausiert · '+text
        self.status.setText(text); self.log.appendPlainText(text)
        if result['errors']: self.log_dock.show()
    def start_job(self,job,label):
        self.jobs.append(job); self.status.setText(label); self.progress.setRange(0,0); self.progress.show(); self.cancel_btn.show()
        job.message.connect(self.status.setText); job.progress.connect(self.on_progress); job.failed.connect(self.error)
        job.finished.connect(lambda:self.finish_job(job)); job.start()
    def finish_job(self,job):
        if job in self.jobs: self.jobs.remove(job)
        if not self.jobs: self.progress.hide(); self.cancel_btn.hide()
        job.deleteLater()
        if self.scan is job: self.scan=None
        if self.recording is job: self.recording=None; self.rec_btn.setText('● Record Audio')
    def on_progress(self,done,total): self.progress.setRange(0,max(1,total)); self.progress.setValue(done)
    def cancel_jobs(self):
        for job in self.jobs: job.cancel()
        self.status.setText('Abbruch angefordert … laufende Dateianalyse wird beendet.')
    def refresh(self):
        if self.results is not None:
            scores=dict(self.results); all_rows=[]
            for sid,score in self.results[self.page*self.page_size:(self.page+1)*self.page_size]:
                row=self.store.get(sid)
                if row and not row['missing']: all_rows.append(row)
            self.total=len(self.results); self.model.set_rows(all_rows,scores)
        else:
            self.total=self.store.query(self.filters,count=True)
            rows=self.store.query(self.filters,self.sort_key,self.desc,self.page_size,self.page*self.page_size)
            self.model.set_rows(rows)
        self.prev.setEnabled(self.page>0); self.next.setEnabled((self.page+1)*self.page_size<self.total)
        self.count_label.setText(f'{self.total:,} Samples · Seite {self.page+1} / {max(1,(self.total+self.page_size-1)//self.page_size)}'.replace(',','.'))
        bits=[]
        for k,v in self.filters.items():
            if v is not None and v!='': bits.append(f'{k}: {v}')
        self.context_label.setText('  '+('Ähnlichkeitsergebnisse · '+str(len(self.results))+' Treffer' if self.results is not None else '  /  '.join(bits) or 'Alle Samples'))
    def change_page(self,direction): self.page=max(0,self.page+direction); self.refresh()
    def sort_table(self,col):
        if col>=len(COLUMNS):
            if self.results is not None: self.results.reverse(); self.page=0; self.refresh()
            return
        key=COLUMNS[col][0]
        self.desc=not self.desc if self.sort_key==key else False; self.sort_key=key
        self.table.horizontalHeader().setSortIndicator(col,Qt.DescendingOrder if self.desc else Qt.AscendingOrder)
        self.table.horizontalHeader().setSortIndicatorShown(True)
        if self.results is not None:
            # Similarity list is bounded; sort metadata without fetching the whole database.
            cache={sid:self.store.get(sid) for sid,score in self.results}
            self.results.sort(key=lambda x:(cache[x[0]].get(key) is None,cache[x[0]].get(key) if cache[x[0]].get(key) is not None else 0),reverse=self.desc)
        self.page=0; self.refresh()
    def apply_search(self):
        self.filters['text']=self.search.text(); self.page=0; self.results=None; self.refresh()
    def quick_filter(self):
        self.filters.pop('kind',None); self.filters.pop('format',None)
        if self.kind.currentIndex(): self.filters['kind']=self.kind.currentText()
        if self.format.currentIndex(): self.filters['format']=self.format.currentText()
        self.page=0; self.results=None; self.refresh()
    def category_filter(self,category):
        self.filters.pop('category',None)
        if category: self.filters['category']=category
        self.page=0; self.results=None; self.refresh()
    def advanced_filters(self):
        dialog=FilterDialog(self.filters,self)
        if dialog.exec(): self.filters=dialog.result_filters(); self.page=0; self.results=None; self.refresh()
    def sync_filter_controls(self):
        for obj in (self.search,self.kind,self.format): obj.blockSignals(True)
        self.search.setText(self.filters.get('text','')); self.kind.setCurrentText(self.filters.get('kind','Alle Typen')); self.format.setCurrentText(self.filters.get('format','Alle Formate'))
        for obj in (self.search,self.kind,self.format): obj.blockSignals(False)
    def reset_filters(self):
        self.filters={}; self.page=0; self.results=None; self.sync_filter_controls(); self.refresh()
    def selected(self):
        return [self.model.rows[index.row()] for index in self.table.selectionModel().selectedRows()]
    def select_sample(self,index,previous):
        if not index.isValid(): return
        row=self.model.rows[index.row()]; self.current=row['id']; self.now.setText('  '+row['name']); self.inspect(row)
        data=json.loads(row['data']); self.wave.set_audio(data.get('waveform',[]),row.get('duration') or 0)
        self.player.load(row['path'],self.auto_play.isChecked())
    def inspect(self,row):
        d=json.loads(row['data']); self.inspect_title.setText(row['name'])
        lines=[row['path'],'',f'Format: {row.get("format") or "unbekannt"} · {row.get("bit_depth") or "unbekannt"}',
               f'Kategorien: {", ".join(json.loads(row["manual_categories"]) if row["manual_categories"] else d.get("categories",[])) or "unbekannt"}',
               f'Quelle: {"manuell / Trainingsbeispiel" if row["manual_categories"] else d.get("classifier","unbekannt")}',
               f'KI-Score / Beispielähnlichkeit: {display(row.get("confidence"),"confidence")}',
               f'Tags: {row.get("tags") or "—"}','']
        labels=[('duration','Dauer (s)'),('sr','Sample-Rate'),('channels','Kanäle'),('bpm','BPM'),('musical_key','Tonart'),('kind','Loop / One-Shot'),
          ('peak','Peak (dBFS)'),('true_peak','True Peak, Schätzung (dBTP)'),('rms','RMS (dBFS)'),('lufs','LUFS Integrated'),('crest','Crest (dB)'),('dynamics','Dynamik P95–P10 (dB)'),
          ('silence','Stilleanteil'),('centroid','Schwerpunkt (Hz)'),('brightness','Helligkeit'),('bandwidth','Bandbreite (Hz)'),('rolloff','Rolloff (Hz)'),('flatness','Flatness'),
          ('zcr','Zero Crossing Rate'),('transient','Transientenstärke'),('harmonic','Harmonischer Anteil'),('percussive','Perkussiver Anteil'),('attack','Attack (s)'),('decay','Decay (s)'),('rhythm_confidence','Beat-Regelmäßigkeit')]
        for key,label in labels:
            value=d.get(key)
            lines.append(f'{label}: {display(value,key)}')
        lines += ['',f'Analyseausschnitt: {d.get("analysis_seconds",0):.1f} s (max. 90 s)',
                  f'LUFS: {d.get("lufs_scope","unbekannt")}',f'Beats: {len(d.get("beats",[]))}',
                  'Frequenzbänder: '+', '.join(f'{v:.0%}' for v in d.get('bands',[])),
                  'Grenzen: 0 / 60 / 120 / 250 / 500 / 1k / 2k / 4k / 8k / 11k Hz', '', 'KI-Audioereignisse (EfficientAT):']
        lines += [f'{label}: {score:.1%}' for label,score in d.get('audio_events',[])]
        if row.get('error'): lines += ['', 'FEHLER: '+row['error']]
        self.details.setPlainText('\n'.join(lines))
    def player_ready(self,data,sr,peaks):
        # Prefer cached peaks. Full-length overview is computed on load only for references.
        self.wave.set_audio(peaks,len(data)/sr)
    def player_changed(self,pos,duration,playing):
        self.time.setText(f'{pos:.2f} / {duration:.2f} s'); self.wave.set_position(pos); self.play_btn.setText('Ⅱ' if playing else '▶')
    def set_zoom(self,value): self.wave.zoom=float(value); self.wave.update()
    def step_sample(self,direction):
        row=self.table.currentIndex().row(); new=max(0,min(len(self.model.rows)-1,row+direction))
        if self.model.rows: self.table.selectRow(new)
    def table_clicked(self,index):
        if index.column()==0: self.toggle_favorite()
    def toggle_favorite(self):
        rows=self.selected()
        if not rows and self.current:
            row=self.store.get(self.current); rows=[row] if row else []
        for row in rows: self.store.execute('UPDATE samples SET favorite=1-favorite WHERE id=?',(row['id'],))
        self.refresh()
    def sample_menu(self,pos):
        if not self.selected(): return
        menu=QMenu(self); self.action(menu,'▶ Wiedergabe',self.player.play); self.action(menu,'Find Similar …',self.find_similar)
        self.action(menu,'★ Favorit umschalten',self.toggle_favorite); self.action(menu,'Kategorien …',self.edit_categories)
        self.action(menu,'Tags …',self.edit_tags); self.action(menu,'Zu Projekt / Sammlung …',self.add_to_project)
        if self.filters.get('project_id'): self.action(menu,'Aus diesem Projekt entfernen',self.remove_from_project)
        self.action(menu,'Exportieren …',self.export_selected); self.action(menu,'Datei im Ordner anzeigen',self.reveal)
        menu.exec(self.table.viewport().mapToGlobal(pos))
    def column_menu(self,pos):
        menu=QMenu(self)
        for i,(key,label) in enumerate(COLUMNS):
            action=QAction(label,menu); action.setCheckable(True); action.setChecked(not self.table.isColumnHidden(i))
            action.toggled.connect(lambda visible,col=i:self.table.setColumnHidden(col,not visible)); menu.addAction(action)
        menu.exec(self.table.horizontalHeader().mapToGlobal(pos))
    def reveal(self):
        rows=self.selected()
        if rows:
            import subprocess
            if os.name=='nt': subprocess.Popen(['explorer','/select,',str(Path(rows[0]['path']))])
            else: QDesktopServices.openUrl(QUrl.fromLocalFile(str(Path(rows[0]['path']).parent)))
    def edit_categories(self):
        rows=self.selected()
        if not rows: self.info('Bitte mindestens ein Sample auswählen.'); return
        row=rows[0]; selected=json.loads(row['manual_categories']) if row['manual_categories'] else json.loads(row['data']).get('categories',[])
        dialog=CategoryDialog(self.store.categories(),selected,self)
        if dialog.exec():
            self.store.set_categories([r['id'] for r in rows],dialog.labels()); self.refresh()
            if self.current and self.store.get(self.current): self.inspect(self.store.get(self.current))
            self.status.setText('Zuordnung gespeichert. „Organisieren → Klassifikator anwenden“ übernimmt sie für andere Samples.')
    def add_category(self):
        name,ok=QInputDialog.getText(self,'Kategorie erweitern','Neue Kategorie:')
        if ok and name.strip():
            self.store.execute('INSERT OR IGNORE INTO categories VALUES(?)',(name.strip(),)); self.rebuild_categories()
    def edit_tags(self):
        rows=self.selected()
        if not rows: self.info('Bitte Samples auswählen.'); return
        tags,ok=QInputDialog.getText(self,'Tags','Tags, durch Kommas getrennt (ersetzt vorhandene Tags):',text=rows[0]['tags'])
        if ok:
            for row in rows: self.store.execute('UPDATE samples SET tags=? WHERE id=?',(tags.strip(),row['id']))
            self.refresh()
    def create_project(self):
        name,ok=QInputDialog.getText(self,'Projekt / Sammlung','Name:')
        if not ok or not name.strip(): return
        kind,ok=QInputDialog.getItem(self,'Typ','Art:',['Projekt','Sammlung'],0,False)
        if ok:
            self.store.execute('INSERT OR IGNORE INTO projects(name,kind) VALUES(?,?)',(name.strip(),kind)); self.rebuild_tree()
    def assign_drop(self,ids,data):
        if data[0]=='favorite':
            for sid in ids: self.store.db.execute('UPDATE samples SET favorite=1 WHERE id=?',(sid,))
        elif data[0]=='project':
            for sid in ids: self.store.db.execute('INSERT OR IGNORE INTO project_samples VALUES(?,?)',(data[1],sid))
        self.store.db.commit(); self.status.setText(f'{len(ids)} Samples zugeordnet'); self.refresh()
    def add_to_project(self):
        rows=self.selected(); projects=self.store.rows('SELECT * FROM projects ORDER BY name')
        if not rows: self.info('Bitte Samples auswählen.'); return
        if not projects: self.create_project(); projects=self.store.rows('SELECT * FROM projects ORDER BY name')
        if not projects: return
        name,ok=QInputDialog.getItem(self,'Projekt zuweisen','Projekt / Sammlung:',[p['name'] for p in projects],0,False)
        if ok:
            p=next(p for p in projects if p['name']==name); self.assign_drop([r['id'] for r in rows],('project',p['id']))
    def remove_from_project(self):
        for row in self.selected(): self.store.execute('DELETE FROM project_samples WHERE project_id=? AND sample_id=?',(self.filters['project_id'],row['id']))
        self.refresh()
    def save_search(self):
        if self.results is not None: self.info('Gespeichert werden Filterabfragen. Bitte zuerst die Ähnlichkeitssuche zurücksetzen.'); return
        name,ok=QInputDialog.getText(self,'Suche speichern','Name:')
        if ok and name.strip():
            self.store.execute('INSERT INTO searches(name,data) VALUES(?,?) ON CONFLICT(name) DO UPDATE SET data=excluded.data',(name.strip(),json.dumps(self.filters))); self.rebuild_tree()
    def find_similar(self,reference=None):
        if not reference or isinstance(reference,bool):
            rows=self.selected()
            if not rows: self.info('Ein Sample auswählen oder eine Referenzdatei importieren.'); return
            reference=rows[0]
        dialog=SimilarDialog(self)
        if dialog.exec():
            job=SimilarJob(self.store.path,reference,dialog.mode.currentText(),dialog.threshold.value()/100,{k:s.value() for k,s in dialog.weights.items()},self,filters=dict(self.filters))
            job.result.connect(self.similar_done); self.start_job(job,'Audioähnlichkeit berechnen')
    def similar_done(self,result): self.results=result; self.page=0; self.refresh(); self.status.setText(f'{len(result)} ähnliche Samples gefunden')
    def reference(self):
        path,unused=QFileDialog.getOpenFileName(self,'Externe Referenz (wird nicht zur Bibliothek hinzugefügt)','','Audio (*.wav *.aif *.aiff *.flac *.ogg *.mp3 *.m4a *.opus);;Alle Dateien (*)')
        if path: self.find_similar(path)
    def record(self):
        if self.recording and self.recording.isRunning(): self.recording.cancel(); return
        try:
            import sounddevice as sd
            devices=[(i,d['name']) for i,d in enumerate(sd.query_devices()) if d['max_input_channels']>0]
        except Exception as exc: self.error(str(exc)); return
        if not devices: self.info('Kein Audioeingabegerät gefunden.'); return
        device,ok=QInputDialog.getItem(self,'Audio aufnehmen','Eingabegerät:',[f'{i}: {name}' for i,name in devices],0,False)
        if not ok: return
        seconds,ok=QInputDialog.getInt(self,'Aufnahmedauer','Sekunden:',5,1,30)
        if not ok: return
        self.player.stop(); path=str(Path(self.temp.name)/'recording.wav')
        self.recording=RecordJob(path,seconds,int(device.split(':',1)[0]),self); self.recording.result.connect(self.find_similar)
        self.rec_btn.setText('■ Aufnahme stoppen'); self.start_job(self.recording,'Aufnahme läuft …')
    def choose_output(self):
        try:
            import sounddevice as sd
            devices=[(i,d['name']) for i,d in enumerate(sd.query_devices()) if d['max_output_channels']>0]
            choice,ok=QInputDialog.getItem(self,'Ausgabegerät','Gerät:',['Systemstandard']+[f'{i}: {name}' for i,name in devices],0,False)
            if ok: self.player.stop(); self.player.device=None if choice=='Systemstandard' else int(choice.split(':',1)[0])
        except Exception as exc: self.error(str(exc))
    def duplicates(self):
        job=DuplicateJob(self.store.path,self); job.result.connect(self.show_duplicates); self.start_job(job,'Duplikate prüfen')
    def show_duplicates(self,groups):
        if not groups: self.info('Keine Duplikatgruppen oder nahen Kandidaten gefunden.'); return
        dialog=DuplicateDialog(groups,self)
        dialog.trash.clicked.connect(lambda:self.trash_duplicates(dialog)); dialog.exec()
    def trash_duplicates(self,dialog):
        try: rows=dialog.selected_rows()
        except ValueError as exc: self.error(str(exc)); return
        if not rows: self.info('Bitte Dateien einzeln markieren.'); return
        preview='\n'.join(r['path'] for r in rows[:12])
        if len(rows)>12: preview+=f'\n… und {len(rows)-12} weitere'
        if QMessageBox.warning(self,'In den Papierkorb verschieben',f'{len(rows)} Dateien in den Systempapierkorb verschieben?\n\n{preview}',QMessageBox.Yes|QMessageBox.No,QMessageBox.No)!=QMessageBox.Yes: return
        self.player.stop(); errors=[]
        from send2trash import send2trash
        for row in rows:
            try: send2trash(row['path']); self.store.execute('UPDATE samples SET missing=1 WHERE id=?',(row['id'],))
            except Exception as exc: errors.append(f'{row["name"]}: {exc}')
        dialog.accept(); self.refresh()
        if errors: self.error('\n'.join(errors))
    def export_selected(self):
        rows=self.selected()
        if not rows: self.info('Bitte die zu exportierenden Samples auswählen (Strg/Shift für Mehrfachauswahl).'); return
        dialog=ExportDialog(self)
        if dialog.exec():
            job=ExportJob(rows,self.store.libraries(),dialog.options(),self)
            job.result.connect(lambda result:self.info(f'{len(result[0])} Dateien kopiert. {len(result[1])} Dateikonflikte übersprungen.'+ ('\n\n'+'\n'.join(result[1][:10]) if result[1] else '')))
            self.start_job(job,'Export läuft …')
    def reclassify(self):
        from .training import ClassifyJob
        job=ClassifyJob(self.store.path,self); job.result.connect(lambda n:(self.refresh(),self.info(f'{n} Dateien klassifiziert. Manuelle Zuordnungen bleiben erhalten.')))
        self.start_job(job,'Lokalen Klassifikator anwenden')
    def info(self,text): QMessageBox.information(self,'Sample Intelligence Pro',str(text))
    def error(self,text):
        self.log.appendPlainText(str(text)); self.status.setText('Fehler: '+str(text)[:180]); QMessageBox.warning(self,'Sample Intelligence Pro',str(text))
    def help(self):
        self.info('1. „+ Bibliothek“: lokalen Sample-Ordner auswählen.\n2. Scan analysiert rekursiv; Abbrechen und später Scannen setzt fort.\n3. Sample auswählen, Leertaste zum Vorhören. Mausklick in die Wellenform springt; Mausrad zoomt.\n4. Kategorien manuell zuweisen; der lokale Klassifikator lernt aus diesen Beispielen.\n5. Find Similar durchsucht die Bibliothek mit KI-Embeddings und DSP-Merkmalen.\n6. Samples nach links auf Projekte ziehen oder als Dateien in Cubase ziehen.\n\nGrenzen dieser Version:\n• EfficientAT (AudioSet) ist enthalten. Es erkennt grobe Instrumente und Ereignisse; spezielle Synth-/Loop-Untertypen brauchen eigene Trainingsbeispiele. Scores sind nicht kalibriert.\n• Spektral-/Rhythmusanalyse auf die ersten 90 s begrenzt. LUFS über 90 s unbekannt; kurze Dateien <400 ms ebenfalls.\n• True Peak ist eine 4×-Schätzung, Dynamik ist P95–P10 der Kurzzeit-RMS.\n• Nahe Duplikate sind Kandidaten; keine vollständige Erkennung aller Varianten.\n• 100.000 Dateien und Cubase 15 müssen auf deinem Windows-PC praktisch geprüft werden.\n\nKein Netzwerkzugriff durch die Anwendung. Originaldateien bleiben bei Organisation unverändert.')
    def closeEvent(self,event):
        self.rescan_timer.stop()
        for job in self.jobs: job.cancel()
        # Cooperative shutdown; processEvents keeps the window responsive while workers finish.
        if any(job.isRunning() for job in self.jobs):
            event.ignore(); self.status.setText('Vorgänge werden beendet …'); QTimer.singleShot(250,self.close); return
        self.settings.setValue('geometry',self.saveGeometry()); self.settings.setValue('state',self.saveState()); self.settings.setValue('header',self.table.horizontalHeader().saveState())
        self.player.close(); self.store.close(); self.temp.cleanup(); event.accept()
