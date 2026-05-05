#include "sparse_matrix.h"

SparseMatrix::~SparseMatrix() { clear(); }

// Si la celda existe, retorna el puntero a la celda, otherwise retorna nullptr
SparseMatrix::Cell* SparseMatrix::find_cell(int row, int col) const {
    RowHeader* curr_row = row_head;
    while (curr_row != nullptr && curr_row->row != row) curr_row = curr_row->next;

    if (curr_row == nullptr) return nullptr;

    Cell* curr_cell = curr_row->first;
    while (curr_cell != nullptr && curr_cell->col != col) curr_cell = curr_cell->right;

    if (curr_cell == nullptr) return nullptr;

    return curr_cell;
}

// Retorna puntero al RowHeader si existe, otherwise crea el RowHeader y lo retorna
SparseMatrix::RowHeader* SparseMatrix::find_or_create_row(int row) {
    RowHeader* curr_row = row_head;
    RowHeader* prev_row = nullptr;

    while (curr_row != nullptr && curr_row->row < row) {
        prev_row = curr_row;
        curr_row = curr_row->next;
    }

    if (curr_row != nullptr && curr_row->row == row) return curr_row;

    RowHeader* new_row = new RowHeader(row);
    new_row->next = curr_row;
    new_row->prev = prev_row;
    if (curr_row != nullptr) curr_row->prev = new_row;
    if (prev_row != nullptr) prev_row->next = new_row;
    else row_head = new_row;

    return new_row;
}

// Retorna puntero al ColHeader si existe, otherwise crea el ColHeader y lo retorna
SparseMatrix::ColHeader* SparseMatrix::find_or_create_col(int col) {
    ColHeader* curr_col = col_head;
    ColHeader* prev_col = nullptr;

    while (curr_col != nullptr && curr_col->col < col) {
        prev_col = curr_col;
        curr_col = curr_col->next;
    }

    if (curr_col != nullptr && curr_col->col == col) return curr_col;

    ColHeader* new_col = new ColHeader(col);
    new_col->next = curr_col;
    new_col->prev = prev_col;
    if (curr_col != nullptr) curr_col->prev = new_col;
    if (prev_col != nullptr) prev_col->next = new_col;
    else col_head = new_col;

    return new_col;
}

// Inserta cell en la lista de rh->first manteniendo orden por col
void SparseMatrix::insert_cell_in_row(RowHeader* rh, Cell* cell) {
    Cell* curr_cell = rh->first;
    Cell* prev_cell = nullptr;

    while (curr_cell != nullptr && curr_cell->col < cell->col) {
        prev_cell = curr_cell;
        curr_cell = curr_cell->right;
    }

    cell->right = curr_cell;
    cell->left = prev_cell;
    if (curr_cell != nullptr) curr_cell->left = cell;
    if (prev_cell != nullptr) prev_cell->right = cell;
    else rh->first = cell;
}

// Inserta cell en la lista de ch->first manteniendo orden por row
void SparseMatrix::insert_cell_in_col(ColHeader* ch, Cell* cell) {
    Cell* curr_cell = ch->first;
    Cell* prev_cell = nullptr;

    while (curr_cell != nullptr && curr_cell->row < cell->row) {
        prev_cell = curr_cell;
        curr_cell = curr_cell->down;
    }

    cell->down = curr_cell;
    cell->up = prev_cell;
    if (curr_cell != nullptr) curr_cell->up = cell;
    if (prev_cell != nullptr) prev_cell->down = cell;
    else ch->first = cell;
}

// Quita cell de la lista enlazada de la fila (actualiza left/right)
void SparseMatrix::remove_cell_from_row(RowHeader* rh, Cell* cell) {
    if (cell->left != nullptr) cell->left->right = cell->right;
    else rh->first = cell->right;
    if (cell->right != nullptr) cell->right->left = cell->left;
}

// Quita cell de la lista enlazada de la columna (actualiza up/down)
void SparseMatrix::remove_cell_from_col(ColHeader* ch, Cell* cell) {
    if (cell->up != nullptr) cell->up->down = cell->down;
    else ch->first = cell->down;
    if (cell->down != nullptr) cell->down->up = cell->up;
}

// Encuentra la cell en (row, col), la quita de fila y columna, y hace delete
void SparseMatrix::delete_cell_node(int row, int col) {
    Cell* cell = find_cell(row, col);
    if (cell == nullptr) return;

    RowHeader* curr_row = row_head;
    while (curr_row != nullptr && curr_row->row != row) curr_row = curr_row->next;

    ColHeader* curr_col = col_head;
    while (curr_col != nullptr && curr_col->col != col) curr_col = curr_col->next;

    remove_cell_from_row(curr_row, cell);
    remove_cell_from_col(curr_col, cell);
    delete cell;
}

// Empuja action a undo_stack (si supera MAX_HISTORY, descarta el más antiguo) y limpia redo_stack
void SparseMatrix::push_action(Action action) {
    if ((int)undo_stack.size() >= MAX_HISTORY) undo_stack.pop_front();
    undo_stack.push_back(std::move(action));
    redo_stack.clear();
}

// Libera todas las celdas, cabeceras de fila y columna, y limpia undo/redo stacks
void SparseMatrix::clear() {
    RowHeader* curr_row = row_head;
    while (curr_row != nullptr) {
        Cell* curr_cell = curr_row->first;
        while (curr_cell != nullptr) {
            Cell* next_cell = curr_cell->right;
            delete curr_cell;
            curr_cell = next_cell;
        }
        RowHeader* next_row = curr_row->next;
        delete curr_row;
        curr_row = next_row;
    }
    row_head = nullptr;

    ColHeader* curr_col = col_head;
    while (curr_col != nullptr) {
        ColHeader* next_col = curr_col->next;
        delete curr_col;
        curr_col = next_col;
    }
    col_head = nullptr;

    undo_stack.clear();
    redo_stack.clear();
}

// Guarda CellChange{before, after} y crea o actualiza la celda en (row, col)
void SparseMatrix::set(int row, int col, CellValue value) {
    Cell* existing = find_cell(row, col);
    std::optional<CellValue> before = existing ? std::optional<CellValue>{existing->val} : std::nullopt;
    push_action({{row, col, before, value}});

    if (existing != nullptr) {
        existing->val = std::move(value);
        return;
    }

    RowHeader* rh = find_or_create_row(row);
    ColHeader* ch = find_or_create_col(col);
    Cell* new_cell = new Cell(row, col, std::move(value));
    insert_cell_in_row(rh, new_cell);
    insert_cell_in_col(ch, new_cell);
}

// Guarda un Action con todos los CellChange del batch y aplica cada celda
void SparseMatrix::set_batch(const std::vector<std::tuple<int, int, CellValue>>& cells) {
    Action action;
    for (auto& [row, col, value] : cells) {
        Cell* existing = find_cell(row, col);
        action.push_back({row, col, existing ? std::optional<CellValue>{existing->val} : std::nullopt, value});
    }
    push_action(action);

    for (auto& [row, col, value] : cells) {
        Cell* existing = find_cell(row, col);
        if (existing != nullptr) {
            existing->val = value;
        } else {
            RowHeader* rh = find_or_create_row(row);
            ColHeader* ch = find_or_create_col(col);
            Cell* new_cell = new Cell(row, col, value);
            insert_cell_in_row(rh, new_cell);
            insert_cell_in_col(ch, new_cell);
        }
    }
}

// Retorna el valor de la celda en (row, col), o nullopt si no existe
std::optional<CellValue> SparseMatrix::get(int row, int col) const {
    Cell* cell = find_cell(row, col);
    if (cell == nullptr) return std::nullopt;
    return cell->val;
}

// Guarda CellChange{before, nullopt} y elimina la celda en (row, col)
void SparseMatrix::clear_cell(int row, int col) {
    Cell* cell = find_cell(row, col);
    if (cell == nullptr) return;
    push_action({{row, col, cell->val, std::nullopt}});
    delete_cell_node(row, col);
}

// Guarda un Action con todos los CellChange de la fila y elimina cada celda y el RowHeader
void SparseMatrix::delete_row(int row) {
    RowHeader* curr_row = row_head;
    while (curr_row != nullptr && curr_row->row != row) curr_row = curr_row->next;
    if (curr_row == nullptr) return;

    Action action;
    Cell* curr_cell = curr_row->first;
    while (curr_cell != nullptr) {
        action.push_back({curr_cell->row, curr_cell->col, curr_cell->val, std::nullopt});
        curr_cell = curr_cell->right;
    }
    push_action(std::move(action));

    curr_cell = curr_row->first;
    while (curr_cell != nullptr) {
        Cell* next_cell = curr_cell->right;
        ColHeader* curr_col = col_head;
        while (curr_col != nullptr && curr_col->col != curr_cell->col) curr_col = curr_col->next;
        remove_cell_from_col(curr_col, curr_cell);
        delete curr_cell;
        curr_cell = next_cell;
    }

    if (curr_row->prev != nullptr) curr_row->prev->next = curr_row->next;
    else row_head = curr_row->next;
    if (curr_row->next != nullptr) curr_row->next->prev = curr_row->prev;
    delete curr_row;
}

// Ídem para la columna col
void SparseMatrix::delete_col(int col) {
    ColHeader* curr_col = col_head;
    while (curr_col != nullptr && curr_col->col != col) curr_col = curr_col->next;
    if (curr_col == nullptr) return;

    Action action;
    Cell* curr_cell = curr_col->first;
    while (curr_cell != nullptr) {
        action.push_back({curr_cell->row, curr_cell->col, curr_cell->val, std::nullopt});
        curr_cell = curr_cell->down;
    }
    push_action(std::move(action));

    curr_cell = curr_col->first;
    while (curr_cell != nullptr) {
        Cell* next_cell = curr_cell->down;
        RowHeader* curr_row = row_head;
        while (curr_row != nullptr && curr_row->row != curr_cell->row) curr_row = curr_row->next;
        remove_cell_from_row(curr_row, curr_cell);
        delete curr_cell;
        curr_cell = next_cell;
    }

    if (curr_col->prev != nullptr) curr_col->prev->next = curr_col->next;
    else col_head = curr_col->next;
    if (curr_col->next != nullptr) curr_col->next->prev = curr_col->prev;
    delete curr_col;
}

// Elimina todas las celdas dentro del rango rectangular (r1,c1)-(r2,c2)
void SparseMatrix::delete_range(int r1, int c1, int r2, int c2) {
    Action action;
    for (int r = r1; r <= r2; r++) {
        for (int c = c1; c <= c2; c++) {
            Cell* cell = find_cell(r, c);
            if (cell != nullptr)
                action.push_back({r, c, cell->val, std::nullopt});
        }
    }
    if (action.empty()) return;
    push_action(action);

    for (auto& change : action)
        delete_cell_node(change.row, change.col);
}

// Recorre la columna col y retorna el row del último nodo, o nullopt si la columna no existe
std::optional<int> SparseMatrix::last_row_in_col(int col) const {
    ColHeader* curr_col = col_head;
    while (curr_col != nullptr && curr_col->col != col) curr_col = curr_col->next;
    if (curr_col == nullptr || curr_col->first == nullptr) return std::nullopt;

    Cell* curr_cell = curr_col->first;
    while (curr_cell->down != nullptr) curr_cell = curr_cell->down;
    return curr_cell->row;
}

// Revierte la última acción: aplica before en cada CellChange y mueve la acción a redo_stack
void SparseMatrix::undo() {
    if (undo_stack.empty()) return;
    Action action = std::move(undo_stack.back());
    undo_stack.pop_back();

    for (auto& change : action) {
        if (change.before.has_value()) {
            Cell* existing = find_cell(change.row, change.col);
            if (existing != nullptr) existing->val = *change.before;
            else {
                RowHeader* rh = find_or_create_row(change.row);
                ColHeader* ch = find_or_create_col(change.col);
                Cell* new_cell = new Cell(change.row, change.col, *change.before);
                insert_cell_in_row(rh, new_cell);
                insert_cell_in_col(ch, new_cell);
            }
        } else {
            delete_cell_node(change.row, change.col);
        }
    }

    redo_stack.push_back(std::move(action));
}

// Reaaplica la última acción deshecha: aplica after en cada CellChange y mueve la acción a undo_stack
void SparseMatrix::redo() {
    if (redo_stack.empty()) return;
    Action action = std::move(redo_stack.back());
    redo_stack.pop_back();

    for (auto& change : action) {
        if (change.after.has_value()) {
            Cell* existing = find_cell(change.row, change.col);
            if (existing != nullptr) existing->val = *change.after;
            else {
                RowHeader* rh = find_or_create_row(change.row);
                ColHeader* ch = find_or_create_col(change.col);
                Cell* new_cell = new Cell(change.row, change.col, *change.after);
                insert_cell_in_row(rh, new_cell);
                insert_cell_in_col(ch, new_cell);
            }
        } else {
            delete_cell_node(change.row, change.col);
        }
    }

    undo_stack.push_back(std::move(action));
}

bool SparseMatrix::can_undo() const { return !undo_stack.empty(); }
bool SparseMatrix::can_redo() const { return !redo_stack.empty(); }

// Recorre todas las celdas activas y llama f(row, col, val) por cada una
void SparseMatrix::for_each(std::function<void(int, int, const CellValue&)> f) const {
    RowHeader* curr_row = row_head;
    while (curr_row != nullptr) {
        Cell* curr_cell = curr_row->first;
        while (curr_cell != nullptr) {
            f(curr_cell->row, curr_cell->col, curr_cell->val);
            curr_cell = curr_cell->right;
        }
        curr_row = curr_row->next;
    }
}

// Recorre row_head y retorna el mayor índice de fila activa
int SparseMatrix::max_row() const {
    int max = 0;
    RowHeader* curr_row = row_head;
    while (curr_row != nullptr) {
        if (curr_row->row > max) max = curr_row->row;
        curr_row = curr_row->next;
    }
    return max;
}

// Recorre col_head y retorna el mayor índice de columna activa
int SparseMatrix::max_col() const {
    int max = 0;
    ColHeader* curr_col = col_head;
    while (curr_col != nullptr) {
        if (curr_col->col > max) max = curr_col->col;
        curr_col = curr_col->next;
    }
    return max;
}
