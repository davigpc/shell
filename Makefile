CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lreadline
TARGET = shell

all: $(TARGET)

$(TARGET): shell.c
	$(CC) $(CFLAGS) shell.c -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
