COMPILER = gcc
SOURCE_LIBS = -Ilib/
OSX_OPT = -Llib/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a
TARGET = ./bin/machine.exe
CFILES = src/main.c src/game.c src/vector.c
CFLAGS = -Wall -Wextra -std=c11 -pedantic

RAYLIB_DIR = raylib
RAYLIB_WIN = -L$(RAYLIB_DIR)/lib -lraylib -lgdi32 -lwinmm
RAYLIB_INC = -I$(RAYLIB_DIR)/include

all: $(CFILES)
	$(COMPILER) $(CFLAGS) -o $(TARGET) $(RAYLIB_INC) $(CFILES) $(RAYLIB_WIN) 

run: all
	$(TARGET)

clean:
	rm -f $(TARGET)
