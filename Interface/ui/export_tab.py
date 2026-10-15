from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QPushButton, QTextEdit

class ExportTab(QWidget):
    def __init__(self):
        super().__init__()
        self._build_ui()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("Export center"))
        layout.addWidget(QLabel("Use this tab to export plots, summary tables, and analysis reports."))

        self.export_pdf_btn = QPushButton("Export current plot to PDF")
        self.export_csv_btn = QPushButton("Export current summary to CSV")
        self.export_report_btn = QPushButton("Export analysis report")
        self.log = QTextEdit()
        self.log.setReadOnly(True)
        self.log.setPlaceholderText("Export events will appear here...")

        layout.addWidget(self.export_pdf_btn)
        layout.addWidget(self.export_csv_btn)
        layout.addWidget(self.export_report_btn)
        layout.addWidget(self.log)