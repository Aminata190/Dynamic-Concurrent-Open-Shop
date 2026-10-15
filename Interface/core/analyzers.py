from core.extractors import extract_data_from_directory
from ui.simulation_tab import SimulationTab
import numpy as np

def scan_results_directory(folder, prefix="Results_opt_"):
    return extract_data_from_directory(folder, prtion_tab = SimulationTab()
        self.results_tab = ResultsTab()
        self.analysis_tab = AnalysisTab()
        self.export_tab = ExportTab()

        self.tabs.addTab(self.simulation_tab, "Simulations")
        self.tabs.addTab(self.results_tab, "Results")
        )

def compare_directories(dir_vi, dir_order, prefix="Results_opt_"):
    vi_records = extract_data_from_directory(dir_vi, prefix)
    order_records = extract_data_from_directory(dir_order, prefix)

    vi = _flatten(vi_records, 'values')
    order = _flatten(order_records, 'values')

    n = min(len(vi), len(order))
    vi = vi[:n]
    order = order[:n]
    diffs = vi - order if n else np.array([])

    return {
        'vi': vi,
        'order': order,
        'diffs': diffs,
        'vi_records': vi_records,
        'order_records': order_records,
    }