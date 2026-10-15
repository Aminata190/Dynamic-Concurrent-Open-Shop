from PySide6.QtWidgets import QMainWindow, QTabWidget, QStatusBar
from ui.simulation_tab import SimulationTab
from ui.results_tab import ResultsTab
from ui.analysis_tab import AnalysisTab
from ui.export_tab import ExportTab

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Simulation Manager")
        self.resize(1400, 900)

        self.tabs = QTabWidget()
        self.simulation_tab = SimulationTab()
        self.results_tab = ResultsTab()
        self.analysis_tab = AnalysisTab()
        self.export_tab = ExportTab()

        self.tabs.addTab(self.simulation_tab, "Simulations")
        self.tabs.addTab(self.results_tab, "Results")
        self.tabs.addTab(self.analysis_tab, "Analysis")
        self.tabs.addTab(self.export_tab, "Export")

        self.setCentralWidget(self.tabs)
        self.setStatusBar(QStatusBar())
        self.statusBar().showMessage("Ready")