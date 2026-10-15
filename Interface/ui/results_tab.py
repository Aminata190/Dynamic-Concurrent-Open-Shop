from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLineEdit,
    QFileDialog, QLabel, QTableWidget, QTableWidgetItem, QGroupBox
)
from MDP.Interface.core.analyzers import scan_results_directory

class ResultsTab(QWidget):
    def __init__(self):
        super().__init__()
        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)

        controls = QGroupBox("Results explorer")
        top = QHBoxLayout()
        self.dir_input = QLineEdit()
        self.dir_input.setPlaceholderText("Select directory containing Results_opt_*.txt files")
        self.browse_btn = QPushButton("Browse...")
        self.load_btn = QPushButton("Load results")
        self.browse_btn.clicked.connect(self.select_directory)
        self.load_btn.clicked.connect(self.load_results)
        top.addWidget(self.dir_input)
        top.addWidget(self.browse_btn)
        top.addWidget(self.load_btn)
        controls.setLayout(top)

        self.table = QTableWidget(0, 4)
        self.table.setHorizontalHeaderLabels(["File", "Path", "Avg value", "Avg exec time"])

        root.addWidget(controls)
        root.addWidget(QLabel("Detected result files"))
        root.addWidget(self.table)

    def select_directory(self):
        folder = QFileDialog.getExistingDirectory(self, "Select results directory")
        if folder:
            self.dir_input.setText(folder)

    def load_results(self):
        folder = self.dir_input.text().strip()
        rows = scan_results_directory(folder)
        self.table.setRowCount(len(rows))
        for r, item in enumerate(rows):
            self.table.setItem(r, 0, QTableWidgetItem(item.get('file', '')))
            self.table.setItem(r, 1, QTableWidgetItem(item.get('path', '')))
            self.table.setItem(r, 2, QTableWidgetItem(str(item.get('avg_value', ''))))
            self.table.setItem(r, 3, QTableWidgetItem(str(item.get('avg_exec_time', ''))))