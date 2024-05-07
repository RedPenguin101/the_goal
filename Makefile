COMPILER = gcc
SOURCE_LIBS = -Ilib/
TARGET = ./bin/machine.exe
CFILES = src/main.c src/game.c src/vector.c
CFLAGS = -Wall -Wextra -std=c11 -pedantic

RAYLIB_DIR = raylib
RAYLIB_WIN = -L$(RAYLIB_DIR)/lib -lraylib -lgdi32 -lwinmm
RAYLIB_INC = -I$(RAYLIB_DIR)/include
RAYLIB_OSX = -L$(RAYLIB_DIR)/lib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL $(RAYLIB_DIR)/lib/libraylib.a

all: $(CFILES)
	$(COMPILER) $(CFLAGS) -o $(TARGET) $(RAYLIB_INC) $(CFILES) $(RAYLIB_OSX) 

run: all
	$(TARGET)

clean:
	rm -f $(TARGET)
