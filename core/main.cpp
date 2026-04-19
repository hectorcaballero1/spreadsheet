#include <iostream>
#include "sparse_matrix.h"
#include "formula.h"

int main() {
    SparseMatrix sheet;

    // Prueba básica: set / get
    sheet.set(0, 0, 10);
    sheet.set(0, 1, 20);
    sheet.set(1, 0, std::string{"=A1+B1"});

    auto display = [&](int r, int c) {
        auto val = sheet.get(r, c);
        if (!val) { std::cout << "(vacía)"; return; }
        if (auto* s = std::get_if<std::string>(&*val); s && (*s)[0] == '=') {
            auto result = evaluate_formula(*s, r, c, sheet);
            std::visit([](auto&& v) { std::cout << v; }, result);
        } else {
            std::visit([](auto&& v) { std::cout << v; }, *val);
        }
    };

    std::cout << "A1="; display(0, 0); std::cout << "\n";
    std::cout << "B1="; display(0, 1); std::cout << "\n";
    std::cout << "A2="; display(1, 0); std::cout << "\n";

    return 0;
}
