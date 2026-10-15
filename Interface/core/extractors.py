from pathlib import Path
import re

def extract_avg_data_from_file(filename):
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    value = 0.0
    exec_time = 0.0
    num = 0

    for line in lines[1:]:
        data = line.strip().split()
        if len(data) >= 3:
            try:
                value += float(data[1])
                exec_time += float(data[2])
                num += 1
            except ValueError:
                continue

    if num != 0:
        return {'number': num, 'avg_value': value / num, 'avg_exec_time': exec_time / num}
    return None

def extract_data_from_file(filename):
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    values = []
    exec_times = []
    num = 0

    for line in lines[1:]:
        data = line.strip().split()
        if len(data) >= 3:
            try:
                values.append(float(data[1]))
                exec_times.append(float(data[2]))
                num += 1
            except ValueError:
                continue

    if num != 0:
        return {'number': num, 'values': values, 'exec_times': exec_times}
    return None

def natural_key(path_obj: Path):
    nums = re.findall(r'(\d+)', path_obj.stem)
    return tuple(int(x) for x in nums) if nums else (10**9,)

def extract_data_from_directory(dirname, prefix="Results_opt_"):
    folder = Path(dirname)
    if not folder.exists() or not folder.is_dir():
        return []

    files = [f for f in folder.rglob('*.txt') if f.is_file() and f.name.startswith(prefix)]
    records = []
    for file in sorted(files, key=natural_key):
        avg = extract_avg_data_from_file(str(file))
        full = extract_data_from_file(str(file))
        if avg and full:
            records.append({
                'file': file.name,
                'path': str(file),
                'number': avg['number'],
                'avg_value': avg['avg_value'],
                'avg_exec_time': avg['avg_exec_time'],
                'values': full['values'],
                'exec_times': full['exec_times'],
            })
    return records