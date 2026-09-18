# GridLAB-D Python Bindings

This package provides Python bindings for GridLAB-D, a power system simulation platform.

## Installation

1. **Build the GridLAB-D core** (if not already built):
   ```bash
   # From the repository root
   mkdir build
   cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   cmake --build .
   ```

2. **Install the Python package in development mode**:
   ```bash
   # From the repository root
   cd python_bindings
   pip install -e .
   ```

3. **Run tests to verify installation**:
   ```bash
   cd python_bindings/tests
   pytest -v
   ```

## Native Windows development (MSVC x64)

Use native Windows CMake and Visual Studio 2022 C++ Build Tools. Keep MSYS2 tools
out of the packaging build path. From PowerShell in the repository root:

```powershell
$cmake = "$env:ProgramFiles\CMake\bin\cmake.exe"
& $cmake -S . -B out/build/windows-native -G "Visual Studio 17 2022" -A x64
& $cmake --build out/build/windows-native --config Release --parallel 4
python -m venv .venv-windows-api
$python = "$PWD\.venv-windows-api\Scripts\python.exe"
& $python -m pip install scikit-build-core nanobind numpy pytest pytest-timeout
$env:PATH = "$env:ProgramFiles\CMake\bin;" + (($env:PATH -split ';' | Where-Object { $_ -notlike '*msys64*' }) -join ';')
$env:CMAKE_GENERATOR = "Visual Studio 17 2022"
$env:CMAKE_GENERATOR_PLATFORM = "x64"
$coreBuild = (Resolve-Path out/build/windows-native).Path
& $python -m pip install -e ./python_bindings --no-build-isolation "--config-settings=cmake.define.GRIDLABD_BUILD_DIR=$coreBuild"
& $python -m pytest python_bindings/tests --timeout=120
```

`GRIDLABD_BUILD_DIR` selects the exact core build used by the extension. On
Windows the extension links to `gldapi.lib`; the package bundles `gldapi.dll`,
module DLLs, and runtime data. Editable installs locate these native assets in
the environment's installed package directory while importing Python sources
from this checkout. Reinstall the editable package after rebuilding the core
so its bundled DLLs are refreshed.

## Building Wheels for Distribution

To create wheel (.whl) and source distribution (.tar.gz) files for PyPI:

1. **Run the preparation script** (copies built libraries into the package):
   ```bash
   cd python_bindings
   ./prepare_pypi_build.sh
   ```

2. **Build the distribution files**:
   ```bash
   python3 -m build
   ```

   This creates both files in the `dist/` directory.

**Note:** `pip install -e .` works for local development without running the preparation script because it accesses libraries directly from `../build/lib/`. However, `python -m build` creates an isolated environment and requires the prebuilt libraries to be bundled within the package directory.

## API Usage Examples

### Basic Usage

```python
import gridlabd

# Create a GridLAB-D instance
gld = gridlabd.GridLabD()S

# Load a model file
result = gld.load("path/to/model.glm")
assert result == 0, "Failed to load model"

# Initialize the model
result = gld.setup_after_load()
assert result == 0, "Failed to initialize"

# Run the simulation
result = gld.run()
assert result == 0, "Simulation failed"

print("Simulation completed successfully!")
```

### Querying Objects and Properties

```python
import gridlabd

# Load and initialize model
gld = gridlabd.GridLabD()
gld.load("test_HVAC_balance.glm")
gld.setup_after_load()

# Get all classes in the model
classes = gld.get_all_classes()
print(f"Classes in model: {classes}")

# Get all objects of a specific class
houses = gld.get_objects_by_class("house")
print(f"Found {len(houses)} houses")

# Get properties from a single object
if houses:
    props = gld.get_object_properties(houses[0])
    print(f"Floor area: {props.get('floor_area')}")

# Get all objects with all their properties
all_houses = gld.get_all_objects("house")
for house in all_houses:
    print(f"House {house['__name__']}: floor_area={house['floor_area']}")

# Get entire model as nested dictionary
model = gld.get_model()
for class_name, objects in model.items():
    print(f"{class_name}: {len(objects)} objects")
```

### Setting Properties

```python
import gridlabd

gld = gridlabd.GridLabD()
gld.load("model.glm")
gld.setup_after_load()

# Get objects
houses = gld.get_objects_by_class("house")

# Set a property value
if houses:
    result, value = gld.set_property(houses[0], "air_temperature", "72 degF")
    print(f"Set temperature result: {result}")
    
    # Verify the change
    result, new_value = gld.get_property(houses[0], "air_temperature")
    print(f"New temperature: {new_value}")
```

### Stepping Through Simulation

```python
import gridlabd

gld = gridlabd.GridLabD()
gld.load("model.glm")
gld.setup_after_load()

# Step through simulation timestep by timestep
for i in range(10):
    status, timestamp = gld.step()
    if status < 0:
        print("Simulation complete")
        break
    
    # Query state at each timestep
    houses = gld.get_all_objects("house")
    if houses:
        temp = houses[0].get('air_temperature')
        print(f"Timestep {i}, Time {timestamp}: Temperature = {temp}")
```

### Message Capture

GridLAB-D messages (warnings, errors, debug output) are automatically captured and can be retrieved programmatically. By default, C++ output is suppressed to keep your console clean.

```python
import gridlabd

# Default: C++ output suppressed, clean console
gld = gridlabd.GridLabD()

# Load and run model
gld.load("model.glm")
gld.setup_after_load()
gld.run()

# Get captured messages programmatically
messages = gld.get_messages()
for msg in messages:
    print(f"[{msg['type']}] {msg['timestamp']}: {msg['message']}")

# Filter for errors only
errors = [m for m in messages if m['type'] == 'ERROR']
print(f"Found {len(errors)} errors")
```

**Verbose mode** - Enable C++ console output for debugging:

```python
# Show C++ output on stderr (useful for debugging)
gld = gridlabd.GridLabD(verbose=True)
gld.load("model.glm")
gld.run()

# Messages are still captured even in verbose mode
messages = gld.get_messages()
```

**Message capture controls**:

```python
# Disable message capture (not recommended)
gld.enable_message_capture(False)

# Clear captured messages
gld.clear_messages()

# Set message limit (default: 10000)
gld.set_message_capture_limit(5000)
limit = gld.get_message_capture_limit()
```
   