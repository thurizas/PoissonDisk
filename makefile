CC=g++
CCFLAGS=-std=c++20 -pedantic -Wall -Wextra
CCINCS=-I/opt/lib/SFML/sfml-2.5.1/include

LK=g++
LKFLAGS=-Wl,-rpath=/opt/lib/SFML/sfml-2.5.1/lib
LIBS=-L/opt/lib/SFML/sfml-2.5.1/lib -lsfml-graphics -lsfml-system -lsfml-window

PROJ=poissonTest

OBJS=logger.o main.o poissonDisk.o


$(PROJ) : $(OBJS)
	$(LK) $(LKFLAGS) $(OBJS) $(LIBS) -o $(PROJ)

all : clean $(PROJ)

main.o : main.cpp makefile
	$(CC) -c $(CCFLAGS) $(CCINCS) main.cpp -o main.o

poissonDisk.o : poissonDisk.cpp poissonDisk.h makefile
	$(CC) -c $(CCFLAGS) $(CCINCS) poissonDisk.cpp -o poissonDisk.o

logger.o : logger.cpp logger.h makefile
	$(CC) -c $(CCFLAGS) $(CCINCS) logger.cpp -o logger.o

clean:
	rm -f *.o *.*~ $(PROG)
