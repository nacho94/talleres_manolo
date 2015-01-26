
EXE = talleres-manolo
SRCS = main.c

all:
	$(CC) $(SRCS) -o $(EXE) -lpthread

run: all
	./$(EXE)

.PHONY: clean

clean:
	rm -f *.o $(EXE)
