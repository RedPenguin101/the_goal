#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
  int x;
  int y;
} Vector;

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

bool vec_equal(Vector a, Vector b);
Vector vec_move_towards(Vector current, Vector target);
