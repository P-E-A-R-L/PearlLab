{
    "links": [
        {
            "dst": 8,
            "src": 4
        },
        {
            "dst": 7,
            "src": 2
        },
        {
            "dst": 14,
            "src": 6
        },
        {
            "dst": 12,
            "src": 4
        },
        {
            "dst": 11,
            "src": 2
        },
        {
            "dst": 16,
            "src": 10
        },
        {
            "dst": 26,
            "src": 41
        },
        {
            "dst": 27,
            "src": 41
        },
        {
            "dst": 28,
            "src": 45
        },
        {
            "dst": 38,
            "src": 57
        },
        {
            "dst": 53,
            "src": 24
        },
        {
            "dst": 55,
            "src": 52
        },
        {
            "dst": 70,
            "src": 67
        },
        {
            "dst": 76,
            "src": 69
        },
        {
            "dst": 77,
            "src": 2
        },
        {
            "dst": 84,
            "src": 81
        },
        {
            "dst": 85,
            "src": 69
        },
        {
            "dst": 86,
            "src": 2
        },
        {
            "dst": 88,
            "src": 83
        },
        {
            "dst": 25,
            "src": 43
        },
        {
            "dst": 79,
            "src": 74
        },
        {
            "dst": 75,
            "src": 72
        },
        {
            "dst": 32,
            "src": 123
        },
        {
            "dst": 133858913696640,
            "src": 127
        },
        {
            "dst": 134,
            "src": 130
        },
        {
            "dst": 133,
            "src": 67
        },
        {
            "dst": 143,
            "src": 138
        },
        {
            "dst": 140,
            "src": 132
        },
        {
            "dst": 139,
            "src": 147
        },
        {
            "dst": 141,
            "src": 2
        }
    ],
    "next_id": 150,
    "nodes": [
        {
            "data": {
                "id": 1,
                "inputs": [],
                "module": "test_pearl.cudaDevice",
                "outputs": [
                    2
                ],
                "pointer": false,
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonFunctionNode"
        },
        {
            "data": {
                "id": 3,
                "inputs": [],
                "module": "pearl.provided.AssaultEnv.AssaultEnvShapMask",
                "outputs": [
                    4
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 5,
                "inputs": [
                    7,
                    8
                ],
                "module": "pearl.methods.ShapExplainability.ShapExplainability",
                "outputs": [
                    6
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 9,
                "inputs": [
                    11,
                    12
                ],
                "module": "pearl.methods.LimeExplainability.LimeExplainability",
                "outputs": [
                    10
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 13,
                "inputs": [
                    14
                ],
                "outputs": [],
                "tag": "ShapExplainability"
            },
            "type": "PipelineGraph.Nodes.MethodAcceptorNode"
        },
        {
            "data": {
                "id": 15,
                "inputs": [
                    16
                ],
                "outputs": [],
                "tag": "LimeExplainability"
            },
            "type": "PipelineGraph.Nodes.MethodAcceptorNode"
        },
        {
            "data": {
                "id": 23,
                "inputs": [
                    25,
                    26,
                    27,
                    28,
                    29,
                    30,
                    31,
                    32,
                    33,
                    34,
                    35,
                    36,
                    37,
                    38,
                    39
                ],
                "module": "pearl.enviroments.GymRLEnv.GymRLEnv",
                "outputs": [
                    24
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 40,
                "inputs": [],
                "outputs": [
                    41
                ],
                "rangeEnd": -1,
                "rangeStart": -1,
                "tag": "<tag>",
                "value": 4
            },
            "type": "PipelineGraph.Nodes.PrimitiveIntNode"
        },
        {
            "data": {
                "file": false,
                "id": 42,
                "inputs": [],
                "outputs": [
                    43
                ],
                "tag": "<tag>",
                "value": "ALE/Assault-v5"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        },
        {
            "data": {
                "file": false,
                "id": 44,
                "inputs": [],
                "outputs": [
                    45
                ],
                "tag": "<tag>",
                "value": "rgb_array"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        },
        {
            "data": {
                "id": 51,
                "inputs": [
                    53
                ],
                "module": "test_pearl.Wrapper",
                "outputs": [
                    52
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 54,
                "inputs": [
                    55
                ],
                "outputs": [],
                "tag": "GymRLEnv (Assault)"
            },
            "type": "PipelineGraph.Nodes.EnvAcceptorNode"
        },
        {
            "data": {
                "id": 56,
                "inputs": [],
                "outputs": [
                    57
                ],
                "rangeEnd": -1,
                "rangeStart": -1,
                "tag": "<tag>",
                "value": 1
            },
            "type": "PipelineGraph.Nodes.PrimitiveIntNode"
        },
        {
            "data": {
                "id": 66,
                "inputs": [],
                "outputs": [
                    67
                ],
                "rangeEnd": -1,
                "rangeStart": -1,
                "tag": "<tag>",
                "value": 7
            },
            "type": "PipelineGraph.Nodes.PrimitiveIntNode"
        },
        {
            "data": {
                "id": 68,
                "inputs": [
                    70,
                    133858913696640
                ],
                "module": "test_pearl.DQN",
                "outputs": [
                    69
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "file": false,
                "id": 71,
                "inputs": [],
                "outputs": [
                    72
                ],
                "tag": "<tag>",
                "value": "./py/models/models/dqn_assault_5m.pth"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        },
        {
            "data": {
                "id": 73,
                "inputs": [
                    75,
                    76,
                    77
                ],
                "module": "pearl.agents.TourchDQN.TorchDQN",
                "outputs": [
                    74
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 78,
                "inputs": [
                    79
                ],
                "outputs": [],
                "tag": "TorchDQN (5m)"
            },
            "type": "PipelineGraph.Nodes.AgentAcceptorNode"
        },
        {
            "data": {
                "file": false,
                "id": 80,
                "inputs": [],
                "outputs": [
                    81
                ],
                "tag": "<tag>",
                "value": "./py/models/models/dqn_assault_1k.pth"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        },
        {
            "data": {
                "id": 82,
                "inputs": [
                    84,
                    85,
                    86
                ],
                "module": "pearl.agents.TourchDQN.TorchDQN",
                "outputs": [
                    83
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 87,
                "inputs": [
                    88
                ],
                "outputs": [],
                "tag": "TorchDQN (1k)"
            },
            "type": "PipelineGraph.Nodes.AgentAcceptorNode"
        },
        {
            "data": {
                "id": 122,
                "inputs": [
                    73
                ],
                "module": "test_pearl.preprocess",
                "outputs": [
                    123
                ],
                "pointer": true,
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonFunctionNode"
        },
        {
            "data": {
                "file": false,
                "id": 126,
                "inputs": [],
                "outputs": [
                    127
                ],
                "tag": "<tag>",
                "value": "net"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        },
        {
            "data": {
                "file": false,
                "id": 129,
                "inputs": [],
                "outputs": [
                    130
                ],
                "tag": "<tag>",
                "value": "network"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        },
        {
            "data": {
                "id": 131,
                "inputs": [
                    133,
                    134
                ],
                "module": "test_pearl.DQN",
                "outputs": [
                    132
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 137,
                "inputs": [
                    139,
                    140,
                    141
                ],
                "module": "pearl.agents.TourchDQN.TorchDQN",
                "outputs": [
                    138
                ],
                "tag": "<tag>"
            },
            "type": "PipelineGraph.Nodes.PythonModuleNode"
        },
        {
            "data": {
                "id": 142,
                "inputs": [
                    143
                ],
                "outputs": [],
                "tag": "Agent47"
            },
            "type": "PipelineGraph.Nodes.AgentAcceptorNode"
        },
        {
            "data": {
                "file": false,
                "id": 146,
                "inputs": [],
                "outputs": [
                    147
                ],
                "tag": "<tag>",
                "value": "./py/models/models/AtariDQN.cleanrl_model"
            },
            "type": "PipelineGraph.Nodes.PrimitiveStringNode"
        }
    ]
}