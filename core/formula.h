#pragma once
#include "sparse_matrix.h"
#include <string>
#include <utility>

// Evalua "=A1+2" o "=B3*A1". Dos operandos, operadores + - * /.
// Resuelve referencias a celdas recursivamente. Devuelve "#CICLO" o "#ERROR" ante fallos.
CellValue evaluate_formula(const std::string& formula, int row, int col, const SparseMatrix& matrix);

// "B3" -> {1, 2}
CellCoord parse_cell_id(const std::string& cell_id);

// 0 -> "A",  26 -> "AA"
std::string col_to_letter(int col);
