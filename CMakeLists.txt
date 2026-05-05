cmake_minimum_required(VERSION 3.17)
project(hoja_de_calculo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ejecutable para testear sin UI
add_executable(main
    core/sparse_matrix.cpp
    core/formula.cpp
    core/main.cpp
)
target_include_directories(main PRIVATE core)

# Python + pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

if(NOT DEFINED pybind11_DIR OR pybind11_DIR STREQUAL "")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_DIR
        RESULT_VARIABLE pybind11_RESULT
        ERROR_VARIABLE pybind11_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT pybind11_RESULT EQUAL 0)
        message(FATAL_ERROR "No se pudo encontrar pybind11: ${pybind11_ERROR}")
    endif()
endif()

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(spreadsheet
    core/sparse_matrix.cpp
    core/formula.cpp
    bindings/bindings.cpp
)
target_include_directories(spreadsheet PRIVATE core)

# Dejar el módulo Python compilado en la raíz del proyecto,
# para que main.py pueda hacer import spreadsheet directamente.
set_target_properties(spreadsheet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

foreach(CONFIG_TYPE Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG_TYPE}" CONFIG_TYPE_UPPER)
    set_target_properties(spreadsheet PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_SOURCE_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_SOURCE_DIR}"
    )
endforeach()
