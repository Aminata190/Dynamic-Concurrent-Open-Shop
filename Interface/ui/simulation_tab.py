from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QFormLayout, QLabel, QPushButton,
    QComboBox, QSpinBox, QLineEdit, QFileDialog, QTextEdit, QProgressBar,
    QGroupBox, QListWidget, QListWidgetItem
)
from core.simulation_runner import SimulationRunner

class SimulationTab(QWidget):
    def __init__(self):
        super().__init__()
        self.runner = SimulationRunner()
        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)

        config_group = QGroupBox("Simulation configuration")
        form = QFormLayout()

        self.algorithm_combo = QComboBox()
        self.algorithm_combo.addItems(["Value Iteration", "Order-based", "Custom"])

        self.config_name = QLineEdit("N2_m2")
        self.instances_spin = QSpinBox()
        self.instances_spin.setRange(1, 100000)
        self.instances_spin.setValue(1000)

        self.max_values_spin = QSpinBox()
        self.max_values_spin.setRange(1, 1000)
        self.max_values_spin.setValue(5)

        self.output_dir = QLineEdit()
        self.output_btn = QPushButton("Browse...")
        self.output_btn.clicked.connect(self.select_output_dir)
        output_row = QHBoxLayout()
        output_row.addWidget(self.output_dir)
        output_row.addWidget(self.output_btn)

        form.addRow("Algorithm", self.algorithm_combo)
        form.addRow("Config name", self.config_name)
        form.addRow("Instances", self.instances_spin)
        form.addRow("Max values/task", self.max_values_spin)
        form.addRow("Output directory", output_row)
        config_group.setLayout(form)

        params_group = QGroupBox("Extra parameters")
        params_layout = QVBoxLayout()
        self.params_list = QListWidget()
        self.params_list.addItem(QListWidgetItem("N=2"))
        self.params_list.addItem(QListWidgetItem("M=2"))
        params_layout.addWidget(self.params_list)
        params_group.setLayout(params_layout)

        actions_row = QHBoxLayout()
        self.run_btn = QPushButton("Run simulation")
        self.batch_btn = QPushButton("Run batch")
        self.stop_btn = QPushButton("Stop")
        actions_row.addWidget(self.run_btn)
        actions_row.addWidget(self.batch_btn)
        actions_row.addWidget(self.stop_btn)

        self.progress = QProgressBar()
        self.progress.setRange(0, 100)
        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)
        self.log_output.setPlaceholderText("Simulation logs will appear here...")

        root.addWidget(config_group)
        root.addWidget(params_group)
        root.addLayout(actions_row)
        root.addWidget(QLabel("Progress"))
        root.addWidget(self.progress)
        root.addWidget(QLabel("Logs"))
        root.addWidget(self.log_output)

    def select_output_dir(self):
        folder = QFileDialog.getExistingDirectory(self, "Select output directory")
        if folder:
            self.output_dir.setText(folder)