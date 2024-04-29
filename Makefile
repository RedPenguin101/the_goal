COMPILER = gcc
SOURCE_LIBS = -Ilib/
OSX_OPT = -Llib/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a
TARGET = ./bin/machine
CFILES = src/*.c
CFLAGS = -Wall -Wextra -std=c11 -pedantic

all: $(CFILES)
	$(COMPILER) $(CFILES) $(SOURCE_LIBS) $(CFLAGS) $(OSX_OPT) -o $(TARGET)

run: all
	$(TARGET)

clean:
	rm -f $(TARGET)
