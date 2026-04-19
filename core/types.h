#pragma once
#include <string>
#include <utility>
#include <variant>

using CellValue = std::variant<int, double, std::string>;
using CellCoord = std::pair<int, int>;
