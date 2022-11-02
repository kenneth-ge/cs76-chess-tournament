f1 = open("src/thc2.h", "r")
f2 = open("src/thc2.cpp", "r")
f3 = open("src/ChessAI.cpp", "r")

total = f1.read() + "\n" + f2.read() + "\n" + f3.read()

f = open("all_inline.cpp", "a")
f.write(total)
f.close()

import subprocess
subprocess.run(["g++", "all_inline.cpp"])