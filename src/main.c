#include "game.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>

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
#define FRAME_UNKNOWN (3 * 16) + 15

#define FPS 60
#define TPS 60

bool quit = false;

typedef enum {
  MENU_NONE,
  MENU_MAIN,
  MENU_MACHINE_SELECT,
  MENU_ATTACH_MACHINE_STOCKPILE,
  MENU_ADD_REQUIRED_MATERIAL,
} MenuMode;

struct DrawState {
  GameState *gs;
  char *context_menu_text;
  Texture2D *tilemap;
  Font *font;
  MenuMode menu_mode;
  bool paused;
  bool placement_mode;
  enum ObjectType placement_of;
  int placement_of_sub;
  Vector2 placement_size;
  int menu_modifier;
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
    Stockpile s = gs->stockpiles[i];
    int x = s.location.x;
    int y = s.location.y;
    int w = s.size.x;
    int h = s.size.y;

    for (int i = 0; i < w; i++) {
      for (int j = 0; j < h; j++) {
        draw_frame_in_square(FRAME_STOCKPILE, x + i, y + j, tex);
      }
    }

    DrawLine(SQUARE_SIZE * x, SQUARE_SIZE * y, SQUARE_SIZE * (x + w),
             SQUARE_SIZE * y, WHITE);

    DrawLine(SQUARE_SIZE * x, SQUARE_SIZE * (y + h), SQUARE_SIZE * (x + w),
             SQUARE_SIZE * (y + h), WHITE);

    DrawLine(SQUARE_SIZE * x, SQUARE_SIZE * y, SQUARE_SIZE * x,
             SQUARE_SIZE * (y + h), WHITE);

    DrawLine(SQUARE_SIZE * (x + w), SQUARE_SIZE * y, SQUARE_SIZE * (x + w),
             SQUARE_SIZE * (y + h), WHITE);

    for (int j = 0; j < s.c_contents; j++) {
      if (s.contents_count[j] > 0) {
        draw_frame_in_square(FRAME_MATERIAL, (x + j), y, tex);
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

  if (ds->placement_mode) {
    int y_offset = 1;
    DrawTextEx(*font, "PLACEMENT MODE (q to quit)",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);
    DrawTextEx(*font, "hjkl to resize, c to place.",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);
  }

  else if (ds->menu_mode == MENU_MAIN) {
    int y_offset = 1;
    DrawTextEx(*font, "MENU (q to quit)",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);

    DrawTextEx(*font, "m) MACHINE",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);

    DrawTextEx(*font, "s) STOCKPILE",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);
  }

  else if (ds->menu_mode == MENU_MACHINE_SELECT) {
    int y_offset = 1;

    DrawTextEx(*font, "SELECT MACHINE (q to quit)",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);

    for (int i = 0; i < COUNT_MACHINE_TYPES; i++) {
      sprintf(text_buffer, "%d) %s", i + 1, machine_str(i));
      DrawTextEx(*font, text_buffer,
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (y_offset++ * font_size)},
                 font_size, 4, BLUE);
    }

  } else if (ds->menu_mode == MENU_ADD_REQUIRED_MATERIAL) {
    int y_offset = 1;

    DrawTextEx(*font, "ADD REQUIRED MATERIAL TO STOCKPILE (q to quit)",
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);
    y_offset++;

    sprintf(text_buffer, "Quantity (j/k for dec/inc): %d", ds->menu_modifier);
    DrawTextEx(*font, text_buffer,
               (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                         (y_offset++ * font_size)},
               font_size, 4, BLUE);
    y_offset++;

    for (int i = 1; i < PM_COUNT; i++) {
      sprintf(text_buffer, "%d) %s", i, material_str(i));
      DrawTextEx(*font, text_buffer,
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (y_offset++ * font_size)},
                 font_size, 4, BLUE);
    }

  } else if (ds->menu_mode == MENU_ATTACH_MACHINE_STOCKPILE) {

    int y_offset = 1;

    if (ds->menu_modifier == 'i') {
      DrawTextEx(*font, "SELECT MACHINE TO ATTACH INPUT STOCKPILE (q to quit)",
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (y_offset++ * font_size)},
                 font_size, 4, BLUE);
    } else if (ds->menu_modifier == 'o') {
      DrawTextEx(*font, "SELECT MACHINE TO ATTACH OUTPUT STOCKPILE (q to quit)",
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (y_offset++ * font_size)},
                 font_size, 4, BLUE);
    }

    Machine *m;
    for (int i = 0; i < gs->c_machines; i++) {
      m = get_machine_by_id(i);
      if (ds->menu_modifier == 'i' && m->input_stockpile == -1) {
        sprintf(text_buffer, "%d) %s", i, machine_str(i));
        DrawTextEx(*font, text_buffer,
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (y_offset++ * font_size)},
                   font_size, 4, BLUE);
      }
      if (ds->menu_modifier == 'o' && m->output_stockpile == -1) {
        sprintf(text_buffer, "%d) %s", i, machine_str(i));
        DrawTextEx(*font, text_buffer,
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (y_offset++ * font_size)},
                   font_size, 4, BLUE);
      }
    }
  } else {
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
      int y_offset = 1;
      // printf("DEBUG: Machine under cursor\n");
      Machine *m = get_machine_by_id(o.id);
      sprintf(text_buffer, "%s machine %d", machine_str(m->type), m->id);
      DrawTextEx(*font, text_buffer,
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (font_size * y_offset)},
                 font_size, 4, BLUE);
      y_offset++;

      if (m->has_current_work_order) {
        sprintf(text_buffer, "Machining batch of %s",
                recipe_str(m->active_recipe.name));
        DrawTextEx(*font, text_buffer,
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (font_size * y_offset)},
                   font_size, 4, BLUE);
        y_offset++;
      } else {
        DrawTextEx(*font, "Idle",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (font_size * y_offset)},
                   font_size, 4, BLUE);
      }

      if (m->output_stockpile > -1 && m->input_stockpile > -1) {
        DrawTextEx(*font, "a) add job",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             SQUARE_SIZE * (MAX_Y - 2)},
                   font_size, 4, BLUE);
      } else {

        DrawTextEx(*font, "Can't add job, missing io stockpiles",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             SQUARE_SIZE * (MAX_Y - 2)},
                   font_size, 4, BLUE);
      }
      break;
    }
    case O_STOCKPILE: {
      int y_offset = 1;

      Stockpile *s = get_stockpile_by_id(o.id);

      if (s->attached_machine >= 0) {
        sprintf(text_buffer, "Stockpile %d: %s for machine %d", s->id,
                (s->io == INPUT) ? "input" : "output", s->attached_machine);
      } else {
        sprintf(text_buffer, "Stockpile %d", s->id);
      }
      DrawTextEx(*font, text_buffer,
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (y_offset++ * font_size)},
                 font_size, 4, BLUE);

      y_offset++;

      if (s->c_contents > 0) {
        DrawTextEx(*font, "Contains",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (y_offset++ * font_size)},
                   font_size, 4, BLUE);

        for (int i = 0; i < s->c_contents; i++) {
          if (s->contents_count[i] > 0) {
            sprintf(text_buffer, "\t%s: %d", material_str(s->contents[i]),
                    s->contents_count[i]);
            DrawTextEx(*font, text_buffer,
                       (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                                 (font_size * y_offset++)},
                       font_size, 4, BLUE);
          }
        }
      }

      if (s->c_required_material > 0) {
        DrawTextEx(*font, "Requires",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (font_size * y_offset++)},
                   font_size, 4, BLUE);

        for (int i = 0; i < s->c_required_material; i++) {
          if (s->required_material_count[i] > 0) {
            sprintf(text_buffer, "\t%s: %d",
                    material_str(s->required_material[i]),
                    s->required_material_count[i]);
            DrawTextEx(*font, text_buffer,
                       (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                                 (font_size * y_offset++)},
                       font_size, 4, BLUE);
          }
        }
      }

      y_offset++;

      DrawTextEx(*font, "r) add required material",
                 (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                           (font_size * y_offset++)},
                 font_size, 4, BLUE);

      if (s->attached_machine == -1) {
        DrawTextEx(*font, "i) Add as input for machine",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (font_size * y_offset++)},
                   font_size, 4, BLUE);

        DrawTextEx(*font, "o) Add as output for machine",
                   (Vector2){SQUARE_SIZE * (MAX_X + 1) + font_size,
                             (font_size * y_offset++)},
                   font_size, 4, BLUE);
      }

      break;
    }
    default: {
      // printf("ERROR: Unrecognized object %d under cursor\n", o.object_type);
      exit(1);
    }
    }
  }

  // draw cursor
  if (ds->menu_mode == MENU_NONE) {
    draw_frame_in_square(FRAME_CURSOR, gs->cursor.x, gs->cursor.y, tex);
  }

  // draw placement rect
  if (ds->placement_mode) {
    int frame_sprite;
    if (ds->placement_of == O_STOCKPILE) {
      frame_sprite = FRAME_STOCKPILE;
    } else if (ds->placement_of == O_MACHINE) {
      frame_sprite = FRAME_MACHINE;
    } else {
      frame_sprite = FRAME_UNKNOWN;
    }

    int sx = ds->gs->cursor.x;
    int sy = ds->gs->cursor.y;
    int ex = sx + ds->placement_size.x;
    int ey = sy + ds->placement_size.y;
    for (int x = sx; x < ex; x++) {
      for (int y = sy; y < ey; y++) {
        draw_frame_in_square(frame_sprite, x, y, tex);
      }
    }
  }

  if (ds->paused) {
    DrawTextEx(*font, "PAUSED",
               (Vector2){SQUARE_SIZE * (MAX_X / 2.0f) + font_size,
                         SQUARE_SIZE * (MAX_Y - 1)},
               font_size, 4, BLUE);
  }

  EndDrawing();
}

void handle_input(struct DrawState *ds) {
  GameState *gs = ds->gs;
  ObjectReference o = object_under_point(gs->cursor.x, gs->cursor.y);

  if (ds->menu_mode == MENU_MAIN) {
    if (IsKeyPressed(KEY_Q)) {
      ds->menu_mode = MENU_NONE;
    }

    if (IsKeyPressed(KEY_S)) { // stockpile placement
      ds->menu_mode = MENU_NONE;
      ds->placement_mode = true;
      ds->placement_of = O_STOCKPILE;
      ds->placement_size = (Vector2){1, 1};
    }

    if (IsKeyPressed(KEY_M)) {
      ds->menu_mode = MENU_MACHINE_SELECT;
    }
    return;
  }

  if (ds->menu_mode == MENU_MACHINE_SELECT) {
    if (IsKeyPressed(KEY_Q)) {
      ds->menu_mode = MENU_MAIN;
    }

    for (int i = 0; i < COUNT_MACHINE_TYPES; i++) {
      // 49 is num key 1
      if (IsKeyPressed(49 + i)) {
        printf("DEBUG: pressed key %d\n", 49 + i);
        ds->menu_mode = MENU_NONE;
        ds->placement_mode = true;
        ds->placement_of = O_MACHINE;
        ds->placement_of_sub = i;
        Vector ps = machine_size(i);
        ds->placement_size.x = ps.x;
        ds->placement_size.y = ps.y;
      }
    }
    return;
  }

  if (ds->menu_mode == MENU_ATTACH_MACHINE_STOCKPILE) {
    int sid = o.id;
    if (IsKeyPressed(KEY_Q)) {
      ds->menu_mode = MENU_NONE;
    }

    for (int i = 0; i < 9; i++) {
      // 48 is num key 0
      if (IsKeyPressed(48 + i)) {
        if (ds->menu_modifier == 'i') {
          add_input_stockpile_to_machine(i, sid);
          ds->menu_mode = MENU_NONE;
        } else if (ds->menu_modifier == 'o') {
          add_output_stockpile_to_machine(i, sid);
          ds->menu_mode = MENU_NONE;
        } else {
          printf("ERROR: attach stockpile with invalid modifier %c",
                 (char)ds->menu_modifier);
          exit(1);
        }
      }
    }
    return;
  }

  if (ds->menu_mode == MENU_ADD_REQUIRED_MATERIAL) {
    Stockpile *s = get_stockpile_by_id(o.id);
    if (IsKeyPressed(KEY_Q)) {
      ds->menu_mode = MENU_NONE;
    }

    if (IsKeyPressed(KEY_J)) {
      ds->menu_modifier++;
    }

    if (IsKeyPressed(KEY_K) && ds->menu_modifier > 1) {
      ds->menu_modifier--;
    }

    for (int i = 1; i < PM_COUNT; i++) {
      // 48 is num key 0
      if (IsKeyPressed(48 + i)) {
        add_required_material_to_stockpile(s, i, ds->menu_modifier);
        ds->menu_mode = MENU_NONE;
      }
    }
    return;
  }

  if (ds->menu_mode == MENU_NONE) {
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
  }

  if (ds->placement_mode) {
    if (ds->placement_of == O_STOCKPILE) {
      if (IsKeyPressed(KEY_L)) {
        ds->placement_size.x += 1;
      }
      if (IsKeyPressed(KEY_H) && ds->placement_size.x > 1) {
        ds->placement_size.x -= 1;
      }
      if (IsKeyPressed(KEY_J)) {
        ds->placement_size.y += 1;
      }
      if (IsKeyPressed(KEY_K) && ds->placement_size.y > 1) {
        ds->placement_size.y -= 1;
      }

      if (IsKeyPressed(KEY_C)) {
        add_stockpile(gs->cursor.x, gs->cursor.y, ds->placement_size.x,
                      ds->placement_size.y);
        ds->placement_mode = false;
      }

      if (IsKeyPressed(KEY_Q)) {
        ds->placement_mode = false;
        ds->menu_mode = MENU_MAIN;
      }
    }

    if (ds->placement_of == O_MACHINE) {

      if (IsKeyPressed(KEY_C)) {
        add_machine(ds->placement_of_sub, gs->cursor.x, gs->cursor.y);
        ds->placement_mode = false;
      }

      if (IsKeyPressed(KEY_Q)) {
        ds->placement_mode = false;
        ds->menu_mode = MENU_MACHINE_SELECT;
      }
    }
  }

  if (ds->menu_mode == MENU_NONE && !ds->placement_mode) {
    if (IsKeyPressed(KEY_Q)) {
      quit = true;
    }

    if (IsKeyPressed(KEY_P)) {
      ds->paused = !ds->paused;
    }

    if (IsKeyPressed(KEY_M)) {
      ds->menu_mode = MENU_MAIN;
    }
  }

  /* if (IsKeyPressed(KEY_J)) { */
  /*   debug_print_job_queue(); */
  /* } */
  /* if (IsKeyPressed(KEY_R)) { */
  /*   debug_print_ro_queue(); */
  /* } */

  if (o.object_type == O_MACHINE) {
    if (IsKeyPressed(KEY_A)) {
      Machine *m = get_machine_by_id(o.id);
      const RecipeName *rs = possible_recipes(m);
      // @IMPROVE: currently this just takes the first thing
      // should be able to pick any possible recipe
      RecipeName r = *rs;
      assign_machine_production_job(o.id, r);
    }
  }

  if (o.object_type == O_STOCKPILE) {
    Stockpile *s = get_stockpile_by_id(o.id);

    if (IsKeyPressed(KEY_R)) {
      ds->menu_mode = MENU_ADD_REQUIRED_MATERIAL;
      ds->menu_modifier = 1;
    }

    if (s->attached_machine == -1) {
      if (IsKeyPressed(KEY_I)) {
        ds->menu_mode = MENU_ATTACH_MACHINE_STOCKPILE;
        ds->menu_modifier = 'i';
      }
      if (IsKeyPressed(KEY_O)) {
        ds->menu_mode = MENU_ATTACH_MACHINE_STOCKPILE;
        ds->menu_modifier = 'o';
      }
    }
  }
}

int main(void) {
  int frame = 0;
  long turn = 0;
  const bool setup = false;

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

  int factory_in = add_stockpile(0, 3, 2, 2);
  int factory_out = add_stockpile(14, 3, 2, 2);
  Stockpile *s = get_stockpile_by_id(factory_in);
  s->can_be_taken_from = true;
  add_material_to_stockpile(s, EMPTY_SPINDLE, 5);
  add_material_to_stockpile(s, WASHED_IRON_WIRE_COIL, 5);
  add_material_to_stockpile(s, SMALL_BOWL, 5);

  if (setup) {
    // WINDER machine
    int in = add_stockpile(2, 2, 2, 2);
    Stockpile *s_in = get_stockpile_by_id(in);
    int out = add_stockpile(2, 6, 2, 2);

    add_required_material_to_stockpile(s_in, EMPTY_SPINDLE, 1);
    add_material_to_stockpile(s_in, WASHED_IRON_WIRE_COIL, 1);
    add_material_to_stockpile(s_in, EMPTY_SPINDLE, 1);

    int winder = add_machine(WIRE_WINDER, 2, 4);
    add_output_stockpile_to_machine(winder, out);
    add_input_stockpile_to_machine(winder, in);

    // PULLER machine
    out = add_stockpile(11, 10, 3, 3);

    in = add_stockpile(7, 10, 2, 2);
    // add_material_to_stockpile(in, SPINDLED_WIRE_COIL, 5);
    s_in = get_stockpile_by_id(in);
    add_required_material_to_stockpile(s_in, SPINDLED_WIRE_COIL, 5);

    int puller = add_machine(WIRE_PULLER, 9, 10);
    add_output_stockpile_to_machine(puller, out);
    add_input_stockpile_to_machine(puller, in);

    // CUTTER machine
    int cutter = add_machine(WIRE_CUTTER, 12, 3);
    out = add_stockpile(12, 5, 2, 2);

    in = add_stockpile(10, 3, 2, 2);
    s_in = get_stockpile_by_id(in);
    add_required_material_to_stockpile(s_in, LONG_WIRES, 50);
    add_material_to_stockpile(s_in, SMALL_BOWL, 5);

    add_output_stockpile_to_machine(cutter, out);
    add_input_stockpile_to_machine(cutter, in);

    // GRINDER machine
    int grinder = add_machine(WIRE_GRINDER, 7, 5);
    out = add_stockpile(7, 6, 2, 1);

    in = add_stockpile(6, 4, 2, 1);
    s_in = get_stockpile_by_id(in);

    assign_machine_production_job(winder, WIND_WIRE);
  }

  // add workers
  add_worker();
  add_worker();
  add_worker();

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE_GOAL");
  Font font = LoadFont("assets/romulus.png");
  Texture2D ascii = LoadTexture("assets/16x16-RogueYun-AgmEdit.png");
  ds.font = &font;
  ds.tilemap = &ascii;
  SetTargetFPS(FPS);

  while (!WindowShouldClose() && !quit) {
    handle_input(&ds);
    draw_game_state(&ds);
    if (frame % (FPS / TPS) == 0 && (ds.menu_mode == MENU_NONE || !ds.paused)) {
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
