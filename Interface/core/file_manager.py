from core.extractors import extract_data_from_directory

def scan_results_directory(folder, prefix="Results_opt_"):
    return extract_data_from_directory(folder, prefix)