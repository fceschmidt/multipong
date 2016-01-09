CC = gcc
CFLAGS = -c -Wall `sdl2-config --cflags`
CRELEASEFLAGS = $(CFLAGS) -O2
CDEBUGFLAGS = $(CFLAGS) -DDEBUG -g
LD = $(CC)
LDFLAGS = -lGL -lGLU `sdl2-config --libs` -lSDL2_net -lSDL2_image -lSDL2_ttf

RELEASETARGET = ../build/release/multipong
DEBUGTARGET = ../build/debug/multipong

RELEASEOBJECTS = $(patsubst %.c, ../build/release/%.o, $(shell find . -name "*.c"))
DEBUGOBJECTS = $(patsubst %.c, ../build/debug/%.o, $(shell find . -name "*.c"))

.PHONY: debug
debug: $(DEBUGTARGET)

.PHONY: release
release: $(RELEASETARGET)

$(RELEASETARGET): $(RELEASEOBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

$(DEBUGTARGET): $(DEBUGOBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

../build/release/%.o: %.c
	$(CC) $(CRELEASEFLAGS) $^ -o $@

../build/debug/%.o: %.c
	$(CC) $(CDEBUGFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(RELEASETARGET) $(DEBUGTARGET) $(RELEASEOBJECTS) $(DEBUGOBJECTS)

.PHONY: execute_release
execute_release:
	../build/release/multipong

.PHONY: execute_debug
execute_debug:
	../build/debug/multipong
