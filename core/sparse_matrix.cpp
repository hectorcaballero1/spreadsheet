#include "sparse_matrix.h"

SparseMatrix::~SparseMatrix() { clear(); }

// Si la celda existe, retorna el puntero a la celda, otherwise retorna nullptr
SparseMatrix::Cell* SparseMatrix::find_cell(int row, int col) const {
    RowHeader* curr_row = row_head;
    while (curr_row != nullptr and curr_row->row != row) curr_row = curr_row->next;

    if (curr_row == nullptr) return nullptr;
    
    Cell* curr_cell = curr_row->first;
    while (curr_cell != nullptr and curr_cell->col != col) curr_cell = curr_cell->right;

    if (curr_cell == nullptr) return nullptr;

    return curr_cell; 
}

// Retorna puntero al RowHeader si existe, otherwise crea el RowHeader y lo retorna
SparseMatrix::RowHeader* SparseMatrix::find_or_create_row(int row) {
    return nullptr;
}

// Retorna puntero al ColHeader, otherwise crea el ColHeader y lo retorna
SparseMatrix::ColHeader* SparseMatrix::find_or_create_col(int col) {
    return nullptr;
}

// Inserta cell en la lista de rh->first manteniendo orden por col
void SparseMatrix::insert_cell_in_row(RowHeader* rh, Cell* cell) {
}

// Inserta cell en la lista de ch->first manteniendo orden por row
void SparseMatrix::insert_cell_in_col(ColHeader* ch, Cell* cell) {
}

// Quita cell de la lista enlazada de la fila (actualiza left/right)
void SparseMatrix::remove_cell_from_row(RowHeader* rh, Cell* cell) {
}

// Quita cell de la lista enlazada de la columna (actualiza up/down)
void SparseMatrix::remove_cell_from_col(ColHeader* ch, Cell* cell) {
}

// Encuentra la cell en (row, col), la quita de fila y columna, y hace delete
void SparseMatrix::delete_cell_node(int row, int col) {
}

// Empuja action a undo_stack (si supera MAX_HISTORY, descarta el más antiguo) y limpia redo_stack
void SparseMatrix::push_action(Action action) {
}

// Libera todas las celdas, cabeceras de fila y columna, y limpia undo/redo stacks
void SparseMatrix::clear() {
}

// Guarda CellChange{before, after} y crea o actualiza la celda en (row, col)
void SparseMatrix::set(int row, int col, CellValue value) {
}

// Retorna el valor de la celda en (row, col), o nullopt si no existe
std::optional<CellValue> SparseMatrix::get(int row, int col) const {
    return std::nullopt;
}

// Guarda CellChange{before, nullopt} y elimina la celda en (row, col)
void SparseMatrix::clear_cell(int row, int col) {
}

// Guarda un Action con todos los CellChange de la fila y elimina cada celda y el RowHeader
void SparseMatrix::delete_row(int row) {
}

// Ídem para la columna col
void SparseMatrix::delete_col(int col) {
}

// Elimina todas las celdas dentro del rango rectangular (r1,c1)-(r2,c2)
void SparseMatrix::delete_range(int r1, int c1, int r2, int c2) {
}

// Recorre la columna col y retorna el row del último nodo, o nullopt si la columna no existe
std::optional<int> SparseMatrix::last_row_in_col(int col) const {
    return std::nullopt;
}

// Revierte la última acción: aplica before en cada CellChange y mueve la acción a redo_stack
void SparseMatrix::undo() {
}

// Reaaplica la última acción deshecha: aplica after en cada CellChange y mueve la acción a undo_stack
void SparseMatrix::redo() {
}

bool SparseMatrix::can_undo() const { return !undo_stack.empty(); }
bool SparseMatrix::can_redo() const { return !redo_stack.empty(); }

// Recorre todas las celdas activas y llama f(row, col, val) por cada una
void SparseMatrix::for_each(std::function<void(int, int, const CellValue&)> f) const {
}

// Recorre row_head y retorna el mayor índice de fila activa
int SparseMatrix::max_row() const {
    return 0;
}

// Recorre col_head y retorna el mayor índice de columna activa
int SparseMatrix::max_col() const {
    return 0;
}
