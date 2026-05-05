import subprocess
import sys
import platform
import shutil
from pathlib import Path

import pybind11


def run(cmd):
    print("+", " ".join(str(x) for x in cmd))
    subprocess.run(cmd, check=True)


python_exe = sys.executable
pybind11_dir = pybind11.get_cmake_dir()

cmd = [
    "cmake",
    "-S", ".",
    "-B", "build",
    f"-DPython_EXECUTABLE={python_exe}",
    f"-DPython3_EXECUTABLE={python_exe}",
    f"-Dpybind11_DIR={pybind11_dir}",
]

gxx = None

if platform.system() == "Windows":
    gxx = shutil.which("g++")
    if not gxx:
        print("Error: no se encontró g++ en PATH.")
        print("Instala MSYS2/MinGW o asegúrate de que 'g++ --version' funcione.")
        sys.exit(1)

    cmd += [
        "-G", "Ninja",
        f"-DCMAKE_CXX_COMPILER={gxx}",
    ]

run(cmd)
run(["cmake", "--build", "build"])

# En Windows con MinGW, el .pyd necesita estas DLLs al lado para poder importarse.
if platform.system() == "Windows":
    compiler_dir = Path(gxx).parent
    build_dir = Path("build")

    runtime_dlls = [
        "libstdc++-6.dll",
        "libgcc_s_seh-1.dll",
        "libgcc_s_sjlj-1.dll",
        "libgcc_s_dw2-1.dll",
        "libwinpthread-1.dll",
    ]

    for dll in runtime_dlls:
        src = compiler_dir / dll
        if src.exists():
            dst = build_dir / dll
            shutil.copy2(src, dst)
            print(f"DLL copiada: {src} -> {dst}")

print("Build exitoso.")
