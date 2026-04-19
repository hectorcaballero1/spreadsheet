cmake_minimum_required(VERSION 3.17)
project(hoja_de_calculo)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ejecutable para testear sin UI
add_executable(main
    core/sparse_matrix.cpp
    core/formula.cpp
    core/main.cpp
)
target_include_directories(main PRIVATE core)

# Modulo Python
execute_process(
    COMMAND python3 -m pybind11 --cmakedir
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
find_package(pybind11 REQUIRED)
pybind11_add_module(spreadsheet
    core/sparse_matrix.cpp
    core/formula.cpp
    bindings/bindings.cpp
)
target_include_directories(spreadsheet PRIVATE core)
