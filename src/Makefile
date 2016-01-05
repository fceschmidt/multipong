CC = gcc
CFLAGS = -c -Wall
LD = $(CC)
LDFLAGS = -lGL -lGLU

TARGET = ../build/multipong

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

debug: CFLAGS += -DDEBUG -g

release: CFLAGS += -O2
release: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ ../build/$^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o ../build/$@

clean:
	rm -f $(TARGET) $(OBJECTS)
