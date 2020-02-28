#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from argparse import ArgumentParser
from pathlib import Path
import json
import optuna
import round_robin

def objective(trial, hash):
    card_num = trial.suggest_int('cards_num', 0, 1000000)
    random = trial.suggest_int('random', 0, 1000000)
    weakness = [trial.suggest_int('w' + str(i), 0, 1000000) for i in range(10)]

    config = {
      'cards_num': card_num,
      'random': random,
      'weakness': weakness
    }

    with open("param.json", mode='w') as f:
      f.write(json.dumps(config))

    results = round_robin.round_robin()
    score = results[hash]['score']

    print("score = {}, config = {}".format(score, config))
    return score

def main():
    parser = ArgumentParser(usage="Usage: %prog [options]")
    parser.add_argument("target", type=Path, help="target binary file")
    parser.add_argument("--study")
    parser.add_argument("--storage")

    args = parser.parse_args()
    assert args.target.is_file()

    entry = round_robin.hash(args.target)

    study = optuna.create_study()
    study.optimize(lambda trial: objective(trial, entry), n_trials=100)

if __name__ == "__main__":
    main()
