#pragma once
#include "types.h"
#include <deque>
#include <functional>
#include <optional>
#include <vector>

class SparseMatrix {
    struct Cell {
        int row, col;
        CellValue val;
        Cell* right = nullptr;
        Cell* left = nullptr;
        Cell* down = nullptr;
        Cell* up = nullptr;
        Cell(int r, int c, CellValue v) : row(r), col(c), val(std::move(v)) {}
    };

    struct RowHeader {
        int row;
        Cell* first = nullptr;
        RowHeader* next = nullptr;
        RowHeader* prev = nullptr;
        explicit RowHeader(int r) : row(r) {}
    };

    struct ColHeader {
        int col;
        Cell* first = nullptr;
        ColHeader* next = nullptr;
        ColHeader* prev = nullptr;
        explicit ColHeader(int c) : col(c) {}
    };

    struct CellChange {
        int row, col;
        std::optional<CellValue> before;
        std::optional<CellValue> after;
    };
    using Action = std::vector<CellChange>;

    std::deque<Action> undo_stack;
    std::deque<Action> redo_stack;
    static constexpr int MAX_HISTORY = 100;

    Cell* find_cell(int row, int col) const;
    RowHeader* find_or_create_row(int row);
    ColHeader* find_or_create_col(int col);
    void insert_cell_in_row(RowHeader* rh, Cell* cell);
    void insert_cell_in_col(ColHeader* ch, Cell* cell);
    void remove_cell_from_row(RowHeader* rh, Cell* cell);
    void remove_cell_from_col(ColHeader* ch, Cell* cell);
    void delete_cell_node(int row, int col);
    void push_action(Action action);
    void clear();

    RowHeader* row_head = nullptr;
    ColHeader* col_head = nullptr;

    public:
    
    SparseMatrix() = default;
    ~SparseMatrix();

    void set(int row, int col, CellValue value);
    std::optional<CellValue> get(int row, int col) const;
    void clear_cell(int row, int col);

    void delete_row(int row);
    void delete_col(int col);
    void delete_range(int r1, int c1, int r2, int c2);

    std::optional<int> last_row_in_col(int col) const;

    void undo();
    void redo();
    bool can_undo() const;
    bool can_redo() const;

    void for_each(std::function<void(int, int, const CellValue&)> f) const;

    int max_row() const;
    int max_col() const;
};
