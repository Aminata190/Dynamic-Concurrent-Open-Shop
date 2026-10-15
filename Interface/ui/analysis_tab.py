from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLineEdit, QFileDialog,
    QLabel, QComboBox, QFormLayout, QGroupBox
)
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from core.analyzers import compare_directories
from core.plotting import draw_plot

class AnalysisTab(QWidget):
    def __init__(self):
        super().__init__()
        self.figure = Figure(figsize=(7, 6))
        self.canvas = FigureCanvas(self.figure)
        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)

        controls = QGroupBox("Analysis controls")
        form = QFormLayout()

        self.dir_vi = QLineEdit()
        self.dir_order = QLineEdit()
        self.title_input = QLineEdit("N2_m2")
        self.plot_type = QComboBox()
        self.plot_type.addItems(["Scatter", "Line", "Box", "Bar"])

        vi_row = QHBoxLayout()
        vi_btn = QPushButton("Browse...")
        vi_btn.clicked.connect(lambda: self.select_dir(self.dir_vi))
        vi_row.addWidget(self.dir_vi)
        vi_row.addWidget(vi_btn)

        ord_row = QHBoxLayout()
        ord_btn = QPushButton("Browse...")
        ord_btn.clicked.connect(lambda: self.select_dir(self.dir_order))
        ord_row.addWidget(self.dir_order)
        ord_row.addWidget(ord_btn)

        form.addRow("Directory VI", vi_row)
        form.addRow("Directory Order", ord_row)
        form.addRow("Plot title", self.title_input)
        form.addRow("Plot type", self.plot_type)
        controls.setLayout(form)

        self.run_btn = QPushButton("Generate analysis")
        self.run_btn.clicked.connect(self.generate_analysis)

        root.addWidget(controls)
        root.addWidget(self.run_btn)
        root.addWidget(QLabel("Visualization"))
        root.addWidget(self.canvas)

    def select_dir(self, target):
        folder = QFileDialog.getExistingDirectory(self, "Select directory")
        if folder:
            target.setText(folder)

    def generate_analysis(self):
        self.figure.clear()
        ax = self.figure.add_subplot(111)
        result = compare_directories(self.dir_vi.text().strip(), self.dir_order.text().strip())
        draw_plot(ax, result, self.plot_type.currentText(), self.title_input.text().strip())
        self.canvas.draw()