#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  This is a utility program to generate a list of possible mods from the TOML configuration file.
  Its output can be put in a convenient place online and you can specify the link in the interface
  so that the chatbot can direct viewers to a list of possible mods.
"""
import sys
import argparse
import tomlkit
from itertools import filterfalse

parser = argparse.ArgumentParser(description='Generate a human-readable list of modifiers from a Chaos configuration file')
parser.add_argument("infile", nargs='?', type=argparse.FileType('r'), default=sys.stdin)
parser.add_argument("outfile", nargs='?', type=argparse.FileType('w'), default=sys.stdin)
parser.add_argument("-g", "--groups", action='store_true', help='Add a list of modules by group to the end')

def unique_everseen(iterable, key=None):
  "List unique elements, preserving order. Remember all elements ever seen."
  seen = set()
  seen_add = seen.add
  if key is None:
    for element in filterfalse(seen.__contains__, iterable):
      seen_add(element)
      yield element
  else:
    for element in iterable:
      k = key(element)
      if k not in seen:
        seen_add(k)
        yield element

if __name__ == '__main__':
  try:
    args = parser.parse_args()
    doc = tomlkit.load(args.infile)
  except Exception as err:
    print(err)
    sys.exit()
  
  if 'game' not in doc:
    print('This does not appear to be a Chaos game-configuration file. Game name not found.')
    sys.exit()

  print(f"The following modifiers are available for {doc['game']}:\n", file=args.outfile)
  mod_list = {}
  group_list = {}
  for mod in doc['modifier']:
    mod_list[mod['name']] = mod['description']
    if args.groups:
      this_mods_groups = [mod['type']]
      if 'groups' in mod:
        for g in mod['groups']:
          this_mods_groups.append(g)
      for g in this_mods_groups:
        if g not in group_list.keys():
          group_list[g] = []
        group_list[g].append(mod['name'])

  i = 1
  for name in sorted(mod_list):
    print(f"{i}. {name}: {mod_list[name]}", file=args.outfile)
    i += 1


  if args.groups:
    print(f"\n\nModifiers By Group:\n", file=args.outfile)
    for group in sorted(group_list):
      print(f"[{group}]:", file=args.outfile)
      for mod in group_list[group]:
        print(f"{mod}", file=args.outfile)
      print("\n", file=args.outfile)
