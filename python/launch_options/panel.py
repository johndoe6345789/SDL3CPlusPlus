"""The per-game options form, beside the bootloader list.

Rebuilt per game; remembers input and returns the launch environment.
"""

from __future__ import annotations

from pathlib import Path
from typing import Callable

from PyQt6.QtCore import QSettings
from PyQt6.QtWidgets import QFormLayout, QWidget

from .choice_field import ChoiceField
from .factory import hint_for, make_field, make_label
from .spec import LaunchOption, load_launch_options
from .style import apply_style

ChoiceProvider = Callable[[dict[str, str]], list[str]]


class LaunchOptionsPanel(QWidget):
    def __init__(self, providers: dict[str, ChoiceProvider], parent=None):
        super().__init__(parent)
        self._providers = providers
        self._settings = QSettings("SDL3CPlusPlus", "Launcher")
        self._form = QFormLayout(self)
        self._form.setContentsMargins(0, 0, 0, 0)
        self._fields: list = []
        self._game, self._detected = "", {}
        apply_style(self)
        self.setVisible(False)

    def set_package(self, game_id: str, package_dir: Path,
                    detected: dict[str, str]) -> None:
        while self._form.rowCount():
            self._form.removeRow(0)
        self._fields = []
        self._game, self._detected = game_id, detected
        for option in load_launch_options(package_dir):
            self._add_row(option)
        self._refresh_choices()
        self.setVisible(bool(self._fields))

    def _add_row(self, option: LaunchOption) -> None:
        field = make_field(option)
        key = f"{self._game}/{option.env}"
        saved = self._settings.value(key)
        field.set_value(option.default if saved is None else str(saved))
        field.set_hint(hint_for(option, self._detected))
        field.widget.setToolTip(option.help or option.env)
        field.changed.connect(self._on_changed)
        self._form.addRow(make_label(option), field.widget)
        self._fields.append(field)

    def _on_changed(self, _text: str = "") -> None:
        for field in self._fields:
            key = f"{self._game}/{field.option.env}"
            self._settings.setValue(key, field.value())
        self._refresh_choices()

    def _refresh_choices(self) -> None:
        env = {**self._detected, **self.values()}
        for field in self._fields:
            provider = self._providers.get(field.option.choices_from)
            if isinstance(field, ChoiceField) and provider:
                field.set_choices(provider(env))

    def values(self) -> dict[str, str]:
        return {f.option.env: f.value() for f in self._fields if f.value()}

    def missing(self) -> list[str]:
        """Options that would stop the game from starting, as labels."""
        empty = [f.option.label for f in self._fields
                 if f.option.required and not f.value()
                 and not f.option.default and f.option.env
                 not in self._detected]
        return empty + [f"{f.option.label} (not found)"
                        for f in self._fields
                        if getattr(f, "not_found", False)]
