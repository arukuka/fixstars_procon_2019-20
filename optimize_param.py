#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from argparse import ArgumentParser
from pathlib import Path
import json
import optuna
import round_robin
import tempfile
import shutil

def objective(trial, controller, entries_dir, hash):
    card_num = trial.suggest_int('cards_num', 0, 1000000)
    random = trial.suggest_int('random', 0, 1000000)
    weakness = [trial.suggest_int('w' + str(i), 0, 1000000) for i in range(10)]

    config = {
      'cards_num': card_num,
      'random': random,
      'weakness': weakness
    }

    with tempfile.TemporaryDirectory() as dname:
      temp_dir = Path(dname)
      temp_entries_dir = temp_dir / Path("./entries")
      shutil.copytree(entries_dir, temp_entries_dir)
      with open(temp_dir / Path("param.json"), mode='w') as f:
        f.write(json.dumps(config))
      results = round_robin.round_robin(controller=controller, padding=4, directory=temp_entries_dir.resolve())
      # score = results[hash]['score']
      score = -(results["stock"] + results["max_cuts"] * 3 * 5) + results["max_cuts"] * 0.001

      print("stock = {}, max_cuts = {}, config = {}".format(results["stock"], results["max_cuts"], config))
      return score

def main():
    parser = ArgumentParser()
    parser.add_argument("target", type=Path, help="target binary file")
    parser.add_argument("--study")
    parser.add_argument("--storage")

    args = parser.parse_args()
    assert args.target.is_file()

    entry = round_robin.hash(args.target)

    if args.study and args.storage:
      print("LOAD STUDY")
      study = optuna.load_study(study_name=args.study, storage=args.storage)
    else:
      print("CREATE STUDY")
      study = optuna.create_study()

    controller = Path('./prime_daihinmin.py').resolve()
    entries = Path('./opt_param').resolve()

    study.optimize(lambda trial: objective(trial, controller, entries, entry), n_jobs=-1)

if __name__ == "__main__":
    main()
