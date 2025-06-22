import yaml
import sys

yaml_file = sys.argv[1]
key = sys.argv[2]
new_value = sys.argv[3]

with open(yaml_file, 'r') as f:
    data = yaml.safe_load(f)

# Update the field
data[key] = new_value

with open(yaml_file, 'w') as f:
    yaml.dump(data, f, sort_keys=False)
