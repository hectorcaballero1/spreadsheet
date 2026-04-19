import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "build"))


def main():
    try:
        import spreadsheet
    except ImportError:
        print("Error: módulo C++ no compilado.")
        print("Ejecuta: uv run build.py")
        sys.exit(1)

    sheet = spreadsheet.SparseMatrix()
    from ui.app import SpreadsheetApp
    SpreadsheetApp(sheet).run()


if __name__ == "__main__":
    main()
