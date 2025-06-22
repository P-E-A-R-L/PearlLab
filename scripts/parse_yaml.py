import yaml
import sys

def flatten(data, prefix=""):
    result = {}
    for key, value in data.items():
        full_key = f"{prefix}.{key}" if prefix else key
        if isinstance(value, dict):
            result.update(flatten(value, full_key))
        else:
            result[full_key] = value
    return result

with open(sys.argv[1], "r") as f:
    config = yaml.safe_load(f)

flat = flatten(config)
for k, v in flat.items():
    print(f"{k}={v}")
