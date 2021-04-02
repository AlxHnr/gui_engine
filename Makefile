CFLAGS  = `sdl-config --cflags`
LDFLAGS = `sdl-config --libs` -lSDL_gfx

.PHONY: all run clean
all: example

example: $(wildcard *.c)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
run: example
	./example
clean:
	- rm example
