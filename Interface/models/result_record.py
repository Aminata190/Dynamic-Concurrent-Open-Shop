from dataclasses import dataclass, field

@dataclass
class ResultRecord:
    file: str
    path: str
    number: int
    avg_value: float
    avg_exec_time: float
    values: list = field(default_factory=list)
    exec_times: list = field(default_factory=list)