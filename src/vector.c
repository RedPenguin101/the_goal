#include "vector.h"

/* void vec_print(Vector a) { */
/*   printf("[%d, %d]", a.x, a.y); */
/* } */

bool vec_equal(Vector a, Vector b) { return ((a.x == b.x) && (a.y == b.y)); }

Vector vec_move(Vector v, Direction d) {
  switch (d) {
  case UP:
    return (Vector){v.x, v.y - 1};
  case DOWN:
    return (Vector){v.x, v.y + 1};
  case LEFT:
    return (Vector){v.x - 1, v.y};
  case RIGHT:
    return (Vector){v.x + 1, v.y};
  default:
    printf("Unknown direction %d\n", d);
    exit(1);
  }
}

Vector vec_move_towards(Vector current, Vector target) {
  int dx = target.x - current.x;
  int dy = target.y - current.y;

  // printf("dx: %d, dy: %d\n", dx, dy);

  if (abs(dx) >= abs(dy)) {
    if (dx > 0)
      return vec_move(current, RIGHT);
    else
      return vec_move(current, LEFT);
  } else if (dy > 0)
    return vec_move(current, DOWN);
  else
    return vec_move(current, UP);
}

int rand_between(int lower, int upper) {
  return (rand() % (upper - lower + 1)) + lower;
}

Vector vec_move_random(Vector current, int die_size) {
  int roll = rand_between(0, die_size);
  if (roll < 3)
    return vec_move(current, roll);
  else
    return current;
}
