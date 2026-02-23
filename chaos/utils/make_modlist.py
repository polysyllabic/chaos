#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
Generate a human-readable list of modifiers from a Chaos game configuration file.

If the main file references a template via ``input_file``, the utility loads the
template first and then overlays the main file, matching engine merge behavior.
"""

from __future__ import annotations

import argparse
import copy
import sys
import tomllib
from pathlib import Path
from typing import Any, TextIO

INPUT_FILE_KEY = "input_file"
ROLE_KEY = "chaos_toml"
ROLE_MAIN = "main"
ROLE_TEMPLATE = "template"


def _parse_toml(path: Path) -> dict[str, Any]:
  with path.open("rb") as in_f:
    return tomllib.load(in_f)


def _is_named_table_array(value: Any) -> bool:
  if not isinstance(value, list) or not value:
    return False
  for entry in value:
    if not isinstance(entry, dict):
      return False
    name = entry.get("name")
    if not isinstance(name, str) or not name:
      return False
  return True


def _merge_values(base_value: Any, overlay_value: Any) -> Any:
  if isinstance(base_value, dict) and isinstance(overlay_value, dict):
    _merge_tables(base_value, overlay_value)
    return base_value
  if isinstance(base_value, list) and isinstance(overlay_value, list):
    if _is_named_table_array(base_value) and _is_named_table_array(overlay_value):
      _merge_named_table_arrays(base_value, overlay_value)
      return base_value
    return copy.deepcopy(overlay_value)
  return copy.deepcopy(overlay_value)


def _merge_named_table_arrays(base_array: list[Any], overlay_array: list[Any]) -> None:
  name_to_index: dict[str, int] = {}
  for idx, entry in enumerate(base_array):
    name = entry.get("name")
    if isinstance(name, str):
      name_to_index[name] = idx

  for overlay_entry in overlay_array:
    name = overlay_entry.get("name")
    if not isinstance(name, str):
      continue
    if name in name_to_index:
      base_idx = name_to_index[name]
      base_array[base_idx] = _merge_values(base_array[base_idx], overlay_entry)
    else:
      base_array.append(copy.deepcopy(overlay_entry))
      name_to_index[name] = len(base_array) - 1


def _merge_tables(base: dict[str, Any], overlay: dict[str, Any]) -> None:
  for key, overlay_value in overlay.items():
    if key == INPUT_FILE_KEY:
      continue
    if key not in base:
      base[key] = copy.deepcopy(overlay_value)
      continue
    base[key] = _merge_values(base[key], overlay_value)


def _validate_roles(main_doc: dict[str, Any], template_doc: dict[str, Any] | None) -> None:
  main_role = main_doc.get(ROLE_KEY)
  if main_role is not None and main_role != ROLE_MAIN:
    raise ValueError(
      f"Main configuration has {ROLE_KEY}={main_role!r}; expected {ROLE_MAIN!r}."
    )

  if template_doc is None:
    return

  template_role = template_doc.get(ROLE_KEY)
  if template_role is not None and template_role != ROLE_TEMPLATE:
    raise ValueError(
      f"Template configuration has {ROLE_KEY}={template_role!r}; expected {ROLE_TEMPLATE!r}."
    )
  if INPUT_FILE_KEY in template_doc:
    raise ValueError(
      f"Template configuration must not define {INPUT_FILE_KEY!r}."
    )


def load_effective_config(main_path: Path) -> dict[str, Any]:
  main_doc = _parse_toml(main_path)
  template_doc = None

  input_file = main_doc.get(INPUT_FILE_KEY)
  if input_file is not None:
    if not isinstance(input_file, str):
      raise ValueError(f"{INPUT_FILE_KEY!r} must be a string in {main_path}")
    template_path = Path(input_file)
    if not template_path.is_absolute():
      template_path = main_path.parent / template_path
    template_doc = _parse_toml(template_path)

  _validate_roles(main_doc, template_doc)

  if template_doc is None:
    effective_doc = copy.deepcopy(main_doc)
  else:
    effective_doc = copy.deepcopy(template_doc)
    _merge_tables(effective_doc, main_doc)
  effective_doc.pop(INPUT_FILE_KEY, None)
  return effective_doc


def _get_modifiers(doc: dict[str, Any]) -> list[dict[str, Any]]:
  modifiers = doc.get("modifier")
  if modifiers is None:
    return []
  if not isinstance(modifiers, list):
    raise ValueError("'modifier' must be an array of tables.")
  for entry in modifiers:
    if not isinstance(entry, dict):
      raise ValueError("'modifier' entries must be tables.")
  return modifiers


def generate_mod_list(doc: dict[str, Any], include_groups: bool, out_f: TextIO) -> None:
  game_name = doc.get("game")
  if not isinstance(game_name, str) or not game_name:
    game_name = "Unknown Game"

  print(f"The following modifiers are available for {game_name}:\n", file=out_f)

  mod_list: dict[str, str] = {}
  group_list: dict[str, list[str]] = {}
  for mod in _get_modifiers(doc):
    name = mod.get("name")
    if not isinstance(name, str) or not name:
      continue
    if mod.get("unlisted", False):
      continue
    description = mod.get("description", "")
    mod_list[name] = str(description)
    if include_groups:
      groups: list[str] = []
      mod_type = mod.get("type")
      if isinstance(mod_type, str) and mod_type:
        groups.append(mod_type)
      extra_groups = mod.get("groups", [])
      if isinstance(extra_groups, list):
        for group_name in extra_groups:
          if isinstance(group_name, str) and group_name:
            groups.append(group_name)
      for group_name in groups:
        group_list.setdefault(group_name, []).append(name)

  for idx, name in enumerate(sorted(mod_list), start=1):
    print(f"{idx}. {name}: {mod_list[name]}", file=out_f)

  if include_groups:
    print("\n\nModifiers By Group:\n", file=out_f)
    for group_name in sorted(group_list):
      print(f"[{group_name}]:", file=out_f)
      for mod_name in group_list[group_name]:
        print(mod_name, file=out_f)
      print("", file=out_f)


def build_parser() -> argparse.ArgumentParser:
  parser = argparse.ArgumentParser(
    description=(
      "Generate a human-readable list of modifiers from a Chaos game configuration file."
    )
  )
  parser.add_argument(
    "infile",
    type=Path,
    help="Path to the main game TOML file.",
  )
  parser.add_argument(
    "outfile",
    nargs="?",
    default="-",
    help="Output file path. Use '-' for stdout (default).",
  )
  parser.add_argument(
    "-g",
    "--groups",
    action="store_true",
    help="Add a grouped modifier listing at the end.",
  )
  return parser


def main() -> int:
  args = build_parser().parse_args()

  try:
    effective_doc = load_effective_config(args.infile)
  except Exception as err:
    print(f"Error loading configuration: {err}", file=sys.stderr)
    return 1

  out_f: TextIO | None = None
  try:
    if args.outfile == "-":
      out_f = sys.stdout
      generate_mod_list(effective_doc, args.groups, out_f)
    else:
      with open(args.outfile, "w", encoding="utf-8") as out_f:
        generate_mod_list(effective_doc, args.groups, out_f)
  except Exception as err:
    print(f"Error writing modifier list: {err}", file=sys.stderr)
    return 1
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
