from dataclasses import dataclass, field

@dataclass
class RunConfig:
    algorithm: str
    config_name: str
    instances: int
    max_values_per_task: int
    output_dir: str
    extra_params: dict = field(default_factory=dict)