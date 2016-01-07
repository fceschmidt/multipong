CC = gcc
CFLAGS = -c -Wall
CRELEASEFLAGS = $(CFLAGS) -O2
CDEBUGFLAGS = $(CFLAGS) -DDEBUG -g
LD = $(CC)
LDFLAGS = -lGL -lGLU

RELEASETARGET = ../build/release/multipong
DEBUGTARGET = ../build/debug/multipong

RELEASEOBJECTS = $(patsubst %.c, ../build/release/%.o, $(wildcard *.c))
DEBUGOBJECTS = $(patsubst %.c, ../build/debug/%.o, $(wildcard *.c))

debug: $(DEBUGTARGET)

release: $(RELEASETARGET)

$(RELEASETARGET): $(RELEASEOBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

$(DEBUGTARGET): $(DEBUGOBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

../build/release/%.o: %.c
	$(CC) $(CRELEASEFLAGS) $^ -o $@

../build/debug/%.o: %.c
	$(CC) $(CDEBUGFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

execute_release:
	../build/release/multipong

execute_debug:
	../build/debug/multipong
