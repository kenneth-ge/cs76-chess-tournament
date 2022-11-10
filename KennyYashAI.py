'''
Written and modified by Kenneth Ge
October 24 2022
'''

import chess
import math
import random
import subprocess

class AlphaBetaAI():
    # same as MinimaxAI -- see that for additional comments
    def __init__(self, max_depth, color):
        self.b = subprocess.Popen(["a.exe"])

    # max_val for white, min_val for black
    def choose_move(self, board) -> chess.Move:
        latest_move = board.peek()
        self.b.stdin.write(latest_move.uci() + '\n')
        line = str(self.b.stdout.readline())
        return chess.Move.from_uci(line.strip())