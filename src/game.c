#include "game.h"
#include <string.h>

GameState game = {0};
GameState *new_game(void) { return &game; }

Worker *get_worker_by_id(int id);

/* -------------
 * MESSAGE BUFFER
 * ------------- */

void add_message(char *message) {
  if (strlen(message) > MESSAGE_MAX_SIZE) {
    printf("ERROR: message too long\n");
    exit(1);
  }

  strcpy(game.message_buffer[game.message_head], message);

  game.message_head++;
  if (game.message_head >= MESSAGE_BUFFER_SIZE) {
    game.message_head = 0;
  }
}

/* -------------
 * MATERIALS
 * ------------- */

char _material[50] = {0};

char *material_str(ProductionMaterial m) {
  switch (m) {

  case EMPTY_SPINDLE: {
    strcpy(_material, "EMPTY_SPINDLE");
    break;
  }

  case WASHED_IRON_WIRE_COIL: {
    strcpy(_material, "WASHED_IRON_WIRE_COIL");
    break;
  }

  case SPINDLED_WIRE_COIL: {
    strcpy(_material, "SPINDLED_WIRE_COIL");
    break;
  }

  case WIRE: {
    strcpy(_material, "WIRE");
    break;
  }
  }
  return _material;
}

/* -------------
 * JOBS
 * ------------- */

#define MAX_JOB_QUEUE 100

int job_queue_head = 0;
int job_queue_tail = 0;

struct JobQueueItem {
  int machine_id;
  enum Job job;
} job_queue[MAX_JOB_QUEUE];

char _job[50] = {0};

char *job_str(enum Job j) {
  switch (j) {

  case NONE: {
    strcpy(_job, "NONE");
    break;
  }

  case MAN_MACHINE: {
    strcpy(_job, "MAN_MACHINE");
    break;
  }

  case EMPTY_OUTPUT_BUFFER: {
    strcpy(_job, "EMPTY_OUTPUT_BUFFER");
    break;
  }

  case FILL_INPUT_BUFFER: {
    strcpy(_job, "FILL_INPUT_BUFFER");
    break;
  }
  }

  return _job;
}

void enqueue_job(int mid, enum Job job) {
  printf("DEBUG_ENQUEUE_JOB: MachineID %d and job %s\n", mid, job_str(job));
  job_queue[job_queue_tail].machine_id = mid;
  job_queue[job_queue_tail].job = job;
  job_queue_tail++;
  if (job_queue_tail > MAX_JOB_QUEUE) {
    job_queue_tail = 0;
  }
}

bool jobs_on_queue(void) { return job_queue_tail != job_queue_head; }

struct JobQueueItem pop_job(void) {
  struct JobQueueItem j;
  if (job_queue_tail == job_queue_head) {
    j.machine_id = -1;
    j.job = 0;
  } else {
    j.machine_id = job_queue[job_queue_head].machine_id;
    j.job = job_queue[job_queue_head].job;
    job_queue_head++;
  }
  return j;
}

/* -------------
 * RECIPES
 * ------------- */

const Recipe wind_wire = {.name = WIND_WIRE,
                          .i_count = 2,
                          .inputs = {WASHED_IRON_WIRE_COIL, EMPTY_SPINDLE},
                          .inputs_count = {1, 1},
                          .o_count = 1,
                          .outputs = {SPINDLED_WIRE_COIL},
                          .outputs_count = {1},
                          .time = 10};

const Recipe pull_wire = {.name = PULL_WIRE,
                          .i_count = 1,
                          .inputs = {SPINDLED_WIRE_COIL},
                          .inputs_count = {1},
                          .o_count = 1,
                          .outputs = {WIRE},
                          .outputs_count = {100},
                          .time = 10};

Recipe get_recipe_from_name(RecipeName rn) {
  switch (rn) {
  case PULL_WIRE:
    return pull_wire;
  case WIND_WIRE:
    return wind_wire;
  case CUT_WIRE:
    printf("CUT_WIRE recipe not implemented\n");
    exit(1);
  default:
    printf("Unknown recipe %d\n", rn);
    exit(1);
  }
}

char _recipe[50] = {0};

char *recipe_str(RecipeName rn) {
  switch (rn) {

  case WIND_WIRE: {
    strcpy(_recipe, "WIND_WIRE");
    break;
  }

  case PULL_WIRE: {
    strcpy(_recipe, "PULL_WIRE");
    break;
  }

  case CUT_WIRE: {
    strcpy(_recipe, "CUT_WIRE");
    break;
  }
  }
  return _recipe;
}

/* -------------
 * STOCKPILES
 * ------------- */

int add_stockpile(int x, int y, int w, int h) {

  int id = game.c_stockpile;

  if (id > MAX_STOCKPILES) {
    printf("ERROR: Exceeded maximum stockpiles\n");
    exit(1);
  }

  game.stockpiles[id] = (Stockpile){id, {x, y}, {w, h}, 0, {0}, {0}};
  game.c_stockpile++;
  return id;
}

Stockpile *get_stockpile_by_id(int id) { return &game.stockpiles[id]; }

void add_material_to_stockpile(int sid, ProductionMaterial p, int count) {
  Stockpile *s = get_stockpile_by_id(sid);
  s->contents[s->things_in_stockpile] = p;
  s->content_count[s->things_in_stockpile] = count;
  s->things_in_stockpile++;
}

int material_in_stockpile(Stockpile const *s, ProductionMaterial p) {
  int count = 0;
  for (int i = 0; i < s->things_in_stockpile; i++) {
    if (s->contents[i] == p) {
      count += s->content_count[i];
    }
  }
  return count;
}

void remove_material_from_stockpile(Stockpile *s, ProductionMaterial p,
                                    int count) {
  int mis = material_in_stockpile(s, p);
  if (mis < count) {
    printf("ERROR: Not enough material in stockpile to remove");
    exit(1);
  }

  int remaining = count;
  for (int i = 0; i < s->things_in_stockpile; i++) {
    if (s->contents[i] == p) {
      int available = s->content_count[i];
      int to_remove = (available > remaining) ? remaining : available;
      remaining -= to_remove;
      s->content_count[i] -= to_remove;
    }
    if (remaining == 0) {
      return;
    }
  }
}

/* -------------
 * MACHINES
 * ------------- */

char _machine[50] = {0};

char *machine_str(enum MachineType m) {
  switch (m) {

  case WIRE_WINDER: {
    strcpy(_machine, "WIRE_WINDER");
    break;
  }

  case WIRE_PULLER: {
    strcpy(_machine, "WIRE_PULLER");
    break;
  }

  case WIRE_CUTTER: {
    strcpy(_machine, "WIRE_CUTTER");
    break;
  }
  }
  return _machine;
}

int add_machine(enum MachineType type, char *name, int x, int y) {
  int id = game.c_machines;

  if (id > MAX_MACHINES) {
    printf("ERROR: Exceeded maximum machines\n");
    exit(1);
  }

  if (strlen(name) > 9) {
    printf("ERROR: Machine name %s too long\n", name);
    exit(1);
  }

  game.machines[id] = (Machine){
      .id = id,
      .name = {0},
      .mtype = type,
      .job_time_left = 0,
      .current_work_order = false,
      .outputs = 0,
      .worker = -1,
      .location = (Vector){x, y},
      .size = (Vector){2, 2},
      .output_stockpile = -1,
  };

  strcpy(game.machines[id].name, name);

  game.c_machines++;

  return id;
}

Machine *get_machine_by_id(int id) { return &game.machines[id]; }

void add_output_stockpile_to_machine(int mid, int sid) {
  Machine *m = get_machine_by_id(mid);
  m->output_stockpile = sid;
}

void add_input_stockpile_to_machine(int mid, int sid) {
  Machine *m = get_machine_by_id(mid);
  m->input_stockpile = sid;
}

void assign_machine_production_job(int id, RecipeName rn) {
  Recipe r = get_recipe_from_name(rn);
  Machine *m = get_machine_by_id(id);

  m->current_work_order = true;
  m->recipe = r;
  m->job_time_left = r.time;
  enqueue_job(id, MAN_MACHINE);
}

void complete_production(Machine *m) {
  Worker *w = get_worker_by_id(m->worker);
  w->status = W_IDLE;
  w->job = NONE;

  int num_outputs = m->recipe.o_count;
  m->current_work_order = false;
  m->worker = -1;
  m->outputs = num_outputs;

  for (int i = 0; i < num_outputs; i++) {
    m->output_buffer[i] = m->recipe.outputs[i];
    m->output_buffer_count[i] = m->recipe.outputs_count[i];
  }

  enqueue_job(m->id, EMPTY_OUTPUT_BUFFER);
}

int machine_has_input(Machine *m, ProductionMaterial p) {
  int count = 0;
  for (int i = 0; i < m->inputs; i++) {
    if (m->input_buffer[i] == p) {
      count += m->input_buffer_count[i];
    }
  }
  return count;
}

typedef struct MaterialCount {
  ProductionMaterial material;
  int count;
} MaterialCount;

MaterialCount next_unfullfilled_material(Machine *m, Recipe r) {
  MaterialCount mc = {-1, 0};

  for (int i = 0; i < r.i_count; i++) {
    ProductionMaterial p = r.inputs[i];
    int required = r.inputs_count[i];

    int current = machine_has_input(m, p);

    // printf("DEBUG_UNF: %s - need: %d, have: %d\n", material_str(p),
    // required,
    //        current);

    if (current < required) {
      mc.material = p;
      mc.count = required - current;
      return mc;
    }
  }
  return mc;
}

bool machine_has_required_inputs(Machine *m, Recipe r) {
  MaterialCount mc = next_unfullfilled_material(m, r);
  return (mc.count == 0);
}

void tick_machine(Machine *m) {
  if (m->current_work_order && m->worker >= 0 &&
      machine_has_required_inputs(m, m->recipe)) {
    // printf("DEBUG_TICK_MACHINE: machine is working...\n");
    if (m->job_time_left > 0) {
      m->job_time_left--;
    } else {
      complete_production(m);

      printf("DEBUG_%ld: Machine %s produced output: ", game.turn, m->name);

      for (int i = 0; i < m->outputs; i++)
        printf("%s: %d\n", material_str(m->output_buffer[i]),
               m->output_buffer_count[i]);
    }
  }
}

/* -------------
 * WORKERS
 * ------------- */

Worker *get_worker_by_id(int id) { return &game.workers[id]; }

char _status[50] = {0};

char *status_str(enum WorkerStatus s) {
  switch (s) {

  case W_IDLE: {
    strcpy(_status, "W_IDLE");
    break;
  }

  case W_CARRYING: {
    strcpy(_status, "W_CARRYING");
    break;
  }

  case W_PRODUCING: {
    strcpy(_status, "W_PRODUCING");
    break;
  }

  case W_MOVING: {
    strcpy(_status, "W_MOVING");
    break;
  }
  }

  return _status;
}

void print_worker(const Worker *w) {
  printf("DEBUG: Summary for worker %d: ", w->id);
  printf("\tJOB: %s", job_str(w->job));
  printf("\tSTATUS: %s", status_str(w->status));
  printf("\n");
}

int first_idle_worker(void) {
  for (int i = 0; i < game.c_workers; i++) {
    if (game.workers[i].status == W_IDLE)
      return game.workers[i].id;
  }
  return -1;
}

int add_worker(void) {
  int id = game.c_workers;

  if (id > MAX_WORKERS) {
    printf("ERROR: Exceeded maximum workers\n");
    exit(1);
  }

  game.workers[id] = (Worker){.id = id,
                              .status = W_IDLE,
                              .location = {0, 0},
                              .target = {0, 0},
                              .carrying_count = 0};

  game.c_workers++;

  return id;
}

void worker_take_job(int worker_id, struct JobQueueItem jq) {
  Machine *m = get_machine_by_id(jq.machine_id);
  Worker *w = get_worker_by_id(worker_id);
  w->status = W_MOVING;
  w->machine = m;
  w->job = jq.job;

  char mb[256] = {0};

  switch (jq.job) {
  case MAN_MACHINE:
    w->target = m->location;

    printf("DEBUG_WORKER_TAKE_JOB: worker %d took job to to man machine %s\n",
           worker_id, m->name);
    sprintf(mb, "%ld: assigning worker %d to man machine %s\n", game.turn,
            worker_id, m->name);
    add_message(mb);

    break;
  case EMPTY_OUTPUT_BUFFER:
    w->target = m->location;
    sprintf(mb, "%ld: assigning worker %d to empty machine %s\n", game.turn,
            worker_id, m->name);
    printf("DEBUG_WORKER_TAKE_JOB: worker %d took job to empty machine %s\n",
           worker_id, m->name);
    add_message(mb);
    break;
  default:
    printf("ERROR: Couldn't find job\n");
    exit(EXIT_FAILURE);
  }
}

void worker_pickup_output(Worker *w, Machine *m) {
  // printf("DEBUG: %d\n", m->outputs);
  // printf("DEBUG: %d\n", m->output_buffer[0]);
  m->outputs--;
  int o = m->outputs;
  ProductionMaterial mat = m->output_buffer[o];
  int mat_count = m->output_buffer_count[o];
  w->carrying = mat;
  w->carrying_count = mat_count;
  printf("%ld: worker %d picked up %d %s from %s\n", game.turn, w->id,
         mat_count, material_str(mat), m->name);
}

void worker_drop_material_at_machine(Worker *w, Machine *m) {
  // printf("DEBUG: %d\n", m->outputs);
  // printf("DEBUG: %d\n", m->output_buffer[0]);

  int i = m->inputs;
  m->input_buffer[i] = w->carrying;
  m->input_buffer_count[i] = w->carrying_count;

  w->carrying = -1;
  w->carrying_count = 0;

  m->inputs++;
  printf("DEBUG_%ld: worker %d dropped %d %s to machine %s\n", game.turn, w->id,
         m->input_buffer_count[i], material_str(m->input_buffer[i]), m->name);
}

void worker_pickup_from_stockpile(Worker *w, Stockpile *s, ProductionMaterial p,
                                  int count) {
  int mis = material_in_stockpile(s, p);
  printf("DEBUG_%ld: getting %d %s from stockpile. There is %d\n", game.turn,
         count, material_str(p), mis);
  // printf("DEBUG: %d\n", m->output_buffer[0]);

  if (mis < count) {
    printf("ERROR: material not in stockpile\n");
    exit(1);
  } else {
    w->carrying_count = count;
    w->carrying = p;
    remove_material_from_stockpile(s, p, count);
    printf("DEBUG_%ld: picked up %d %s from stockpile. There are %d left\n",
           game.turn, count, material_str(p), material_in_stockpile(s, p));
  }
}

void worker_drop_at_stockpile(Worker *w, Stockpile *s) {
  printf("%ld: worker %d dropped %d %s at stockpile\n", game.turn, w->id,
         w->carrying_count, material_str(w->carrying));

  s->contents[s->things_in_stockpile] = w->carrying;
  s->content_count[s->things_in_stockpile] = w->carrying_count;
  s->things_in_stockpile++;
  w->carrying_count = 0;
}

void tick_worker(Worker *w) {
  if (!vec_equal(w->location, w->target)) {
    w->location = vec_move_towards(w->location, w->target);
    return;
  }

  // Worker is at its destination
  switch (w->job) {
  case EMPTY_OUTPUT_BUFFER: {

    if (w->status == W_CARRYING) {
      Stockpile *s = get_stockpile_by_id(w->machine->output_stockpile);
      worker_drop_at_stockpile(w, s);

      if (w->machine->outputs == 0) {
        w->status = W_IDLE;
        w->job = NONE;
      } else {
        w->status = W_MOVING;
        w->target = w->machine->location;
      }

    } else if (w->status == W_MOVING) {
      Stockpile *s = get_stockpile_by_id(w->machine->output_stockpile);
      worker_pickup_output(w, w->machine);
      w->status = W_CARRYING;
      w->target = s->location;
    } else {
      printf("ERROR: Unhandled worker status %d for empty output buffer job\n",
             w->status);
    }

  } break;

  case FILL_INPUT_BUFFER: {
    Machine *m = w->machine;

    Stockpile *s = get_stockpile_by_id(m->input_stockpile);

    if (w->status == W_CARRYING) {
      // printf("DEBUG_TICK_WORKER: dropping material at machine\n");
      worker_drop_material_at_machine(w, m);
      MaterialCount mc = next_unfullfilled_material(m, m->recipe);
      if (mc.count > 0) { // the machine requires more materials
        int mis = material_in_stockpile(s, mc.material);
        if (mis < mc.count) { // not enough material in stockpile
          printf("ERROR: case where there's not enough material in the "
                 "stockpile to make a machine run is not handled\n");
          exit(1);
        }
        w->status = W_MOVING;
        w->target = s->location;
      } else { // machine has what it needs
        printf("DEBUG_TICK_WORKER: Machine has what it needs, switching to "
               "producing\n");
        m->worker = w->id;
        w->job = MAN_MACHINE;
        w->status = W_PRODUCING;
      }
    } else if (w->status == W_MOVING) {
      MaterialCount mc = next_unfullfilled_material(m, m->recipe);
      worker_pickup_from_stockpile(w, s, mc.material, mc.count);
      w->target = m->location;
      w->status = W_CARRYING;
    } else {
      printf("ERROR: Unhandled worker status %d for fill input buffer job\n",
             w->status);
    }

    break;

  case MAN_MACHINE: {
    if (w->status == W_PRODUCING) {
      return;
    }
    Machine *m = w->machine;

    if (machine_has_required_inputs(m, m->recipe)) {
      w->status = W_PRODUCING;
      return;
    }

    w->job = FILL_INPUT_BUFFER;
    w->status = W_MOVING;
    Stockpile *s = get_stockpile_by_id(m->input_stockpile);
    w->target = s->location;
  }

  break;

  case NONE: {
    // randomly wander around
    w->location = vec_move_random(w->location, 20);
  } break;
  }
  }
}

/* -------------
 * GAME
 * ------------- */

ObjectReference object_under_point(int x, int y) {
  // Worker
  for (int i = 0; i < game.c_workers; i++) {
    Worker w = game.workers[i];
    bool in_x_bound = (x == w.location.x);
    bool in_y_bound = (y == w.location.y);
    if (in_x_bound && in_y_bound)
      return (ObjectReference){O_WORKER, w.id};
  }

  // Machine
  for (int i = 0; i < game.c_machines; i++) {
    Machine m = game.machines[i];
    bool in_x_bound = (x >= m.location.x && x < (m.location.x + m.size.x));
    bool in_y_bound = (y >= m.location.y && y < (m.location.y + m.size.y));
    if (in_x_bound && in_y_bound)
      return (ObjectReference){O_MACHINE, m.id};
  }

  // Stockpile
  for (int i = 0; i < game.c_stockpile; i++) {
    Stockpile s = game.stockpiles[i];
    bool in_x_bound = (x >= s.location.x && x < (s.location.x + s.size.x));
    bool in_y_bound = (y >= s.location.y && y < (s.location.y + s.size.y));
    if (in_x_bound && in_y_bound)
      return (ObjectReference){O_STOCKPILE, s.id};
  }

  return (ObjectReference){O_NOTHING, -1};
}

void tick_game(void) {

  if (jobs_on_queue()) {
    int idle_worker = first_idle_worker();
    printf("DEBUG_TICK_GAME: First Idle Worker is %d\n", idle_worker);
    print_worker(get_worker_by_id(0));

    while (idle_worker >= 0 && jobs_on_queue()) {
      worker_take_job(idle_worker, pop_job());
      idle_worker = first_idle_worker();
    }
  }

  for (int i = 0; i < game.c_machines; i++) {
    tick_machine(&game.machines[i]);
  }

  for (int i = 0; i < game.c_workers; i++) {
    tick_worker(&game.workers[i]);
  }

  game.turn++;
}
