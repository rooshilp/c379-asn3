# Rooshil Patel
# 1255318
# Assignment 3
# CMPUT 379

asn2: asn3.c
	cc asn3.c -lcurses -lpthread -o asn3

clean:
	rm -f *.o asn3 core
