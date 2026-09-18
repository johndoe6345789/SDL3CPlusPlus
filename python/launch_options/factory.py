"""Builds an option's widget, its label and the hint shown when empty."""

from __future__ import annotations

from PyQt6.QtWidgets import QLabel

from .choice_field import ChoiceField
from .fields import PathField, TextField
from .spec import LaunchOption

_LABEL_STYLE = "color: #8f98a0; font-weight: bold; font-size: 9pt;"


def make_field(option: LaunchOption):
    if option.kind in ("directory", "file"):
        return PathField(option)
    if option.kind == "choice":
        return ChoiceField(option)
    return TextField(option)


def hint_for(option: LaunchOption, detected: dict[str, str]) -> str:
    if option.env in detected:
        return f"auto: {detected[option.env]}"
    if option.default:
        return f"default: {option.default}"
    return "required" if option.required else "optional"


def make_label(option: LaunchOption) -> QLabel:
    """Upper-case label; a trailing * marks a required option."""
    label = QLabel(option.label.upper() + " *" * option.required)
    label.setStyleSheet(_LABEL_STYLE)
    label.setToolTip(option.help or option.env)
    return label
