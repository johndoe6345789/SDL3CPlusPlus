"""Launcher colours for the options form.

The main window only sets a palette, which the native Windows style
ignores for line edits, so their text came out black on dark blue.
A style sheet is honoured by every platform style.
"""

from __future__ import annotations

from PyQt6.QtGui import QColor, QPalette
from PyQt6.QtWidgets import QWidget

_SHEET = """
QLineEdit, QComboBox {
    background-color: #171a21;
    color: #c6d1db;
    border: 1px solid #0e1216;
    border-radius: 3px;
    padding: 4px 6px;
    selection-background-color: #2a475e;
}
QComboBox QAbstractItemView {
    background-color: #171a21;
    color: #c6d1db;
    selection-background-color: #2a475e;
}
QPushButton {
    background-color: #2a475e;
    color: #c6d1db;
    border: none;
    border-radius: 3px;
    padding: 5px 12px;
}
QPushButton:hover {
    background-color: #3e5c78;
}
"""


def apply_style(widget: QWidget) -> None:
    palette = widget.palette()
    palette.setColor(QPalette.ColorRole.PlaceholderText, QColor("#6b7785"))
    palette.setColor(QPalette.ColorRole.Text, QColor("#c6d1db"))
    widget.setPalette(palette)
    widget.setStyleSheet(_SHEET)
