# Decisiones de Diseño y Complejidad

## Arquitectura general

El proyecto separa responsabilidades en dos capas:

- El core de datos vive en C++17: la matriz dispersa, el evaluador de fórmulas y el historial de undo/redo. C++ permite control directo sobre la memoria y punteros, lo cual es esencial para una estructura enlazada como esta.

- La interfaz de usuario está en Python usando Textual, una librería de TUI de alto nivel. Textual provee un sistema de widgets, manejo de eventos y estilos CSS-like que simplifica considerablemente el renderizado y la gestión de input. pybind11 compila el core como un módulo Python, por lo que la UI simplemente llama métodos del objeto `SparseMatrix` como si fuera una clase Python.

La decisión de usar Textual en lugar de ncurses en C++ permite implementar los modos vim-like (NORMAL, INSERT, VISUAL, COMMAND) con mucho menos código, y hace el proyecto más mantenible sin sacrificar rendimiento en la capa de datos.

## Estructura de datos: matriz dispersa con listas enlazadas cruzadas

Una hoja de cálculo tiene potencialmente millones de celdas, pero en la práctica solo una fracción pequeña tiene contenido. Usar una matriz bidimensional densa desperdiciaría memoria proporcional al tamaño total de la grilla independientemente de cuántas celdas estén ocupadas.

La estructura elegida es una lista enlazada cruzada (cross-linked list):

- Cada celda con contenido es un nodo con punteros `right`, `left`, `down`, `up`.
- Existe un nodo cabecera (`RowHeader`) por cada fila activa y un nodo cabecera (`ColHeader`) por cada columna activa.
- Los cabeceras son punteros en un lista enlazada, lo que permite que la matriz crezca dinámicamente sin redimensionar ningún arreglo.
- Solo se crean nodos para celdas con contenido. Una hoja con pocas celdas ocupa memoria proporcional a esas celdas, no al tamaño de la grilla.

## Complejidad de las operaciones

| Operación | Complejidad | Justificación |
|---|---|---|
| `set(row, col, value)` | O(R + C) | Buscar o crear RowHeader en O(R), ColHeader en O(C), insertar en la lista de la fila en O(k_r) |
| `get(row, col)` | O(R + k_r) | Buscar RowHeader en O(R), recorrer celdas de la fila en O(k_r) |
| `clear_cell(row, col)` | O(R + k_r) | Igual que get, más actualizar punteros vecinos en O(1) |
| `delete_row(row)` | O(R + k_r * C) | Encontrar la fila en O(R), borrar cada celda de la fila desenlazándola de su columna en O(C) |
| `delete_col(col)` | O(C + k_c * R) | Simétrico |
| `delete_range(r1,c1,r2,c2)` | O((r2-r1) * (c2-c1) * max(R,C)) | Un `clear_cell` por cada celda del rango |
| `for_each` | O(N) | Recorre todos los nodos existentes, N = celdas ocupadas |
| `undo` / `redo` | O(k) | k = número de cambios en la acción a revertir |

Donde R = número de filas activas, C = número de columnas activas, k_r = celdas en la fila, k_c = celdas en la columna, N = total de celdas ocupadas.

En comparación, una matriz densa tendría `get`/`set` en O(1) pero ocuparía O(n*m) memoria y `delete_row`/`delete_col` requerirían O(n*m) para desplazar elementos.

## Evaluación de fórmulas

Las fórmulas se almacenan como strings y se evalúan al momento de mostrarlas (evaluación lazy). Esto evita mantener un grafo de dependencias explícito y recalcular celdas en cascada al modificar un valor.

La detección de ciclos usa un `std::set<CellCoord>` de celdas visitadas en el stack de llamadas actual. Si al resolver una referencia la celda destino ya está en el conjunto, se devuelve `#CICLO` en lugar de entrar en recursión infinita.
