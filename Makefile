
boop:	boop.cpp
	g++ -std=gnu++11 -g -o boop boop.cpp -pipe -Wall -Werror -lasound

clean:
	rm -f *.o core *~ boop
