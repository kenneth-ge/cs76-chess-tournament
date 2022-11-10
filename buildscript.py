f1 = open("thc2.h", "r")
f2 = open("thc2.cpp", "r")
f3 = open("src/ChessAI.cpp", "r")

total = f1.read() + "\n" + f2.read() + "\n" + f3.read()

total = total.replace('#include "chess/thc.cpp"', '')

f = open("all_inline.cpp", "w")
f.write(total)
f.close()

import subprocess
subprocess.run(["g++", "all_inline.cpp", "-O3"])
subprocess.run(["./a"])
subprocess.run(["./a.out"])