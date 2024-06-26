#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  int x;
  int y;
} Vector;

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

bool vec_equal(Vector a, Vector b);
Vector vec_move_towards(Vector current, Vector target);
Vector vec_move_random(Vector current, int die_size);
