Simple computer boop generator
==============================

In the movies, robots and computers always make boopy noises.
The computer or robot on my desk was not doing that. Until I 
added this command.

"boop" will make a robot-ey boopy noise, based on some random 
phase distortion parameters. A few command line options make 
for a bit of control in the level of stress and duration of 
the boop.

usage: boop [len [num [hw:#,3]]]
length: 0.01 to 10  (length in seconds of each piece)
num: 1 to 10        (number of pieces in a boop)

Try:
./boop 0.2 1    # single burst
./boop 0.08 2   # fast
./boop 0.12 5   # acknowledgement
./boop 2 10    # wake the neighbors
