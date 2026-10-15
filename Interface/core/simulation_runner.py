class SimulationRunner:
    def __init__(self):
        self.is_running = False

    def run_single(self, config):
        self.is_running = True
        return {"status": "done", "config": config}

    def run_batch(self, configs):
        self.is_running = True
        return [{"status": "done", "config": cfg} for cfg in configs]

    def stop(self):
        self.is_running = False