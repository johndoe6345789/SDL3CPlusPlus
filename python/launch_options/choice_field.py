"""Dropdown input for launch options whose values come from a provider.

A provider is a function the launcher registers by name; a package
points an option at it with "choices_from" (e.g. "quake3_maps").
"""

from __future__ import annotations

from PyQt6.QtWidgets import QComboBox

from .spec import LaunchOption


class ChoiceField:
    """An editable dropdown, so a value missing from the list still works."""

    def __init__(self, option: LaunchOption):
        self.option = option
        self.combo = QComboBox()
        self.combo.setEditable(True)
        self.widget = self.combo
        self.changed = self.combo.currentTextChanged

    def value(self) -> str:
        return self.combo.currentText().strip()

    def set_value(self, value: str) -> None:
        self.combo.setCurrentText(value)

    def set_hint(self, hint: str) -> None:
        self.combo.lineEdit().setPlaceholderText(hint)

    def set_choices(self, choices: list[str]) -> None:
        current = self.value()
        self.combo.blockSignals(True)
        self.combo.clear()
        self.combo.addItems(choices)
        self.combo.setCurrentText(current)
        self.combo.blockSignals(False)
