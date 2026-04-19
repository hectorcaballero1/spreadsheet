from textual.widget import Widget
from textual.strip import Strip
from rich.segment import Segment
from rich.style import Style

try:
    import spreadsheet as _sp
except ImportError:
    _sp = None

ROW_HEADER_WIDTH = 5

# Catppuccin Mocha
BG      = "#1e1e2e"
SURFACE = "#313244"
TEXT    = "#cdd6f4"
SUBTEXT = "#a6adc8"
BLUE    = "#89b4fa"
GREEN   = "#a6e3a1"
RED     = "#f38ba8"
VISUAL  = "#45475a"

STYLE_GRID          = Style(bgcolor=BG,      color=TEXT)
STYLE_HEADER        = Style(bgcolor=SURFACE, color=SUBTEXT, bold=True)
STYLE_HEADER_ACTIVE = Style(bgcolor=SURFACE, color=BLUE,    bold=True)
STYLE_SELECTED      = Style(bgcolor=BLUE,    color=BG,      bold=True)
STYLE_VISUAL        = Style(bgcolor=VISUAL,  color=TEXT)
STYLE_FORMULA       = Style(bgcolor=BG,      color=GREEN)
STYLE_ERROR         = Style(bgcolor=BG,      color=RED)


def _col_letter(col: int) -> str:
    letters = ""
    while True:
        letters = chr(ord("A") + col % 26) + letters
        col = col // 26 - 1
        if col < 0:
            break
    return letters


def _fit(text: str, width: int) -> str:
    if len(text) >= width:
        return text[:width - 1] + "…" if width > 1 else " "
    return text.ljust(width)


class GridWidget(Widget):

    def render_line(self, y: int) -> Strip:
        app   = self.app
        width = self.size.width
        if y == 0:
            return self._col_headers(width, app)
        data_r = app.scroll_row + (y - 1)
        return self._grid_row(data_r, width, app)

    def _col_headers(self, width: int, app) -> Strip:
        segs = [Segment(" " * ROW_HEADER_WIDTH, STYLE_HEADER)]
        x   = ROW_HEADER_WIDTH
        col = app.scroll_col
        while x < width:
            cw    = app.col_width(col)
            label = _fit(_col_letter(col).center(cw), cw)
            style = STYLE_HEADER_ACTIVE if col == app.cursor_col else STYLE_HEADER
            segs.append(Segment(label, style))
            x += cw
            col += 1
        return Strip(segs, width)

    def _grid_row(self, data_r: int, width: int, app) -> Strip:
        vr = self._visual_range(app)

        segs = []
        row_style = STYLE_HEADER_ACTIVE if data_r == app.cursor_row else STYLE_HEADER
        segs.append(Segment(str(data_r + 1).rjust(ROW_HEADER_WIDTH - 1) + " ", row_style))

        x   = ROW_HEADER_WIDTH
        col = app.scroll_col
        while x < width:
            cw       = app.col_width(col)
            selected = (data_r == app.cursor_row and col == app.cursor_col)
            in_visual = vr is not None and vr[0] <= data_r <= vr[2] and vr[1] <= col <= vr[3]

            if selected and app.mode == app.MODE_INSERT:
                segs.append(Segment(_fit(app.input_buffer + "▏", cw), STYLE_SELECTED))
            else:
                text, style = self._cell_content(data_r, col, app)
                if selected:
                    style = STYLE_SELECTED
                elif in_visual:
                    style = STYLE_VISUAL
                segs.append(Segment(_fit(text, cw), style))
            x += cw
            col += 1
        return Strip(segs, width)

    def _cell_content(self, row: int, col: int, app) -> tuple[str, Style]:
        raw = app.sheet.get(row, col)
        if raw is None:
            return "", STYLE_GRID
        if isinstance(raw, str) and raw.startswith("="):
            if _sp:
                try:
                    return str(_sp.evaluate_formula(raw, row, col, app.sheet)), STYLE_FORMULA
                except Exception:
                    return "#ERROR", STYLE_ERROR
            return raw, STYLE_FORMULA
        return str(raw), STYLE_GRID

    def _visual_range(self, app):
        if app.mode != app.MODE_VISUAL or app.visual_anchor is None:
            return None
        ar, ac = app.visual_anchor
        return (min(ar, app.cursor_row), min(ac, app.cursor_col),
                max(ar, app.cursor_row), max(ac, app.cursor_col))
