#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "sparse_matrix.h"
#include "formula.h"

namespace py = pybind11;

PYBIND11_MODULE(spreadsheet, m) {
    m.doc() = "Core de hoja de cálculo: matriz dispersa + evaluador de fórmulas";

    py::class_<SparseMatrix>(m, "SparseMatrix")
        .def(py::init<>())
        // set acepta int, float o str desde Python
        .def("set_int",   [](SparseMatrix& s, int r, int c, int v) { s.set(r, c, v); })
        .def("set_float", [](SparseMatrix& s, int r, int c, double v) { s.set(r, c, v); })
        .def("set_str",   [](SparseMatrix& s, int r, int c, const std::string& v) { s.set(r, c, v); })
        // get devuelve int/float/str/None
        .def("get", [](const SparseMatrix& s, int r, int c) -> py::object {
            auto val = s.get(r, c);
            if (!val) return py::none();
            return std::visit([](auto&& v) -> py::object { return py::cast(v); }, *val);
        })
        .def("clear_cell", &SparseMatrix::clear_cell)
        .def("delete_row", &SparseMatrix::delete_row)
        .def("delete_col", &SparseMatrix::delete_col)
        .def("undo", &SparseMatrix::undo)
        .def("redo", &SparseMatrix::redo)
        .def("can_undo", &SparseMatrix::can_undo)
        .def("can_redo", &SparseMatrix::can_redo)
        .def("max_row", &SparseMatrix::max_row)
        .def("max_col", &SparseMatrix::max_col)
        .def("delete_range", &SparseMatrix::delete_range)
        .def("last_row_in_col", [](const SparseMatrix& s, int col) -> py::object {
            auto val = s.last_row_in_col(col);
            return val ? py::cast(*val) : py::none();
        })
        // for_each(callback(row, col, value))
        .def("for_each", [](const SparseMatrix& s, py::function f) {
            s.for_each([&](int r, int c, const CellValue& v) {
                f(r, c, std::visit([](auto&& x) -> py::object { return py::cast(x); }, v));
            });
        });

    m.def("evaluate_formula",
        [](const std::string& formula, int row, int col, const SparseMatrix& matrix) -> py::object {
            auto val = evaluate_formula(formula, row, col, matrix);
            return std::visit([](auto&& v) -> py::object { return py::cast(v); }, val);
        });

    m.def("parse_cell_id", &parse_cell_id);
    m.def("col_to_letter",  &col_to_letter);
}
