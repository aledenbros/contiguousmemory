CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g

TARGET = test-contiguous
SOURCES = contiguous.c test-contiguous.c
OBJS = $(SOURCES:.c=.o)
HDRS = contiguous.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
	make clean
