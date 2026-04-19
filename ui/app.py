import csv
from textual.app import App, ComposeResult
from textual.widgets import Static
from textual import events

from .grid import GridWidget


class SpreadsheetApp(App):

    CSS = """
    Screen {
        layout: vertical;
        background: #1e1e2e;
    }
    #formula-bar {
        height: 1;
        background: #313244;
        color: #cdd6f4;
    }
    GridWidget {
        height: 1fr;
    }
    #status-bar {
        height: 1;
        background: #313244;
        color: #cdd6f4;
    }
    """

    MODE_NORMAL  = "NORMAL"
    MODE_INSERT  = "INSERT"
    MODE_VISUAL  = "VISUAL"
    MODE_COMMAND = "COMMAND"

    def __init__(self, sheet):
        super().__init__()
        self.sheet = sheet
        self.mode = self.MODE_NORMAL
        self.cursor_row = 0
        self.cursor_col = 0
        self.scroll_row = 0
        self.scroll_col = 0
        self.input_buffer = ""
        self.command_buffer = ""
        self.status_message = ""
        self.col_widths: dict[int, int] = {}
        self.filename: str | None = None
        self.visual_anchor: tuple[int, int] | None = None
        self._g_pending = False
        self._d_pending = False

    def compose(self) -> ComposeResult:
        yield Static("", id="formula-bar")
        yield GridWidget()
        yield Static("", id="status-bar")

    def on_mount(self) -> None:
        self._refresh_ui()

    # ── Refresco de UI ────────────────────────────────────────────────────────

    def _refresh_ui(self) -> None:
        cell_id = self._cell_id(self.cursor_row, self.cursor_col)

        if self.mode == self.MODE_INSERT:
            formula_content = f" {cell_id}  {self.input_buffer}▏"
        else:
            raw = self.sheet.get(self.cursor_row, self.cursor_col)
            formula_content = f" {cell_id}  {raw if raw is not None else ''}"

        if self.mode == self.MODE_COMMAND:
            status = f" {self.mode}  :{self.command_buffer}"
        else:
            msg = f"  {self.status_message}" if self.status_message else ""
            status = f" {self.mode}{msg}"

        self.query_one("#formula-bar", Static).update(formula_content)
        self.query_one("#status-bar", Static).update(status)
        self.query_one(GridWidget).refresh()

    # ── Entrada de teclas ─────────────────────────────────────────────────────

    def on_key(self, event: events.Key) -> None:
        event.stop()
        self.status_message = ""

        if self.mode == self.MODE_NORMAL:
            self._normal(event)
        elif self.mode == self.MODE_INSERT:
            self._insert(event)
        elif self.mode == self.MODE_COMMAND:
            self._command(event)
        elif self.mode == self.MODE_VISUAL:
            self._visual(event)

        self._refresh_ui()

    def _normal(self, event: events.Key) -> None:
        key  = event.key
        char = event.character

        if self._g_pending:
            self._g_pending = False
            if char == "g":
                self.cursor_row = 0
                self._clamp_scroll()
            return

        if self._d_pending:
            self._d_pending = False
            if char == "d":
                self.sheet.delete_row(self.cursor_row)
                self.status_message = f"Fila {self.cursor_row + 1} eliminada"
            elif char == "c":
                self.sheet.delete_col(self.cursor_col)
                self.status_message = "Columna eliminada"
            return

        if   char == "h" or key == "left":  self.move_cursor(0, -1)
        elif char == "l" or key == "right": self.move_cursor(0, 1)
        elif char == "k" or key == "up":    self.move_cursor(-1, 0)
        elif char == "j" or key in ("down", "enter"): self.move_cursor(1, 0)
        elif char == "g": self._g_pending = True
        elif char == "G":
            last = self.sheet.last_row_in_col(self.cursor_col)
            if last is None:
                self.status_message = "Columna vacía"
            else:
                self.cursor_row = last
                self._clamp_scroll()
        elif char == "0": self.cursor_col = 0; self._clamp_scroll()
        elif char == "$": self.cursor_col = max(0, self.sheet.max_col()); self._clamp_scroll()
        elif char == "i" or key == "enter": self._enter_insert_mode()
        elif char == "v": self._enter_visual_mode()
        elif key in ("backspace", "delete"): self.sheet.clear_cell(self.cursor_row, self.cursor_col)
        elif char == "d": self._d_pending = True
        elif char == "u":
            if self.sheet.can_undo(): self.sheet.undo()
            else: self.status_message = "Nada que deshacer"
        elif key == "ctrl+r":
            if self.sheet.can_redo(): self.sheet.redo()
            else: self.status_message = "Nada que rehacer"
        elif char == ":": self._enter_command_mode()
        elif char == "q": self.exit()

    def _insert(self, event: events.Key) -> None:
        key  = event.key
        char = event.character

        if key == "escape":
            self._enter_normal_mode(commit=False)
        elif key == "enter":
            self._enter_normal_mode(commit=True)
            self.move_cursor(1, 0)
        elif key == "backspace":
            self.input_buffer = self.input_buffer[:-1]
        elif key == "delete":
            self.input_buffer = ""
        elif char and 32 <= ord(char) <= 126:
            self.input_buffer += char

    def _visual(self, event: events.Key) -> None:
        key  = event.key
        char = event.character

        if   char == "h" or key == "left":  self.move_cursor(0, -1)
        elif char == "l" or key == "right": self.move_cursor(0, 1)
        elif char == "k" or key == "up":    self.move_cursor(-1, 0)
        elif char == "j" or key == "down":  self.move_cursor(1, 0)
        elif key in ("backspace", "delete"): self._delete_visual_range()
        elif key == "escape": self._exit_visual_mode()

    def _command(self, event: events.Key) -> None:
        key  = event.key
        char = event.character

        if key == "escape":
            self.mode = self.MODE_NORMAL
            self.command_buffer = ""
        elif key == "enter":
            cmd = self.command_buffer
            self.mode = self.MODE_NORMAL
            self.command_buffer = ""
            if not self._execute_command(cmd):
                self.exit()
        elif key == "backspace":
            self.command_buffer = self.command_buffer[:-1]
        elif char and 32 <= ord(char) <= 126:
            self.command_buffer += char

    # ── Cambio de modo ────────────────────────────────────────────────────────

    def _enter_insert_mode(self) -> None:
        val = self.sheet.get(self.cursor_row, self.cursor_col)
        self.input_buffer = str(val) if val is not None else ""
        self.mode = self.MODE_INSERT

    def _enter_normal_mode(self, commit: bool = True) -> None:
        if self.mode == self.MODE_INSERT and commit:
            self._commit_input()
        self.input_buffer = ""
        self.command_buffer = ""
        self.mode = self.MODE_NORMAL

    def _enter_command_mode(self) -> None:
        self.command_buffer = ""
        self.mode = self.MODE_COMMAND

    def _enter_visual_mode(self) -> None:
        self.visual_anchor = (self.cursor_row, self.cursor_col)
        self.mode = self.MODE_VISUAL

    def _exit_visual_mode(self) -> None:
        self.visual_anchor = None
        self.mode = self.MODE_NORMAL

    # ── Edición de celdas ─────────────────────────────────────────────────────

    def _commit_input(self) -> None:
        text = self.input_buffer.strip()
        if not text:
            self.sheet.clear_cell(self.cursor_row, self.cursor_col)
            return
        if text.startswith("="):
            self.sheet.set_str(self.cursor_row, self.cursor_col, text)
        else:
            try:
                self.sheet.set_int(self.cursor_row, self.cursor_col, int(text))
            except ValueError:
                try:
                    self.sheet.set_float(self.cursor_row, self.cursor_col, float(text))
                except ValueError:
                    self.sheet.set_str(self.cursor_row, self.cursor_col, text)
        self._recompute_col_width(self.cursor_col)

    def _delete_visual_range(self) -> None:
        r1, c1, r2, c2 = self._visual_range()
        self.sheet.delete_range(r1, c1, r2, c2)
        self._exit_visual_mode()

    def _visual_range(self) -> tuple[int, int, int, int]:
        ar, ac = self.visual_anchor
        r1, r2 = min(ar, self.cursor_row), max(ar, self.cursor_row)
        c1, c2 = min(ac, self.cursor_col), max(ac, self.cursor_col)
        return r1, c1, r2, c2

    # ── Movimiento ────────────────────────────────────────────────────────────

    def move_cursor(self, dr: int, dc: int) -> None:
        self.cursor_row = max(0, self.cursor_row + dr)
        self.cursor_col = max(0, self.cursor_col + dc)
        self._clamp_scroll()

    def _clamp_scroll(self) -> None:
        grid = self.query_one(GridWidget)
        height = grid.size.height - 1
        width  = grid.size.width

        if self.cursor_row < self.scroll_row:
            self.scroll_row = self.cursor_row
        elif self.cursor_row >= self.scroll_row + height:
            self.scroll_row = self.cursor_row - height + 1

        if self.cursor_col < self.scroll_col:
            self.scroll_col = self.cursor_col
        else:
            x = 4  # ROW_HEADER_WIDTH
            for c in range(self.scroll_col, self.cursor_col + 1):
                x += self.col_width(c)
                if x > width:
                    self.scroll_col += 1
                    break

    # ── Comandos ──────────────────────────────────────────────────────────────

    def _execute_command(self, cmd: str) -> bool:
        parts = cmd.strip().split()
        if not parts:
            return True
        verb = parts[0].lower()

        if verb == "q":
            return False
        elif verb == "wq":
            path = parts[1] if len(parts) > 1 else (self.filename or "sheet.csv")
            self._save_csv(path)
            return False
        elif verb == "w":
            path = parts[1] if len(parts) > 1 else (self.filename or "sheet.csv")
            self._save_csv(path)
            self.filename = path
            self.status_message = f"Guardado en {path}"
        elif verb == "e":
            if len(parts) < 2:
                self.status_message = "Uso: e <archivo>"
            else:
                self._load_csv(parts[1])
                self.filename = parts[1]
                self.status_message = f"Cargado {parts[1]}"
        elif verb == "undo":
            if self.sheet.can_undo(): self.sheet.undo()
            else: self.status_message = "Nada que deshacer"
        elif verb == "redo":
            if self.sheet.can_redo(): self.sheet.redo()
            else: self.status_message = "Nada que rehacer"
        elif verb == "goto":
            if len(parts) < 2:
                self.status_message = "Uso: goto A1"
            else:
                try:
                    import spreadsheet as _sp
                    r, c = _sp.parse_cell_id(parts[1])
                    self.cursor_row = r
                    self.cursor_col = c
                    self._clamp_scroll()
                except Exception:
                    self.status_message = f"Referencia inválida: {parts[1]}"
        elif verb == "delrow":
            self.sheet.delete_row(self.cursor_row)
            self.status_message = f"Fila {self.cursor_row + 1} eliminada"
        elif verb == "delcol":
            self.sheet.delete_col(self.cursor_col)
            self.status_message = "Columna eliminada"
        elif verb in ("sum", "avg", "max", "min"):
            self._aggregate(verb, parts[1] if len(parts) > 1 else None)
        else:
            self.status_message = f"Comando desconocido: {verb}"
        return True

    def _aggregate(self, func: str, range_str: str | None) -> None:
        if not range_str or ":" not in range_str:
            self.status_message = f"Uso: {func} A1:B3"
            return
        try:
            import spreadsheet as _sp
            a, b = range_str.split(":")
            r1, c1 = _sp.parse_cell_id(a)
            r2, c2 = _sp.parse_cell_id(b)
            values = []
            for r in range(min(r1, r2), max(r1, r2) + 1):
                for c in range(min(c1, c2), max(c1, c2) + 1):
                    v = self.sheet.get(r, c)
                    if v is not None and not isinstance(v, str):
                        values.append(float(v))
            if not values:
                self.status_message = "Rango vacío"
                return
            if func == "sum":   result = sum(values)
            elif func == "avg": result = sum(values) / len(values)
            elif func == "max": result = max(values)
            elif func == "min": result = min(values)
            self.status_message = f"{func.upper()}={result}"
        except Exception as e:
            self.status_message = f"Error: {e}"

    # ── CSV ───────────────────────────────────────────────────────────────────

    def _save_csv(self, path: str) -> None:
        max_r = self.sheet.max_row()
        max_c = self.sheet.max_col()
        with open(path, "w", newline="") as f:
            writer = csv.writer(f)
            for r in range(max_r + 1):
                row = []
                for c in range(max_c + 1):
                    v = self.sheet.get(r, c)
                    row.append("" if v is None else str(v))
                writer.writerow(row)

    def _load_csv(self, path: str) -> None:
        with open(path, newline="") as f:
            for r, row in enumerate(csv.reader(f)):
                for c, text in enumerate(row):
                    text = text.strip()
                    if not text:
                        continue
                    if text.startswith("="):
                        self.sheet.set_str(r, c, text)
                    else:
                        try:
                            self.sheet.set_int(r, c, int(text))
                        except ValueError:
                            try:
                                self.sheet.set_float(r, c, float(text))
                            except ValueError:
                                self.sheet.set_str(r, c, text)
                    self._recompute_col_width(c)

    # ── Helpers ───────────────────────────────────────────────────────────────

    def col_width(self, col: int) -> int:
        return self.col_widths.get(col, 10)

    def _cell_id(self, row: int, col: int) -> str:
        letters = ""
        c = col
        while True:
            letters = chr(ord("A") + c % 26) + letters
            c = c // 26 - 1
            if c < 0:
                break
        return f"{letters}{row + 1}"

    def _recompute_col_width(self, col: int) -> None:
        max_w = 4
        try:
            import spreadsheet as _sp
            def check(r, c, v):
                nonlocal max_w
                if c != col:
                    return
                if isinstance(v, str) and v.startswith("="):
                    try:
                        v = str(_sp.evaluate_formula(v, r, c, self.sheet))
                    except Exception:
                        v = "#ERROR"
                max_w = max(max_w, len(str(v)))
            self.sheet.for_each(check)
        except Exception:
            pass
        self.col_widths[col] = max(max_w + 2, 6)
