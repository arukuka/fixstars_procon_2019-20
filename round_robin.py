#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import threading
import random
from argparse import ArgumentParser
from pathlib import Path
import hashlib
import itertools
from pprint import pprint

def hash(entry):
  h = hashlib.sha256()
  l = h.block_size * 0x800

  with open(entry, 'rb') as f:
    b = f.read(l)

    while b:
      h.update(b)
      b = f.read(l)

  return h.hexdigest()

def cal_num_match(n, r, i = 0):
  if i >= r:
    return 1
  else:
    return n * cal_num_match(n - 1, r, i + 1)

def parse_result(stdout):
  ret = []
  for line in stdout.splitlines()[-5:-1]:
    arr = line.split()
    sub = dict(score=int(arr[0]), name=arr[2], remain=int(arr[3]))
    sub["err"] = sub["remain"] == 3001
    ret = ret + [sub]
  return ret

def round_robin(args):
  hash2entry = {hash(e): e for e in args.directory.iterdir()}
  pprint(hash2entry)
  entries = list(hash2entry.keys())
  if args.padding:
    entries = entries * 3
  entries.sort()
  pprint(entries)

  matches = set(itertools.permutations(entries, 4))
  num_match = len(matches)

  results = dict()
  for i in range(args.iter):
    print("[{} / {}]".format(i, args.iter))
    index = 0
    for battle in matches:
      print("\t[{} / {}] {}".format(index, num_match, tuple([hash2entry[x].name for x in list(battle)])))
      index = index + 1
      command = ["python", str(args.controller)] + list(itertools.chain.from_iterable([[x, str(hash2entry[x])] for x in list(battle)]))
      process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
      for result in parse_result(process.stdout.decode("utf-8")):
        if result["name"] in results:
          sub = results[result["name"]]
          sub["score"] = sub["score"] + result["score"]
          sub["remain"] = sub["remain"] + result["remain"]
          sub["err"] = sub["err"] or result["err"]
          results[result["name"]] = sub
        else:
          result["exe"] = hash2entry[result["name"]].name
          results[result["name"]] = result
      pprint(results)
  pprint(results)

def main():
    parser = ArgumentParser(usage="Usage: %prog [options]")
    parser.add_argument("-d", "--dir", type=Path, dest="directory", default=Path('./entries'))
    parser.add_argument("-p", "--padding",  action="store_true", dest="padding", default=False)
    parser.add_argument("-s", "--seed", action="store_true", dest="debug", default=False)
    parser.add_argument("-c", "--controller", type=Path, dest="controller", default=Path('./prime_daihinmin.py'))
    parser.add_argument("-n", "--iter", type=int, dest="iter", default=100)

    args = parser.parse_args()
    assert args.controller.is_file()

    round_robin(args)

if __name__ == "__main__":
    main()