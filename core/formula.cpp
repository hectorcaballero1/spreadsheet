#include "formula.h"
#include <cctype>
#include <optional>
#include <set>
#include <string>

CellCoord parse_cell_id(const std::string& cell_id) {
    int i = 0;
    int col = 0;
    while (i < (int)cell_id.size() && std::isalpha(cell_id[i])) {
        col = col * 26 + (cell_id[i] - 'A' + 1);
        i++;
    }
    int row = std::stoi(cell_id.substr(i));
    return {row - 1, col - 1};
}

std::string col_to_letter(int col) {
    std::string result = "";
    while (col >= 0) {
        char letter = 'A' + (col % 26);
        result = letter + result;
        col = (col / 26) - 1;
    }
    return result;
}

// Helpers para evaluar formulas y detectar ciclos/errores

static CellValue eval_helper(const std::string&, const SparseMatrix&, std::set<CellCoord>&);

static CellValue parse_number(const std::string& token) {
    try {
        size_t pos;
        int ival = std::stoi(token, &pos);
        if (pos == token.size()) return ival;
        double dval = std::stod(token, &pos);
        if (pos == token.size()) return dval;
    } catch (...) {}
    return std::string{"#ERROR"};
}

static CellValue resolve_cell(const std::string& token, const SparseMatrix& matrix, std::set<CellCoord>& visited) {
    auto [r, c] = parse_cell_id(token);
    if (visited.count({r, c})) return std::string{"#CICLO"};
    visited.insert({r, c});
    auto raw = matrix.get(r, c);
    if (!raw) return std::string{"#ERROR"};
    if (auto* s = std::get_if<std::string>(&*raw)) {
        bool is_formula = ((*s)[0] == '=');
        if (is_formula) return eval_helper(*s, matrix, visited);
        return std::string{"#ERROR"}; // texto plano no es operable
    }
    return *raw;
}

static CellValue resolve_operand(const std::string& token, const SparseMatrix& matrix, std::set<CellCoord>& visited) {
    if (token.empty()) return std::string{"#ERROR"};
    if (std::isalpha(token[0])) return resolve_cell(token, matrix, visited);
    return parse_number(token);
}

static std::optional<double> to_double(const CellValue& v) {
    if (auto* i = std::get_if<int>(&v))    return (double)*i;
    if (auto* d = std::get_if<double>(&v)) return *d;
    return std::nullopt;
}

static CellValue apply_op(char op, const CellValue& left, const CellValue& right) {
    auto l = to_double(left);
    auto r = to_double(right);

    if (!l || !r) {
        auto* sl = std::get_if<std::string>(&left);
        if (sl && *sl == "#CICLO") return left;
        auto* sr = std::get_if<std::string>(&right);
        if (sr && *sr == "#CICLO") return right;
        return std::string{"#ERROR"};
    }
    
    double result;
    switch (op) {
        case '+': result = *l + *r; break;
        case '-': result = *l - *r; break;
        case '*': result = *l * *r; break;
        case '/':
            if (*r == 0) return std::string{"#ERROR"};
            result = *l / *r;
            break;
        default: return std::string{"#ERROR"};
    }
    if (result == (int)result) return (int)result;
    return result;
}

static CellValue eval_helper(const std::string& formula, const SparseMatrix& matrix, std::set<CellCoord>& visited) {
    std::string expr = formula.substr(1);

    int op_pos = -1;
    char op = 0;
    for (int i = 0; i < (int)expr.size(); i++) {
        char ch = expr[i];
        if (ch == '+' or ch == '-' or ch == '*' or ch == '/') {
            op_pos = i;
            op = ch;
            break;
        }
    }

    if (op_pos == -1) return resolve_operand(expr, matrix, visited);

    CellValue left  = resolve_operand(expr.substr(0, op_pos), matrix, visited);
    CellValue right = resolve_operand(expr.substr(op_pos + 1), matrix, visited);
    return apply_op(op, left, right);
}

// Public API

CellValue evaluate_formula(const std::string& formula, int row, int col, const SparseMatrix& matrix) {
    std::set<CellCoord> visited;
    visited.insert({row, col});
    return eval_helper(formula, matrix, visited);
}
