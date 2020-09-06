CC := gcc

IDIR := ./

SRC := octree.c 
OBJ := octree.o


FLAGS := --std=c99 -Wall -I$(IDIR) -lm -g

TARGET := liboctree.a


$(TARGET): $(OBJ)
	ar rsc $@ $^


%.o: %.c
	$(CC) -o $@ -c $< $(FLAGS)


.PHONY: clean


clean:
	rm -fv $(OBJS) $(TARGET)
