# Hoja de Cálculo

Hoja de cálculo de terminal implementada en C++. Los datos viven en una matriz dispersa con listas enlazadas cruzadas expuesta a Python via pybind11.

## Prerequisitos

- Python 3.12+
- `uv`
- `g++` con soporte C++17

## Compilar y ejecutar

```bash
uv run build.py   # compilar el core C++ (solo la primera vez o al cambiar .cpp)
uv run main.py    # ejecutar
```

## Uso

La TUI tiene cuatro modos: NORMAL, INSERT, VISUAL y COMMAND.

### Modo NORMAL

| Tecla | Acción |
|---|---|
| `h j k l` o flechas | Mover cursor |
| `gg` | Ir a la fila 0 |
| `G` | Ir a la última celda ocupada de la columna actual |
| `0` | Ir a la columna 0 |
| `$` | Ir a la última columna con datos |
| `i` o `Enter` | Entrar a modo INSERT |
| `v` | Entrar a modo VISUAL |
| `Backspace` / `Delete` | Borrar celda actual |
| `dd` | Borrar fila actual |
| `dc` | Borrar columna actual |
| `u` | Deshacer |
| `Ctrl+R` | Rehacer |
| `:` | Entrar a modo COMMAND |
| `q` | Salir |

### Modo INSERT

| Tecla | Acción |
|---|---|
| Cualquier caracter | Escribe en la celda |
| `Enter` | Confirmar y volver a NORMAL |
| `Esc` | Cancelar sin guardar |
| `Backspace` | Borrar último caracter |

### Modo VISUAL

Selecciona un rango rectangular moviéndote con las flechas desde el punto de anclaje.

| Tecla | Acción |
|---|---|
| Flechas / `h j k l` | Extender selección |
| `Backspace` / `Delete` | Borrar rango seleccionado |
| `Esc` | Cancelar selección |

### Modo COMMAND

| Comando | Descripción |
|---|---|
| `:w [archivo.csv]` | Guardar en CSV |
| `:e <archivo.csv>` | Cargar desde CSV |
| `:wq` | Guardar y salir |
| `:q` | Salir sin guardar |
| `:goto A5` | Mover cursor a celda |
| `:delrow` | Eliminar fila actual |
| `:delcol` | Eliminar columna actual |
| `:sum A1:C4` | Suma del rango |
| `:avg A1:C4` | Promedio del rango |
| `:max A1:C4` | Máximo del rango |
| `:min A1:C4` | Mínimo del rango |
| `:undo` | Deshacer |
| `:redo` | Rehacer |

### Fórmulas

Las fórmulas inician con `=` y soportan dos operandos con `+ - * /`. Los operandos pueden ser números o referencias a celdas:

```
=A1+B1
=A1*2
=B3/A2
```

Si una referencia apunta a otra celda con fórmula, se resuelve recursivamente. Si hay un ciclo de dependencias, la celda muestra `#CICLO`.
