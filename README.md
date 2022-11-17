# cs76-chess-tournament
The winning submission for the CS 76 chess AI tournament at Dartmouth College!

It plays around 1550 ELO Lichess, and can beat human players with an even higher rating (because chess bots play weird :shrug:)

## Techniques
* Alphabeta pruning
* Move ordering
* Quiescence search -- moves with more action get evaluated to a higher depth
* Transposition table -- simple string board hashing, nothing fancy like Zobrist
* Improved evaluation function that takes into account king safety, piece mobility, and gives a bonus for castling (in the early game) and check
* Various other improvements and micro-optimizations in the implementation of our code
