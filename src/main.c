#include "game.h"
#include <raylib.h>
#include <stdbool.h>

#define MAX_X 15
#define MAX_Y 15
#define SQUARE_SIZE 30
#define SCREEN_WIDTH (MAX_X * SQUARE_SIZE)
#define SCREEN_HEIGHT (MAX_Y * SQUARE_SIZE)
#define TEXT_SIZE 15

long turn = 0;

bool quit = false;

void draw_game_state(GameState *gs) {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  for (int i = 0; i <= MAX_X; i++) {
    DrawLineV((Vector2){SQUARE_SIZE * i, 0},
              (Vector2){SQUARE_SIZE * i, SCREEN_HEIGHT}, LIGHTGRAY);
  }

  for (int i = 0; i <= MAX_Y; i++) {
    DrawLineV((Vector2){0, SQUARE_SIZE * i},
              (Vector2){SCREEN_WIDTH, SQUARE_SIZE * i}, LIGHTGRAY);
  }

  for (int i = 0; i < gs->c_machines; i++) {
    Machine w = gs->machines[i];
    DrawRectangle(SQUARE_SIZE * w.location.x, SQUARE_SIZE * w.location.y,
                  SQUARE_SIZE * w.size.x, SQUARE_SIZE * w.size.y, RED);
  }

  for (int i = 0; i < gs->c_stockpile; i++) {
    Stockpile w = gs->stockpiles[i];
    int start_x = w.location.x;
    int start_y = w.location.y;
    DrawRectangle(SQUARE_SIZE * start_x, SQUARE_SIZE * start_y,
                  SQUARE_SIZE * w.size.x, SQUARE_SIZE * w.size.y, GREEN);
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
    DrawRectangle(SQUARE_SIZE * w.location.x, SQUARE_SIZE * w.location.y,
                  SQUARE_SIZE, SQUARE_SIZE, BLUE);
  }

  for (int i = 0; i < MESSAGE_BUFFER_SIZE; i++) {
    DrawText(gs->message_buffer[i], MAX_X * SQUARE_SIZE + 20,
             20 + (i * TEXT_SIZE), TEXT_SIZE, BLACK);
  }

  EndDrawing();
}

int main(void) {
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
  //assign_machine_production_job(puller, PULL_WIRE);

#define UI 0

  if (UI) {
    InitWindow(SCREEN_WIDTH * 2, SCREEN_HEIGHT, "THE_GOAL");
    SetTargetFPS(5);
  }

  // while (!WindowShouldClose() && !quit) {
  while (!quit && turn < 50) {
    if (UI) {
      draw_game_state(gs);
    }
    tick_game();
    turn++;
  }

  if (UI)
    CloseWindow();

  return 0;
}
