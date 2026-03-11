#!/usr/bin/env python3
"""Generate a synthetic residential benchmark JSON with N independent houses.

Each house has its own triplex_meter parent (no shared parent state between houses),
so all house objects at the same rank are independently parallelisable.

Usage: python3 gen_houses.py [N] > bench_N_houses.json
"""

import sys
import random
import json

N = int(sys.argv[1]) if len(sys.argv) > 1 else 500
random.seed(42)

meters = []
houses = []

for i in range(N):
    floor_area  = round(random.uniform(800,  3000), 1)
    ceiling_ht  = round(random.uniform(8,    12),   1)
    Rroof       = round(random.uniform(20,   50),   1)
    Rwall       = round(random.uniform(10,   30),   1)
    Rfloor      = round(random.uniform(10,   25),   1)
    Rwindows    = round(random.uniform(2,    6),    2)
    air_changes = round(random.uniform(0.3,  1.5),  2)
    heating_set = round(random.uniform(68,   72),   1)
    cooling_set = round(random.uniform(74,   78),   1)
    init_temp   = round(random.uniform(68,   76),   1)

    meters.append({
        "name": f"meter_{i}",
        "phases": "AS",
        "nominal_voltage": 120
    })

    houses.append({
        "name": f"house_{i}",
        "parent": f"meter_{i}",
        "floor_area": f"{floor_area} sf",
        "ceiling_height": f"{ceiling_ht} ft",
        "Rroof": Rroof,
        "Rwall": Rwall,
        "Rfloor": Rfloor,
        "Rwindows": Rwindows,
        "airchange_per_hour": air_changes,
        "heating_setpoint": f"{heating_set} degF",
        "cooling_setpoint": f"{cooling_set} degF",
        "air_temperature": f"{init_temp} degF",
        "mass_temperature": f"{init_temp} degF",
        "heating_system_type": "RESISTANCE",
        "cooling_system_type": "ELECTRIC",
        "weather": "climate1"
    })

model = {
    "_directives": {
        "#set": {
            "minimum_timestep": 60
        }
    },
    "clock": {
        "timezone": "PST+8PDT",
        "starttime": "'2000-01-01 00:00:00'",
        "stoptime":  "'2000-01-08 00:00:00'"
    },
    "modules": {
        "residential": {"implicit_enduses": "NONE"},
        "climate": {},
        "powerflow": {},
        "tape": {}
    },
    "objects": {
        "climate": {
            "instances": [{
                "name": "climate1",
                "tmyfile": "WA-Yakima.tmy2",
                "interpolate": "QUADRATIC"
            }]
        },
        "triplex_meter": {"instances": meters},
        "house":         {"instances": houses},
        "recorder": {
            "instances": [{
                "parent": "house_0",
                "file": "house_out.csv",
                "interval": 60,
                "limit": 24,
                "property": "outdoor_temperature,outdoor_rh"
            }]
        }
    }
}

print(json.dumps(model, indent=2))
