.phony all:
all: acs

acs: acs.c
	gcc -Wall acs.c -pthread -lpthread -o ACS

.PHONY clean:
clean:
	-rm -rf *.o *.exe ACS