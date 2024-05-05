#include "game.h"
#include "raylib.h"
#include <stdbool.h>

#define MAX_X 15
#define MAX_Y 15
#define SQUARE_SIZE 30
#define MENU_SIZE_SQUARES 30
#define SCREEN_WIDTH ((MAX_X + MENU_SIZE_SQUARES) * SQUARE_SIZE)
#define SCREEN_HEIGHT (MAX_Y * SQUARE_SIZE)
#define TEXT_SIZE 15

#define FRAMES_PER_ROW 16
#define FRAMES_PER_COL 16
#define FRAME_WORKER 1
#define FRAME_MACHINE (13 * 16) + 1
#define FRAME_STOCKPILE (11 * 16) + 0
#define FRAME_CURSOR (5 * 16) + 8
#define FRAME_MATERIAL (1 * 16) + 14

#define FPS 60
#define TPS 5

bool quit = false;
bool paused = false;

struct DrawState {
  GameState *gs;
  char *context_menu_text;
  Texture2D *tilemap;
  Font *font;
} draw_state;

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

void draw_game_state(struct DrawState *ds) {
  GameState *gs = ds->gs;
  Texture2D *tex = ds->tilemap;
  Font *font = ds->font;
  int font_size = font->baseSize * 2.0;
  char *text_buffer = ds->context_menu_text;
  BeginDrawing();
  ClearBackground(RAYWHITE);

  // Draw machines
  for (int i = 0; i < gs->c_machines; i++) {
    Machine w = gs->machines[i];
    for (int i = 0; i < w.size.x; i++) {
      for (int j = 0; j < w.size.y; j++) {
        draw_frame_in_square(FRAME_MACHINE, w.location.x + i, w.location.y + j,
                             tex);
      }
    }
  }

  // Draw stockpiles
  for (int i = 0; i < gs->c_stockpile; i++) {
    Stockpile w = gs->stockpiles[i];
    int start_x = w.location.x;
    int start_y = w.location.y;

    for (int i = 0; i < w.size.x; i++) {
      for (int j = 0; j < w.size.y; j++) {
        draw_frame_in_square(FRAME_STOCKPILE, start_x + i, start_y + j, tex);
      }
    }

    for (int j = 0; j < w.things_in_stockpile; j++) {

      if (w.content_count[j] > 0) {

        draw_frame_in_square(FRAME_MATERIAL, (start_x + j), (start_y), tex);
      }
    }
  }

  // Draw workers
  for (int i = 0; i < gs->c_workers; i++) {
    Worker w = gs->workers[i];
    draw_frame_in_square(FRAME_WORKER, w.location.x, w.location.y, tex);
  }

  DrawLine(SQUARE_SIZE * (MAX_X + 1), 0, SQUARE_SIZE * (MAX_X + 1),
           SCREEN_HEIGHT, GRAY);

  // Draw context menu
  ObjectReference o = object_under_point(gs->cursor.x, gs->cursor.y);

  switch (o.object_type) {
  case O_NOTHING: {
    break;
  }
  case O_WORKER: {
    // printf("DEBUG: Worker under cursor\n");
    Worker *w = get_worker_by_id(o.id);
    sprintf(text_buffer, "Worker %d, doing %s", w->id, job_str(w->job));
    DrawTextEx(*font, text_buffer,
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size, font_size},
               font_size, 4, BLUE);

    break;
  }
  case O_MACHINE: {
    // printf("DEBUG: Machine under cursor\n");
    Machine *m = get_machine_by_id(o.id);
    sprintf(text_buffer, "%s machine %d", machine_str(m->mtype), m->id);
    DrawTextEx(*font, text_buffer,
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size, font_size},
               font_size, 4, BLUE);
    if (m->current_work_order) {
      sprintf(text_buffer, "Machining batch of %s", recipe_str(m->recipe.name));
      DrawTextEx(
          *font, text_buffer,
          (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size, (font_size * 2)},
          font_size, 4, BLUE);
    } else {
      DrawTextEx(
          *font, "Idle",
          (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size, (font_size * 2)},
          font_size, 4, BLUE);
    }

    DrawTextEx(*font, "a) add job",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         SQUARE_SIZE * (MAX_Y - 2)},
               font_size, 4, BLUE);
    break;
  }
  case O_STOCKPILE: {
    // printf("DEBUG: Stockpile under cursor\n");

    Stockpile *s = get_stockpile_by_id(o.id);
    sprintf(text_buffer, "Stockpile with ID %d", s->id);
    DrawTextEx(*font, text_buffer,
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size, font_size},
               font_size, 4, BLUE);

    int y_offset = 2;

    if (s->things_in_stockpile > 0) {
      DrawTextEx(
          *font, "Contains",
          (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size, (font_size * 2)},
          font_size, 4, BLUE);
      y_offset++;

      for (int i = 0; i < s->things_in_stockpile; i++) {
        if (s->content_count[i] > 0) {
          sprintf(text_buffer, "\t%s: %d", material_str(s->contents[i]),
                  s->content_count[i]);
          DrawTextEx(*font, text_buffer,
                     (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                               (font_size * y_offset)},
                     font_size, 4, BLUE);
          y_offset++;
        }
      }
    }

    if (s->c_required_material > 0) {
      DrawTextEx(*font, "Requires",
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (font_size * y_offset)},
                 font_size, 4, BLUE);

      y_offset++;

      for (int i = 0; i < s->c_required_material; i++) {
        if (s->required_material_count[i] > 0) {
          sprintf(text_buffer, "\t%s: %d",
                  material_str(s->required_material[i]),
                  s->required_material_count[i]);
          DrawTextEx(*font, text_buffer,
                     (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                               (font_size * y_offset)},
                     font_size, 4, BLUE);
          y_offset++;
        }
      }
    }

    break;
  }
  default: {
    // printf("ERROR: Unrecognized object %d under cursor\n", o.object_type);
    exit(1);
  }
  }

  // draw cursor
  draw_frame_in_square(FRAME_CURSOR, gs->cursor.x, gs->cursor.y, tex);

  if (paused) {
    DrawTextEx(*font, "PAUSED",
               (Vector2){SQUARE_SIZE * (MAX_X / 2.0f) + font_size,
                         SQUARE_SIZE * (MAX_Y - 1)},
               font_size, 4, BLUE);
  }

  EndDrawing();
}

void handle_input(GameState *gs) {
  if (IsKeyPressed(KEY_RIGHT) && (gs->cursor.x < MAX_X)) {
    gs->cursor.x++;
  }
  if (IsKeyPressed(KEY_LEFT) && (gs->cursor.x > 0)) {
    gs->cursor.x--;
  }
  if (IsKeyPressed(KEY_UP) && (gs->cursor.y > 0)) {
    gs->cursor.y--;
  }
  if (IsKeyPressed(KEY_DOWN) && (gs->cursor.y < MAX_Y)) {
    gs->cursor.y++;
  }
  if (IsKeyPressed(KEY_P)) {
    paused = !paused;
  }
  ObjectReference o = object_under_point(gs->cursor.x, gs->cursor.y);
  if (o.object_type == O_MACHINE) {
    if (IsKeyPressed(KEY_A)) {
      // @HACK, this is game logic, shouldn't be here.
      // possible jobs should be part of machine definition
      Machine *m = get_machine_by_id(o.id);
      RecipeName r;
      switch (m->mtype) {
      case WIRE_PULLER:
        r = PULL_WIRE;
        break;

      case WIRE_WINDER:
        r = WIND_WIRE;
        break;

      default:
        break;
      }
      assign_machine_production_job(o.id, r);
    }
  }
}

int main(void) {
  int frame = 0;
  long turn = 0;

  srand(time(0));
  GameState *gs = new_game();
  char *context_menu_text = malloc(sizeof(char) * 100);

  struct DrawState ds = {
      .gs = gs,
      .context_menu_text = context_menu_text,
  };

  if (!context_menu_text) {
    printf("Allocation Error for context menu text\n");
    exit(1);
  }

  // Winder machine
  int in = add_stockpile(2, 2, 2, 2);
  Stockpile *s_in = get_stockpile_by_id(in);
  int out = add_stockpile(2, 6, 3, 3);
  Stockpile *s_out = get_stockpile_by_id(out);
  s_out->can_be_taken_from = true;
  add_material_to_stockpile(s_in, WASHED_IRON_WIRE_COIL, 1);
  add_material_to_stockpile(s_in, EMPTY_SPINDLE, 1);

  int winder = add_machine(WIRE_WINDER, "WireWind", 2, 4);
  add_output_stockpile_to_machine(winder, out);
  add_input_stockpile_to_machine(winder, in);

  // Puller machine
  out = add_stockpile(11, 10, 3, 3);
  s_out = get_stockpile_by_id(out);
  s_out->can_be_taken_from = true;

  in = add_stockpile(7, 10, 2, 2);
  // add_material_to_stockpile(in, SPINDLED_WIRE_COIL, 5);
  s_in = get_stockpile_by_id(in);
  add_required_material_to_stockpile(s_in, SPINDLED_WIRE_COIL, 5);

  int puller = add_machine(WIRE_PULLER, "WirePull1", 9, 10);
  add_output_stockpile_to_machine(puller, out);
  add_input_stockpile_to_machine(puller, in);

  add_worker();
  add_worker();
  assign_machine_production_job(winder, WIND_WIRE);
  // assign_machine_production_job(puller, PULL_WIRE);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE_GOAL");
  Font font = LoadFont("assets/romulus.png");
  Texture2D ascii = LoadTexture("assets/16x16-RogueYun-AgmEdit.png");
  ds.font = &font;
  ds.tilemap = &ascii;
  SetTargetFPS(FPS);

  while (!WindowShouldClose() && !quit) {
    handle_input(gs);
    draw_game_state(&ds);
    if (frame % (FPS / TPS) == 0 && !paused) {
      tick_game();
      turn++;
    }
    frame++;
  }

  UnloadTexture(ascii);
  UnloadFont(font);
  CloseWindow();

  free(context_menu_text);

  return 0;
}
