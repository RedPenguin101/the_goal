#include "vector.h"

#define MAX_WORKERS 10
#define MAX_STOCKPILES 50
#define MAX_MACHINES 10
#define MESSAGE_BUFFER_SIZE 10
#define MESSAGE_MAX_SIZE 256

typedef enum ObjectType {
  O_NOTHING,
  O_MACHINE,
  O_WORKER,
  O_STOCKPILE
} ObjectType;

typedef struct ObjectReference {
  ObjectType object_type;
  int id;
} ObjectReference;

enum Job {
  JOB_NONE,
  JOB_MAN_MACHINE,
  JOB_EMPTY_OUTPUT_BUFFER,
  JOB_FILL_INPUT_BUFFER,
  JOB_REPLENISH_STOCKPILE
};

typedef enum RecipeName {
  WIND_WIRE,
  PULL_WIRE,
  CUT_WIRE,
  GRIND_POINT
} RecipeName;

typedef enum ProductionMaterial {
  NONE,
  WASHED_IRON_WIRE_COIL,
  EMPTY_SPINDLE,
  SPINDLED_WIRE_COIL,
  LONG_WIRES,
  SMALL_BOWL,
  BOWL_OF_SHORT_WIRES,
  BOWL_OF_HEADLESS_PINS
} ProductionMaterial;

typedef struct Recipe {
  enum RecipeName name;

  int c_inputs;
  ProductionMaterial inputs[10];
  int inputs_count[10];

  int c_outputs;
  ProductionMaterial outputs[10];
  int outputs_count[10];

  int time;
} Recipe;

enum MachineType { WIRE_WINDER, WIRE_PULLER, WIRE_CUTTER, WIRE_GRINDER };

typedef struct Machine {
  int id;
  char name[10];
  enum MachineType type;
  int job_time_left;
  bool has_current_work_order;
  Recipe active_recipe;

  int c_input_buffer;
  ProductionMaterial input_buffer[10];
  int input_buffer_count[10];

  int c_output_buffer;
  ProductionMaterial output_buffer[10];
  int output_buffer_count[10];

  int worker;

  Vector location;
  Vector size;
  int output_stockpile;
  int input_stockpile;
} Machine;

enum WorkerStatus { W_IDLE, W_CANT_PROCEED, W_CARRYING, W_MOVING, W_PRODUCING };

typedef struct Worker {
  int id;
  enum WorkerStatus status;
  Vector location;
  Vector target;
  enum Job job;
  ObjectReference job_target;
  ProductionMaterial carrying;
  int carrying_count;
} Worker;

typedef struct Stockpile {
  int id;
  Vector location;
  Vector size;
  bool can_be_taken_from;

  int c_contents;
  ProductionMaterial contents[10];
  int contents_count[10];

  int c_required_material;
  ProductionMaterial required_material[10];
  int required_material_count[10];
} Stockpile;

typedef struct GameState {
  int c_machines;
  Machine machines[MAX_MACHINES];
  int c_workers;
  Worker workers[MAX_WORKERS];
  int c_stockpile;
  Stockpile stockpiles[MAX_STOCKPILES];
  long turn;
  char message_buffer[MESSAGE_BUFFER_SIZE][MESSAGE_MAX_SIZE];
  int message_head;
  Vector cursor;
} GameState;

GameState *new_game(void);

char *material_str(ProductionMaterial m);
char *machine_str(enum MachineType m);
char *recipe_str(RecipeName rn);
char *job_str(enum Job j);
void debug_print_job_queue();

int add_stockpile(int x, int y, int w, int h);
void add_material_to_stockpile(Stockpile *s, ProductionMaterial p, int count);
void add_required_material_to_stockpile(Stockpile *s, ProductionMaterial p,
                                        int count);
Stockpile *get_stockpile_by_id(int id);

int add_machine(enum MachineType type, char *name, int x, int y);
const RecipeName *possible_recipes(const Machine *m);
void add_output_stockpile_to_machine(int machine_id, int stockpile_id);
void add_input_stockpile_to_machine(int machine_id, int stockpile_id);
Machine *get_machine_by_id(int id);

void assign_machine_production_job(int machine_id, RecipeName rn);

Worker *get_worker_by_id(int id);
int add_worker(void);

ObjectReference object_under_point(int x, int y);
void tick_game(void);
