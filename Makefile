src = $(wildcard src/*.c)

all: paxos

paxos: $(src)
	gcc -Wall -g -o paxos $(src)

clean:
	$(RM) paxos
