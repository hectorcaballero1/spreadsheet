import subprocess
import sys

try:
    import pybind11
except ImportError:
    print("Error: pybind11 no está instalado en el entorno.")
    print("Prueba ejecutar: uv add pybind11")
    sys.exit(1)


def run(cmd):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


python_exe = sys.executable
pybind11_dir = pybind11.get_cmake_dir()

run([
    "cmake",
    "-S", ".",
    "-B", "build",
    f"-DPython_EXECUTABLE={python_exe}",
    f"-DPython3_EXECUTABLE={python_exe}",
    f"-Dpybind11_DIR={pybind11_dir}",
])

run([
    "cmake",
    "--build", "build",
])

print("Build exitoso.")
