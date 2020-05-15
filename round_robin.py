#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import threading
import random
import sys
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

def parse_result(stdout, stderr, info):
  ret = []
  try:
    lines = stdout.splitlines()
    for line in lines[-6:-2]:
      arr = line.split()
      sub = dict(score=int(arr[0]), name=arr[2], remain=int(arr[3]))
      sub["err"] = sub["remain"] == 3001
      ret = ret + [sub]
    ret = {"players": ret}
    ret["stock"] = int(lines[-2].split()[-1])
    ret["max_cuts"] = int(lines[-1].split()[-1])
  except:
    import traceback
    traceback.print_exc()
    print(stderr)
    print(info)
  return ret

def round_robin(directory = Path('./entries'), padding = False, seed = False, controller = Path('./prime_daihinmin.py'), iter = 100):
  hash2entry = {hash(e): e for e in directory.iterdir()}
  # pprint(hash2entry)
  entries = list(hash2entry.keys())
  entries = entries * padding
  entries.sort()
  # pprint(entries)

  matches = list(set(itertools.permutations(entries, 4)))
  matches.sort()
  num_match = len(matches)

  seed = 0

  results = dict()
  results["stock"] = 0
  results["max_cuts"] = 0
  for i in range(iter):
    # print("[{} / {}]".format(i, iter))
    index = 0
    for battle in matches:
      battlers = list(battle)
      # print("\t[{} / {}] {}".format(index, num_match, tuple([hash2entry[x].name for x in battlers])))
      index = index + 1
      command = ["python", str(controller)] \
          + list(itertools.chain.from_iterable([[str(i), str(hash2entry[x]) + ' ' + str(seed)] for i, x in zip(range(len(battlers)), battlers)])) \
          + ["--seed", str(seed), "--show"]
      # print(command, file=sys.stderr)
      process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      stdout = process.stdout.decode("utf-8")
      stderr = process.stderr.decode("utf-8")
      one_game_results = parse_result(stdout, stderr, {"cmd": command, "seed": seed})
      for result in one_game_results["players"]:
        name = battlers[int(result["name"])]
        del result["name"]
        if name in results:
          sub = results[name]
          sub["score"] = sub["score"] + result["score"]
          sub["remain"] = sub["remain"] + result["remain"]
          sub["err"] = sub["err"] or result["err"]
          results[name] = sub
        else:
          result["exe"] = hash2entry[name].name
          results[name] = result
      results["stock"] = results["stock"] + one_game_results["stock"]
      results["max_cuts"] = results["max_cuts"] + one_game_results["max_cuts"]
      # pprint(results)
      seed = seed + 1
  # pprint(results)

  return results

def main():
    parser = ArgumentParser(usage="Usage: %prog [options]")
    parser.add_argument("-d", "--dir", type=Path, dest="directory", default=Path('./entries'))
    parser.add_argument("-p", "--padding",  type=int, dest="padding", default=1)
    parser.add_argument("-s", "--seed", action="store_true", dest="seed", default=False)
    parser.add_argument("-c", "--controller", type=Path, dest="controller", default=Path('./prime_daihinmin.py'))
    parser.add_argument("-n", "--iter", type=int, dest="iter", default=100)

    args = parser.parse_args()
    assert args.controller.is_file()

    round_robin(directory=args.directory, padding=args.padding, seed=args.seed, controller=args.controller, iter=args.iter)

if __name__ == "__main__":
    main()
