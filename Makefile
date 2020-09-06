CC := gcc

SRC := octree.c 
OBJ := octree.o


FLAGS := --std=c99 -Wall -lm
FLAGS += -g

TARGET := liboctree.a


$(TARGET): $(OBJS)
	ar rsc $@ $^


%.o: %.c
	$(CC) -o $@ -c $< $(FLAGS)


.PHONY: clean


clean:
	rm -fv $(OBJS) $(TARGET)
