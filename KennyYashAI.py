'''
Written and modified by Kenneth Ge
November 15 2022
'''

import chess
import math
import random
from subprocess import Popen, PIPE, STDOUT

class KennyYashAI():
    def __init__(self, white):
        self.white = white
        self.b = Popen(["./a"], stdout=PIPE, stdin=PIPE, stderr=PIPE)
        self.num_pos = -1
        if white:
            self.b.stdin.write(b'w\r\n')
        else:
            self.b.stdin.write(b'b\r\n')

        self.did_move = False
        self.restart = False
        self.need_reset = False

    # max_val for white, min_val for black
    """
    time is a tuple which contains time left of white and black in seconds
    (whiteTimeLeft, blackTimeLeft)
    """
    def choose_move(self, board, time) -> chess.Move:
        if board.fullmove_number <= 1:
            self.total_time = time
            if self.white:
                self.b.stdin.write(bytes(str(time[0]) + '\r\n', encoding='ascii'))
            else:
                self.b.stdin.write(bytes(str(time[1]) + '\r\n', encoding='ascii'))


        if self.restart or (self.white and not self.did_move):
            if self.restart:
                #print('new input fen', board.fen())
                self.b.stdin.write(bytes(board.fen() + '\r\n', encoding='ascii'))
                self.restart = False
            else:
                self.b.stdin.write(b'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\r\n')
                self.did_move = True
        else:
            latest_move = board.peek()
            self.b.stdin.write(bytes(latest_move.uci() + '\r\n', encoding='ascii'))
            if self.white:
                self.b.stdin.write(bytes(str(time[0]) + '\r\n', encoding='ascii'))
            else:
                self.b.stdin.write(bytes(str(time[1]) + '\r\n', encoding='ascii'))
        self.b.stdin.flush()
        line = self.b.stdout.readline().decode(encoding='ascii')

        move = chess.Move.from_uci(line.strip())

        if not board.is_legal(move) or self.need_reset:
            #print('-------')
            #print(board.fen())
            #print(move)
            #print('restart')
            self.b.kill()
            self.b = Popen(["./a"], stdout=PIPE, stdin=PIPE, stderr=PIPE)
            self.b.stdin.write(b'p\r\n')
            if self.white:
                self.b.stdin.write(bytes(str(self.total_time[0]) + '\r\n', encoding='ascii'))
            else:
                self.b.stdin.write(bytes(str(self.total_time[1]) + '\r\n', encoding='ascii'))

            #print('this is what i"m writing')
            self.b.stdin.write(bytes(str(len(board.move_stack)) + '\r\n', encoding='ascii'))
            #print(len(board.move_stack))
            for move in board.move_stack:
                self.b.stdin.write(bytes(move.uci() + '\r\n', encoding='ascii'))
                #print(move.uci())
            #print('end what i"m writing')

            self.b.stdin.flush()
            line = self.b.stdout.readline().decode(encoding='ascii')

            move = chess.Move.from_uci(line.strip())

            #print(line.strip())
            #print('new move after restarting', move)

            # fen fallback
            if not board.is_legal(move):
                self.need_reset = True
                self.b.kill()
                self.b = Popen(["./a"], stdout=PIPE, stdin=PIPE, stderr=PIPE)
                self.b.stdin.write(b'w\r\n')
                self.b.stdin.write(bytes(str(self.total_time[0]) + '\r\n', encoding='ascii'))
                self.b.stdin.write(bytes(board.fen() + '\r\n', encoding='ascii'))
                self.b.stdin.flush()

                line = self.b.stdout.readline().decode(encoding='ascii')

                move = chess.Move.from_uci(line.strip())

                if board.is_legal(move):
                    self.need_reset = False
                    return move

                self.need_reset = True

                # random move fallback
                return random.choice(list(board.legal_moves))

        return move