#include "game.h"
#include <stdio.h>
#include <string.h>

/*-------------------
  Declarations
 ------------------*/

// Job queue
// ---------

#define MAX_JOB_QUEUE 100

struct JobQueueItem {
  ObjectReference object;
  enum Job job;
} job_queue[MAX_JOB_QUEUE];

void enqueue_job(ObjectReference o, enum Job job);
bool jobs_on_queue(void);
struct JobQueueItem pop_job(void);

// Stockpiles
// ----------

Stockpile *get_stockpile_by_id(int id);

typedef struct MaterialCount {
  ProductionMaterial material;
  int count;
} MaterialCount;

int add_stockpile(int x, int y, int w, int h);

void add_required_material_to_stockpile(Stockpile *s, ProductionMaterial m,
                                        int amount);

int index_of_material_in_stockpile(const Stockpile *s, ProductionMaterial p);

Stockpile *find_stockpile_with_material(MaterialCount mc);
Stockpile *find_stockpile_with_free_material(MaterialCount mc);

int material_in_stockpile(Stockpile const *s, ProductionMaterial p);
int free_material_in_stockpile(Stockpile const *s, ProductionMaterial p);

void earmark_material_in_stockpile(Stockpile *s, ProductionMaterial p,
                                   int count);
void add_material_to_stockpile(Stockpile *s, ProductionMaterial p, int count);
void remove_material_from_stockpile(Stockpile *s, ProductionMaterial p,
                                    int amount_to_remove);

void update_replenishment_orders(Stockpile *s);

// Machines
// --------

Machine *get_machine_by_id(int id);

int add_machine(enum MachineType type, int x, int y);
void add_output_stockpile_to_machine(int mid, int sid);
void add_input_stockpile_to_machine(int mid, int sid);

// TODO: Should probably take a machine pointer, not id.
void assign_machine_production_job(int machine_id, RecipeName rn);
void start_production_job(Machine *m);
void complete_production_job(Machine *m);
int machine_has_input(Machine *m, ProductionMaterial p);
MaterialCount next_unfullfilled_material(Machine *m, Recipe r);
bool machine_has_required_inputs(Machine *m, Recipe r);

void tick_machine(Machine *m);

// Workers
// -------

Worker *get_worker_by_id(int id);

int add_worker(void);

// TODO: Should probably return a pointer to worker.
int first_idle_worker(void);
void worker_take_job(int worker_id, struct JobQueueItem jq);

void worker_pickup_output(Worker *w, Machine *m);
void worker_drop_material_at_machine(Worker *w, Machine *m);
void worker_pickup_from_stockpile(Worker *w, Stockpile *s, ProductionMaterial p,
                                  int count);
void worker_drop_at_stockpile(Worker *w, Stockpile *s);

void tick_worker(Worker *w);

// Replenishment Orders
// --------------------

#define MAX_REPLENISHMENT_QUEUE 100

struct ReplenishmentOrder {
  int ordering_stockpile;
  ProductionMaterial material;
  int amount_ordered;
  int amount_picked_up;
} replenishment_order_queue[MAX_REPLENISHMENT_QUEUE];

struct ReplenishmentOrder *get_replenishment_order(int id);
int next_fillable_replenishment_order(void);
void enqueue_replenishment_order(int stockpile_id, ProductionMaterial pm,
                                 int amount);
void complete_replenishment_order(int ro_id, int amount);
int outstanding_replenishment_orders(int stockpile_id, ProductionMaterial pm);

/* -------------
 * STATE
 * ------------- */

GameState game = {.cursor = (Vector){10, 10}};
GameState *new_game(void) { return &game; }

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

  case NONE: {
    strcpy(_material, "NONE");
    break;
  }

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

  case LONG_WIRES: {
    strcpy(_material, "WIRE");
    break;
  }

  case SMALL_BOWL: {
    strcpy(_material, "SMALL_BOWL");
    break;
  }

  case BOWL_OF_SHORT_WIRES: {
    strcpy(_material, "BOWL_OF_SHORT_WIRES");
    break;
  }

  case BOWL_OF_HEADLESS_PINS: {
    strcpy(_material, "BOWL_OF_HEADLESS_PINS");
    break;
  }
  }
  return _material;
}

/* -------------
 * JOBS
 * ------------- */

int job_queue_head = 0;
int job_queue_tail = 0;

char _job[50] = {0};

char *job_str(enum Job j) {
  switch (j) {

  case JOB_NONE: {
    strcpy(_job, "JOB_NONE");
    break;
  }

  case JOB_MAN_MACHINE: {
    strcpy(_job, "JOB_MAN_MACHINE");
    break;
  }

  case JOB_EMPTY_OUTPUT_BUFFER: {
    strcpy(_job, "JOB_EMPTY_OUTPUT_BUFFER");
    break;
  }

  case JOB_FILL_INPUT_BUFFER: {
    strcpy(_job, "JOB_FILL_INPUT_BUFFER");
    break;
  }

  case JOB_REPLENISH_STOCKPILE: {
    strcpy(_job, "JOB_REPLENISH_STOCKPILE");
    break;
  }
  }

  return _job;
}

void enqueue_job(ObjectReference o, enum Job job) {
  // printf("DEBUG_ENQUEUE_JOB: MachineID %d and job %s\n", machine_id,
  // job_str(job));
  job_queue[job_queue_tail].object = o;
  job_queue[job_queue_tail].job = job;
  job_queue_tail++;
  if (job_queue_tail > MAX_JOB_QUEUE) {
    job_queue_tail = 0;
  }
}

bool jobs_on_queue(void) { return job_queue_tail != job_queue_head; }

void debug_print_job_queue(void) {
  if (jobs_on_queue()) {
    printf("JOB QUEUE:\n");
    for (int i = job_queue_head; i < job_queue_tail; i++) {
      printf("\t%2d: %s\n", i, job_str(job_queue[i].job));
    }
  } else {
    printf("NO jobs on queue\n");
  }
}

struct JobQueueItem pop_job(void) {
  struct JobQueueItem j;
  if (job_queue_tail == job_queue_head) {
    j.object.id = -1;
    j.job = 0;
  } else {
    j.object = job_queue[job_queue_head].object;
    j.job = job_queue[job_queue_head].job;
    job_queue_head++;
  }
  return j;
}

/* -------------
 * REPLENISHMENT
 * ------------- */

void debug_print_ro(const struct ReplenishmentOrder *ro) {
  printf("RO for S%d, order of %d %s (%d picked up).\n", ro->ordering_stockpile,
         ro->amount_ordered, material_str(ro->material), ro->amount_picked_up);
}

void debug_print_ro_queue(void) {
  for (int i = 0; i < MAX_REPLENISHMENT_QUEUE; i++) {
    debug_print_ro(&replenishment_order_queue[i]);
  }
}

struct ReplenishmentOrder *get_replenishment_order(int id) {
  return &replenishment_order_queue[id];
}

int next_fillable_replenishment_order(void) {
  struct ReplenishmentOrder ro;
  int un_picked_amount;

  for (int i = 0; i < MAX_REPLENISHMENT_QUEUE; i++) {
    ro = replenishment_order_queue[i];
    un_picked_amount = ro.amount_ordered - ro.amount_picked_up;
    if ((un_picked_amount) > 0 &&
        find_stockpile_with_free_material((MaterialCount){ro.material, 1})) {
      return i;
    }
  }
  return -1;
}

void enqueue_replenishment_order(int stockpile_id, ProductionMaterial pm,
                                 int amount) {
  struct ReplenishmentOrder *ro;
  int outstanding;
  for (int i = 0; i < MAX_REPLENISHMENT_QUEUE; i++) {
    ro = &replenishment_order_queue[i];
    outstanding = ro->amount_ordered - ro->amount_picked_up;

    if (outstanding == 0) {
      ro->ordering_stockpile = stockpile_id;
      ro->material = pm;
      ro->amount_ordered = amount;
      ro->amount_picked_up = 0;
      return;
    }
  }
  printf("ERROR: Couldn't find a space to put replenishment order\n");
  exit(1);
}

void complete_replenishment_order(int ro_id, int amount) {
  struct ReplenishmentOrder *ro = get_replenishment_order(ro_id);
  ro->amount_ordered -= amount;
  ro->amount_picked_up -= amount;

  if (ro->amount_ordered == 0) {
    ro->material = NONE;
    ro->ordering_stockpile = -1;
  }
}

int outstanding_replenishment_orders(int stockpile_id, ProductionMaterial pm) {
  int amount = 0;
  struct ReplenishmentOrder ro;

  for (int i = 0; i < MAX_REPLENISHMENT_QUEUE; i++) {
    ro = replenishment_order_queue[i];
    if (ro.ordering_stockpile == stockpile_id && ro.material == pm) {
      amount += (ro.amount_ordered);
    }
  }
  return amount;
}

/* -------------
 * RECIPES
 * ------------- */

const Recipe wind_wire = {.name = WIND_WIRE,
                          .c_inputs = 2,
                          .inputs = {WASHED_IRON_WIRE_COIL, EMPTY_SPINDLE},
                          .inputs_count = {1, 1},
                          .c_outputs = 1,
                          .outputs = {SPINDLED_WIRE_COIL},
                          .outputs_count = {1},
                          .time = 1};

const Recipe pull_wire = {.name = PULL_WIRE,
                          .c_inputs = 1,
                          .inputs = {SPINDLED_WIRE_COIL},
                          .inputs_count = {1},
                          .c_outputs = 2,
                          .outputs = {LONG_WIRES, EMPTY_SPINDLE},
                          .outputs_count = {100, 1},
                          .time = 1};

const Recipe cut_wire = {.name = CUT_WIRE,
                         .c_inputs = 2,
                         .inputs = {LONG_WIRES, SMALL_BOWL},
                         .inputs_count = {10, 1},
                         .c_outputs = 1,
                         .outputs = {BOWL_OF_SHORT_WIRES},
                         .outputs_count = {1, 1},
                         .time = 1};

const Recipe grind_point = {.name = GRIND_POINT,
                            .c_inputs = 1,
                            .inputs = {BOWL_OF_SHORT_WIRES},
                            .inputs_count = {1},
                            .c_outputs = 1,
                            .outputs = {BOWL_OF_HEADLESS_PINS},
                            .outputs_count = {1},
                            .time = 1};

Recipe get_recipe_from_name(RecipeName rn) {
  switch (rn) {
  case PULL_WIRE:
    return pull_wire;
  case WIND_WIRE:
    return wind_wire;
  case CUT_WIRE:
    return cut_wire;
  case GRIND_POINT:
    return grind_point;
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

  case GRIND_POINT: {
    strcpy(_recipe, "GRIND_POINT");
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

  game.stockpiles[id] = (Stockpile){.id = id,
                                    .location = {x, y},
                                    .size = {w, h},
                                    .can_be_taken_from = false,
                                    .c_contents = 0,
                                    .contents = {0},
                                    .contents_count = {0},
                                    .c_required_material = 0,
                                    .required_material = {0},
                                    .required_material_count = {0},
                                    .io = -1,
                                    .attached_machine = -1};
  game.c_stockpile++;
  return id;
}

void debug_print_stockpile(const Stockpile *s) {
  printf("DEBUG_STOCKPILE: S%d. Can be taken from: %d. Has materials:\n", s->id,
         s->can_be_taken_from);
  for (int i = 0; i < s->c_contents; i++) {
    printf("\t%d %s (of which %d earmarked)\n", s->contents_count[i],
           material_str(s->contents[i]), s->contents_earmarks[i]);
  }
}

int index_of_material_in_stockpile(const Stockpile *s, ProductionMaterial p) {
  for (int i = 0; i < s->c_contents; i++) {
    if (s->contents[i] == p) {
      return i;
    }
  }
  return -1;
}

void add_required_material_to_stockpile(Stockpile *s, ProductionMaterial m,
                                        int amount) {
  s->required_material[s->c_required_material] = m;
  s->required_material_count[s->c_required_material] = amount;
  s->c_required_material++;
}

Stockpile *get_stockpile_by_id(int id) { return &game.stockpiles[id]; }

void add_material_to_stockpile(Stockpile *s, ProductionMaterial p, int count) {
  int i = index_of_material_in_stockpile(s, p);
  if (i == -1) {
    s->contents[s->c_contents] = p;
    s->contents_count[s->c_contents] = count;
    s->c_contents++;
  } else {
    s->contents_count[i] += count;
  }
}

int material_in_stockpile(Stockpile const *s, ProductionMaterial p) {
  int i = index_of_material_in_stockpile(s, p);
  return s->contents_count[i];
}

void earmark_material_in_stockpile(Stockpile *s, ProductionMaterial p,
                                   int count) {
  int i = index_of_material_in_stockpile(s, p);
  if (i == -1) {
    printf(
        "ERROR: Cannot earmark %s in stockpile %d: material is not present\n",
        material_str(p), s->id);
    exit(1);
  }

  s->contents_earmarks[i] += count;

  if (count > 0) {
    printf("DEBUG: Earmarking %d %s in S%d\n", count, material_str(p), s->id);
  } else {
    printf("DEBUG: UNEarmarking %d %s in S%d\n", -count, material_str(p),
           s->id);
  }
  debug_print_stockpile(s);
}

int free_material_in_stockpile(Stockpile const *s, ProductionMaterial p) {
  int i = index_of_material_in_stockpile(s, p);
  return (s->contents_count[i] - s->contents_earmarks[i]);
}

void update_replenishment_orders(Stockpile *s) {
  ProductionMaterial pm;
  int required;
  int current;
  int oro;
  int shortfall;

  for (int i = 0; i < s->c_required_material; i++) {
    pm = s->required_material[i];
    oro = outstanding_replenishment_orders(s->id, pm);
    if (oro > 0) {
      return;
    }

    required = s->required_material_count[i];
    current = material_in_stockpile(s, pm);
    shortfall = required - current;

    if (shortfall > 0) {
      printf("DEBUG: SP %d placed RO for %d %s", s->id, shortfall,
             material_str(pm));
      printf("\t(current %d; in_queue %d)\n", current, oro);
      enqueue_replenishment_order(s->id, pm, shortfall);
    }
  }
}

Stockpile *find_stockpile_with_material(MaterialCount mc) {
  for (int i = 0; i < game.c_stockpile; i++) {
    Stockpile s = game.stockpiles[i];
    if (s.can_be_taken_from && material_in_stockpile(&s, mc.material) > 0) {
      return &game.stockpiles[i];
    }
  }
  return NULL;
}
Stockpile *find_stockpile_with_free_material(MaterialCount mc) {
  for (int i = 0; i < game.c_stockpile; i++) {
    Stockpile s = game.stockpiles[i];
    if (s.can_be_taken_from &&
        free_material_in_stockpile(&s, mc.material) > 0) {
      return &game.stockpiles[i];
    }
  }
  return NULL;
}

void remove_material_from_stockpile(Stockpile *s, ProductionMaterial p,
                                    int amount_to_remove) {
  int idx = index_of_material_in_stockpile(s, p);
  if (idx == -1) {
    printf("ERROR: Material is not in stockpile");
    exit(1);
  }
  int available = s->contents_count[idx];

  if (available < amount_to_remove) {
    printf("ERROR: Not enough material in stockpile to remove");
    exit(1);
  }

  s->contents_count[idx] -= amount_to_remove;

  if (s->contents_count[idx] == 0) {
    for (int i = idx; i < s->c_contents; i++) {
      s->contents[i] = s->contents[i + 1];
      s->contents_count[i] = s->contents_count[i + 1];
    }
    s->c_contents--;
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

  case WIRE_GRINDER: {
    strcpy(_machine, "WIRE_GRINDER");
    break;
  }
  default:
    printf("ERROR: Unrecognized machine type\n");
    exit(1);
  }
  return _machine;
}

const RecipeName wire_winder_recipes[2] = {WIND_WIRE, -1};
const RecipeName wire_cutter_recipes[2] = {CUT_WIRE, -1};
const RecipeName wire_puller_recipes[2] = {PULL_WIRE, -1};

const RecipeName *possible_recipes(const Machine *m) {
  switch (m->type) {

  case WIRE_WINDER: {
    return wire_winder_recipes;
  }
  case WIRE_PULLER: {
    return wire_puller_recipes;
  }

  case WIRE_CUTTER: {
    return wire_cutter_recipes;
  }

  default:
    printf("ERROR: Unrecognized machine type\n");
    exit(1);
  }
}

Vector machine_size(enum MachineType mt) {
  switch (mt) {
  case WIRE_PULLER:
    return (Vector){2, 2};

  case WIRE_WINDER:
    return (Vector){2, 2};

  case WIRE_GRINDER:
    return (Vector){1, 1};

  case WIRE_CUTTER:
    return (Vector){1, 1};

  default:
    printf("ERROR: Unrecognized machine type\n");
    exit(1);
  }
}

int add_machine(enum MachineType type, int x, int y) {
  int id = game.c_machines;

  if (id > MAX_MACHINES) {
    printf("ERROR: Exceeded maximum machines\n");
    exit(1);
  }

  Vector v = machine_size(type);

  game.machines[id] = (Machine){
      .id = id,
      .type = type,
      .job_time_left = 0,
      .has_current_work_order = false,
      .c_output_buffer = 0,
      .worker = -1,
      .location = (Vector){x, y},
      .size = v,
      .output_stockpile = -1,
  };

  game.c_machines++;

  return id;
}

Machine *get_machine_by_id(int id) { return &game.machines[id]; }

void add_output_stockpile_to_machine(int mid, int sid) {
  Machine *m = get_machine_by_id(mid);
  m->output_stockpile = sid;

  Stockpile *s = get_stockpile_by_id(sid);
  s->io = OUTPUT;
  s->attached_machine = mid;
}

void add_input_stockpile_to_machine(int mid, int sid) {
  Machine *m = get_machine_by_id(mid);
  m->input_stockpile = sid;

  Stockpile *s = get_stockpile_by_id(sid);
  s->io = INPUT;
  s->attached_machine = mid;
}

void assign_machine_production_job(int id, RecipeName rn) {
  Recipe r = get_recipe_from_name(rn);
  Machine *m = get_machine_by_id(id);

  printf("DEBUG: machine %d assigned recipe %s\n", id, recipe_str(rn));

  m->has_current_work_order = true;
  m->active_recipe = r;
  m->job_time_left = r.time;
  enqueue_job((ObjectReference){O_MACHINE, id}, JOB_MAN_MACHINE);
}

void complete_production_job(Machine *m) {
  Worker *w = get_worker_by_id(m->worker);

  int num_outputs = m->active_recipe.c_outputs;
  m->has_current_work_order = false;
  m->worker = -1;
  m->c_output_buffer = num_outputs;

  for (int i = 0; i < num_outputs; i++) {
    m->output_buffer[i] = m->active_recipe.outputs[i];
    m->output_buffer_count[i] = m->active_recipe.outputs_count[i];
  }

  w->status = W_MOVING;
  w->job = JOB_EMPTY_OUTPUT_BUFFER;
  w->job_target = (ObjectReference){O_MACHINE, m->id};
  w->target = m->location;
  m->working = false;
}

int machine_has_input(Machine *m, ProductionMaterial p) {
  int count = 0;
  for (int i = 0; i < m->c_input_buffer; i++) {
    if (m->input_buffer[i] == p) {
      count += m->input_buffer_count[i];
    }
  }
  return count;
}

MaterialCount next_unfullfilled_material(Machine *m, Recipe r) {
  MaterialCount mc = {-1, 0};

  for (int i = 0; i < r.c_inputs; i++) {
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

int index_of_material_in_machine_input(Machine *m, ProductionMaterial p) {
  for (int i = 0; i < m->c_input_buffer; i++) {
    if (m->input_buffer[i] == p) {
      return i;
    }
  }
  return -1;
}

void start_production_job(Machine *m) {
  if (!machine_has_required_inputs(m, m->active_recipe)) {
    printf("ERROR: Trying to start production job, but don't have required "
           "materials\n");
    exit(1);
  }

  if (m->working) {
    printf("ERROR: Trying to start production job, machine already working!\n");
    exit(1);
  }

  printf("DEBUG: Starting Production Job, clearing inputs\n");

  Recipe r = m->active_recipe;
  ProductionMaterial pm;
  int required;
  int j;

  for (int i = 0; i < r.c_inputs; i++) {
    pm = r.inputs[i];
    required = r.inputs_count[i];
    j = index_of_material_in_machine_input(m, pm);
    m->input_buffer_count[j] -= required;
  }

  m->working = true;
}

void tick_machine(Machine *m) {
  if (m->has_current_work_order && m->worker >= 0 && m->working) {

    printf("DEBUG_TICK_MACHINE: machine is working...\n");
    if (m->job_time_left > 0) {
      m->job_time_left--;
    } else {
      complete_production_job(m);

      printf("DEBUG: Machine %d produced output: \n", m->id);

      for (int i = 0; i < m->c_output_buffer; i++)
        printf("\t%s: %d\n", material_str(m->output_buffer[i]),
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

  case W_CANT_PROCEED: {
    strcpy(_status, "W_CANT_PROCEED");
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
  printf("DEBUG: Summary for W%d: ", w->id);
  printf("\tJOB: %s", job_str(w->job));
  printf("\tSTATUS: %s", status_str(w->status));
  printf("\n");
  printf("\t target material: %d %s\n", w->target_count,
         material_str(w->target_material));
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
  Worker *w = get_worker_by_id(worker_id);

  char mb[256] = {0};

  switch (jq.job) {
  case JOB_MAN_MACHINE: {
    Machine *m = get_machine_by_id(jq.object.id);

    w->target = m->location;

    w->job = jq.job;
    w->job_target = jq.object;
    w->status = W_MOVING;

    m->worker = worker_id;

    printf("DEBUG: W:%d took job to to man machine %d\n", worker_id, m->id);
    sprintf(mb, "DEBUG: assigning W:%d to man machine %d\n", worker_id,
            m->id);
    add_message(mb);

    break;
  }
  case JOB_EMPTY_OUTPUT_BUFFER: {
    Machine *m = get_machine_by_id(jq.object.id);
    w->target = m->location;
    w->status = W_MOVING;
    w->job = jq.job;
    sprintf(mb, "DEBUG: assigning W%d to empty machine %d\n", worker_id,
            m->id);
    printf("DEBUG: W%d took job to empty machine %d\n", worker_id, m->id);
    add_message(mb);
    break;
  }
  case JOB_REPLENISH_STOCKPILE: {
    printf("ERROR: Replenishment job not implemented\n");
    exit(1);
  }
  default:
    printf("ERROR: Couldn't find job for job type %s\n", job_str(jq.job));
    exit(EXIT_FAILURE);
  }
}

void worker_pickup_output(Worker *w, Machine *m) {
  // printf("DEBUG: %d\n", m->outputs);
  // printf("DEBUG: %d\n", m->output_buffer[0]);
  m->c_output_buffer--;
  int o = m->c_output_buffer;
  ProductionMaterial mat = m->output_buffer[o];
  int mat_count = m->output_buffer_count[o];
  w->carrying = mat;
  w->carrying_count = mat_count;
  printf("DEBUG: W%d picked up %d %s from %d\n", w->id, mat_count,
         material_str(mat), m->id);
}

void worker_drop_material_at_machine(Worker *w, Machine *m) {
  // printf("DEBUG: %d\n", m->outputs);
  // printf("DEBUG: %d\n", m->output_buffer[0]);

  int i = m->c_input_buffer;
  m->input_buffer[i] = w->carrying;
  m->input_buffer_count[i] = w->carrying_count;

  w->carrying = -1;
  w->carrying_count = 0;

  m->c_input_buffer++;
  printf("DEBUG: W%d dropped %d %s to machine %d\n", w->id,
         m->input_buffer_count[i], material_str(m->input_buffer[i]), m->id);
}

void worker_pickup_from_stockpile(Worker *w, Stockpile *s, ProductionMaterial p,
                                  int count) {
  int mis = material_in_stockpile(s, p);
  // printf("DEBUG: W:%d getting %d %s from S:%d. There is %d\n", w->id, count,
  //        material_str(p), s->id, mis);
  //  printf("DEBUG: %d\n", m->output_buffer[0]);

  if (mis < count) {
    printf("ERROR: material not in stockpile\n");
    exit(1);
  } else {
    w->carrying_count = count;
    w->carrying = p;
    remove_material_from_stockpile(s, p, count);
    printf("DEBUG: W%d picked up %d %s from stockpile %d. There are %d left, "
           "of which %d are free.\n",
           w->id, count, material_str(p), s->id, material_in_stockpile(s, p),
           free_material_in_stockpile(s, p));
  }
}

void worker_drop_at_stockpile(Worker *w, Stockpile *s) {
  printf("%ld: W:%d dropped %d %s at S%d\n", game.turn, w->id,
         w->carrying_count, material_str(w->carrying), s->id);

  add_material_to_stockpile(s, w->carrying, w->carrying_count);

  w->carrying = NONE;
  w->carrying_count = 0;
}

void tick_worker(Worker *w) {
  if (w->status == W_CANT_PROCEED) {
    // if the worker is in the 'stuck' status, they should check if
    // they can now complete the assigned job. If circumstances have
    // changed, they should continue with their intended task.
    // Otherwise they should continue to hold.
    // TODO: not implemented yet.

    switch (w->job) {
    default:
      printf("ERROR: W:%d has 'Can't proceed' status with job %s\n", w->id,
             job_str(w->job));
      exit(1);
    }

    return;
  }

  // If the worker isn't at the destination, move towards the
  // destination

  if (!vec_equal(w->location, w->target)) {
    w->location = vec_move_towards(w->location, w->target);
    return;
  }

  // Worker is at its destination
  switch (w->job) {
  case JOB_EMPTY_OUTPUT_BUFFER: {

    if (w->status == W_CARRYING) {
      Machine *m = get_machine_by_id(w->job_target.id);
      Stockpile *s = get_stockpile_by_id(m->output_stockpile);
      worker_drop_at_stockpile(w, s);

      if (m->c_output_buffer == 0) {
        w->status = W_IDLE;
        w->job = JOB_NONE;
        w->target = (Vector){15, 0};
      } else {
        w->status = W_MOVING;
        w->target = m->location;
      }

    } else if (w->status == W_MOVING) {
      Machine *m = get_machine_by_id(w->job_target.id);
      Stockpile *s = get_stockpile_by_id(m->output_stockpile);
      worker_pickup_output(w, m);
      w->status = W_CARRYING;
      w->target = s->location;
    } else {
      printf("ERROR: Unhandled worker status %d for empty output buffer job\n",
             w->status);
    }

  } break;

  case JOB_FILL_INPUT_BUFFER: {
    Machine *m = get_machine_by_id(w->job_target.id);
    Stockpile *s = get_stockpile_by_id(m->input_stockpile);

    if (w->status == W_CARRYING) {
      worker_drop_material_at_machine(w, m);
      MaterialCount mc = next_unfullfilled_material(m, m->active_recipe);
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
        printf("DEBUG: Machine has what it needs, switching to producing\n");
        m->worker = w->id;
        start_production_job(m);
        w->job = JOB_MAN_MACHINE;
        w->status = W_PRODUCING;
      }
    } else if (w->status == W_MOVING) {
      // The worker has reached the input stockpile of the machine and will try
      // to pick up the material required.
      MaterialCount mc = next_unfullfilled_material(m, m->active_recipe);
      int mis = material_in_stockpile(s, mc.material);

      if (mis >= mc.count) {
        worker_pickup_from_stockpile(w, s, mc.material, mc.count);
        w->target = m->location;
        w->status = W_CARRYING;
      } else {
        printf("DEBUG: W%d tried to pick up material from stockpile, but "
               "there wasn't enough in it.\n",
               w->id);
        w->status = W_CANT_PROCEED;
      }
    } else {
      printf("ERROR: Unhandled worker status %d for fill input buffer job\n",
             w->status);
    }

    break;

  case JOB_MAN_MACHINE: {
    if (w->status == W_PRODUCING) {
      return;
    }
    Machine *m = get_machine_by_id(w->job_target.id);

    if (machine_has_required_inputs(m, m->active_recipe)) {
      start_production_job(m);
      w->status = W_PRODUCING;
      return;
    }

    w->job = JOB_FILL_INPUT_BUFFER;
    w->status = W_MOVING;
    Stockpile *s = get_stockpile_by_id(m->input_stockpile);
    w->target = s->location;
  }

  break;

  case JOB_REPLENISH_STOCKPILE: {
    if (w->status == W_MOVING) {
      Stockpile *s = get_stockpile_by_id(w->job_target_secondary.id);
      // worker reached stockpile and will pick up (and un-earmark) material
      earmark_material_in_stockpile(s, w->target_material, (-w->target_count));
      worker_pickup_from_stockpile(w, s, w->target_material, w->target_count);

      w->target = get_stockpile_by_id(w->job_target.id)->location;
      w->status = W_CARRYING;

      w->target_material = NONE;
      w->target_count = 0;
      w->job_target_secondary.object_type = O_NOTHING;
      return;
    }

    if (w->status == W_CARRYING) {
      Stockpile *s = get_stockpile_by_id(w->job_target.id);
      complete_replenishment_order(w->job_id, w->carrying_count);
      worker_drop_at_stockpile(w, s);

      w->job = JOB_NONE;
      w->job_id = -1;
      w->job_target.object_type = O_NOTHING;
      w->status = W_IDLE;

      return;
    }

    break;
  }

  case JOB_NONE: {
    // TODO: randomly wander around
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
  // check stockpiles for missing materials and, if necessary issue
  // replenishment order
  for (int i = 0; i < game.c_stockpile; i++) {
    update_replenishment_orders(&game.stockpiles[i]);
  }

  // take replenishment jobs
  int idle_worker = first_idle_worker();
  int fro = next_fillable_replenishment_order();

  while (idle_worker >= 0 && fro >= 0) {
    Worker *w = get_worker_by_id(idle_worker);
    struct ReplenishmentOrder *ro = get_replenishment_order(fro);
    Stockpile *s =
        find_stockpile_with_material((MaterialCount){ro->material, 1});

    if (!s) {
      printf(
          "ERROR: through there was an FRO, but couldn't find a stockpile\n");
      exit(1);
    }

    int available = free_material_in_stockpile(s, ro->material);
    int desire = ro->amount_ordered;
    int pickup = (available < desire) ? available : desire;

    printf("DEBUG: W:%d is taking replenishment job %d:\n\t", w->id, fro);
    debug_print_ro(ro);
    printf("\tdesire: %d, available: %d\n", desire, available);

    ro->amount_picked_up += pickup;
    earmark_material_in_stockpile(s, ro->material, pickup);

    w->job = JOB_REPLENISH_STOCKPILE;
    w->job_id = fro;
    w->status = W_MOVING;
    w->job_target = (ObjectReference){O_STOCKPILE, ro->ordering_stockpile};
    w->job_target_secondary = (ObjectReference){O_STOCKPILE, s->id};

    w->target = s->location;
    w->target_material = ro->material;
    w->target_count = pickup;

    idle_worker = first_idle_worker();
    fro = next_fillable_replenishment_order();
  }

  // take other jobs
  while (idle_worker >= 0 && jobs_on_queue()) {
    worker_take_job(idle_worker, pop_job());
    idle_worker = first_idle_worker();
  }

  for (int i = 0; i < game.c_machines; i++) {
    tick_machine(&game.machines[i]);
  }

  for (int i = 0; i < game.c_workers; i++) {
    tick_worker(&game.workers[i]);
  }

  game.turn++;
}
