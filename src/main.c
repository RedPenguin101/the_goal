#include "game.h"
#include "raylib.h"
#include <stdbool.h>

#define MAX_X 15
#define MAX_Y 15
#define SQUARE_SIZE 30
#define SCREEN_WIDTH (MAX_X * SQUARE_SIZE)
#define SCREEN_HEIGHT (MAX_Y * SQUARE_SIZE)
#define TEXT_SIZE 15

#define FRAMES_PER_ROW 16
#define FRAMES_PER_COL 16
#define FRAME_WORKER 1
#define FRAME_MACHINE (13 * 16) + 1
#define FRAME_STOCKPILE (11 * 16)

long turn = 0;

bool quit = false;

Vector2 frame_to_row_col(int frame, int frames_per_row) {
  Vector2 v = {frame % frames_per_row, frame / frames_per_row};
  return v;
}

void draw_frame_in_square(int frame, int row, int col, const Texture2D *tex) {
  int f_row = frame % FRAMES_PER_ROW;
  int f_col = frame / FRAMES_PER_COL;

  int frame_width = tex->width / FRAMES_PER_ROW;
  int frame_height = tex->height / FRAMES_PER_COL;
  Rectangle dest_rec = {row * SQUARE_SIZE, col * SQUARE_SIZE, SQUARE_SIZE,
                        SQUARE_SIZE};
  Rectangle source_rec = {(float)(frame_width * f_row),
                          (float)(frame_height * f_col), (float)frame_width,
                          (float)frame_height};
  DrawTexturePro(*tex, source_rec, dest_rec, (Vector2){0, 0}, 0, BLUE);
}

void draw_game_state(GameState *gs, const Texture2D *tex) {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  for (int i = 0; i < gs->c_machines; i++) {
    Machine w = gs->machines[i];
    for (int i = 0; i < w.size.x; i++) {
      for (int j = 0; j < w.size.y; j++) {
        draw_frame_in_square(FRAME_MACHINE, w.location.x + i, w.location.y + j,
                             tex);
      }
    }
  }

  for (int i = 0; i < gs->c_stockpile; i++) {
    Stockpile w = gs->stockpiles[i];
    int start_x = w.location.x;
    int start_y = w.location.y;

    for (int i = 0; i < w.size.x; i++) {
      for (int j = 0; j < w.size.y; j++) {
        draw_frame_in_square(FRAME_STOCKPILE, w.location.x + i,
                             w.location.y + j, tex);
      }
    }

    for (int j = 0; j < w.things_in_stockpile; j++) {
      DrawCircleV(
          (Vector2){SQUARE_SIZE * (start_x + j) + ((float)SQUARE_SIZE / 2),
                    SQUARE_SIZE * start_y + ((float)SQUARE_SIZE / 2)},
          ((float)SQUARE_SIZE / 2), RED);
      char count[5] = {0};
      sprintf(count, "%d", w.content_count[j]);
      DrawText(count, SQUARE_SIZE * (start_x + j) + 10,
               SQUARE_SIZE * (start_y) + 5, 20, BLACK);
    }
  }

  for (int i = 0; i < gs->c_workers; i++) {
    Worker w = gs->workers[i];
    draw_frame_in_square(FRAME_WORKER, w.location.x, w.location.y, tex);
  }

  EndDrawing();
}

int main(void) {
  srand(time(0));
  GameState *gs = new_game();

  int in = add_stockpile(2, 2, 2, 2);
  int out = add_stockpile(2, 6, 3, 3);
  add_material_to_stockpile(in, WASHED_IRON_WIRE_COIL, 1);
  add_material_to_stockpile(in, EMPTY_SPINDLE, 1);

  int winder = add_machine(WIRE_WINDER, "WireWind", 2, 4);
  add_output_stockpile_to_machine(winder, out);
  add_input_stockpile_to_machine(winder, in);

  out = add_stockpile(11, 10, 3, 3);
  in = add_stockpile(7, 10, 2, 2);
  // add_material_to_stockpile(in, SPINDLED_WIRE_COIL, 5);

  int puller = add_machine(WIRE_PULLER, "WirePull1", 9, 10);
  add_output_stockpile_to_machine(puller, out);
  add_input_stockpile_to_machine(puller, in);

  add_worker();
  assign_machine_production_job(winder, WIND_WIRE);
  // assign_machine_production_job(puller, PULL_WIRE);

  InitWindow(SCREEN_WIDTH * 2, SCREEN_HEIGHT, "THE_GOAL");
  Texture2D ascii = LoadTexture("assets/Anno_16x16.png");
  SetTargetFPS(5);

  while (!WindowShouldClose() && !quit) {
    draw_game_state(gs, &ascii);
    tick_game();
    turn++;
  }

  CloseWindow();
  UnloadTexture(ascii);

  return 0;
}
