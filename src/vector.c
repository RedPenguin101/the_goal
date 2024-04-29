#include "vector.h"

/* void vec_print(Vector a) { */
/*   printf("[%d, %d]", a.x, a.y); */
/* } */

bool vec_equal(Vector a, Vector b) {
  //printf("eq A: ");
  //vec_print(a);
  //printf(" B: ");
  //vec_print(b);
  //printf(" %d", (a.x == b.x));
  //printf("a.y %d, b.y %d", a.y, b.y);
  //printf("%d", (a.y == b.y));
  //printf("%d", ((a.x == b.x) && (a.y == b.y)));
  //printf("\n");
  
  return ((a.x == b.x) && (a.y == b.y));
}


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
  }
}

Vector vec_move_towards(Vector current, Vector target) {
  int dx = target.x - current.x;
  int dy = target.y - current.y;

  //printf("dx: %d, dy: %d\n", dx, dy);

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
