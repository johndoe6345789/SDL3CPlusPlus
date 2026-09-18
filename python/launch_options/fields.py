"""Text and path inputs for launch options.

Every field exposes the same small surface - widget, value(),
set_value(), set_hint() and a `changed` signal - so the panel never
has to care which kind it is holding.
"""

from __future__ import annotations

from pathlib import Path

from PyQt6.QtGui import QDoubleValidator
from PyQt6.QtWidgets import (
    QFileDialog, QHBoxLayout, QLineEdit, QPushButton, QWidget,
)

from .spec import LaunchOption

_MISSING_STYLE = "border: 1px solid #c13838;"


class TextField:
    def __init__(self, option: LaunchOption):
        self.option = option
        self.edit = QLineEdit()
        self.widget = self.edit
        self.changed = self.edit.textChanged
        if option.kind == "number":
            self.edit.setValidator(QDoubleValidator())

    def value(self) -> str:
        return self.edit.text().strip()

    def set_value(self, value: str) -> None:
        self.edit.setText(value)

    def set_hint(self, hint: str) -> None:
        self.edit.setPlaceholderText(hint)


class PathField(TextField):
    """A text box with a Browse... button for a folder or a file."""

    def __init__(self, option: LaunchOption):
        super().__init__(option)
        self.widget = QWidget()
        row = QHBoxLayout(self.widget)
        row.setContentsMargins(0, 0, 0, 0)
        row.addWidget(self.edit, 1)
        browse = QPushButton("Browse...")
        browse.clicked.connect(self._browse)
        row.addWidget(browse)
        self.not_found = False
        self.changed.connect(self._check)

    def _check(self, text: str) -> None:
        """Outline a path that is not on disk; the engine would fail."""
        path = Path(text.strip())
        is_dir = self.option.kind == "directory"
        exists = path.is_dir() if is_dir else path.is_file()
        bad = self.not_found = bool(text.strip()) and not exists
        self.edit.setStyleSheet(_MISSING_STYLE if bad else "")
        self.edit.setToolTip("Not found on disk" if bad
                             else self.option.help or self.option.env)

    def _browse(self) -> None:
        hint = self.edit.placeholderText().partition(": ")[2]
        start = self.value() or hint
        title = f"Choose {self.option.label}"
        if self.option.kind == "directory":
            picked = QFileDialog.getExistingDirectory(
                self.widget, title, start)
        else:
            picked, _ = QFileDialog.getOpenFileName(
                self.widget, title, start, self.option.file_filter)
        if picked:
            self.set_value(picked)
