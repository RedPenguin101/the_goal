#include "vector.h"

#define MAX_WORKERS 10
#define MAX_STOCKPILES 50
#define MAX_MACHINES 10
#define MESSAGE_BUFFER_SIZE 10
#define MESSAGE_MAX_SIZE 256

enum Job { NONE, MAN_MACHINE, EMPTY_OUTPUT_BUFFER, FILL_INPUT_BUFFER };

typedef enum RecipeName {
  WIND_WIRE,
  PULL_WIRE,
  CUT_WIRE,
} RecipeName;

typedef enum ProductionMaterial {
  WASHED_IRON_WIRE_COIL,
  EMPTY_SPINDLE,
  SPINDLED_WIRE_COIL,
  WIRE
} ProductionMaterial;

typedef struct Recipe {
  enum RecipeName name;
  int i_count;
  ProductionMaterial inputs[10];
  int inputs_count[10];
  int o_count;
  ProductionMaterial outputs[10];
  int outputs_count[10];
  int time;
} Recipe;

enum MachineType { WIRE_WINDER, WIRE_PULLER, WIRE_CUTTER };

typedef struct Machine {
  int id;
  char name[10];
  enum MachineType mtype;
  int job_time_left;
  bool current_work_order;
  Recipe recipe;

  int inputs;
  ProductionMaterial input_buffer[10];
  int input_buffer_count[10];

  int outputs;
  ProductionMaterial output_buffer[10];
  int output_buffer_count[10];

  int worker;

  Vector location;
  Vector size;
  int output_stockpile;
  int input_stockpile;
} Machine;

enum WorkerStatus { W_IDLE, W_CARRYING, W_MOVING, W_PRODUCING };

typedef struct Worker {
  int id;
  enum WorkerStatus status;
  Vector location;
  Vector target;
  enum Job job;
  Machine *machine;
  ProductionMaterial carrying;
  int carrying_count;
} Worker;

typedef struct Stockpile {
  int id;
  Vector location;
  Vector size;
  int things_in_stockpile;
  ProductionMaterial contents[10];
  int content_count[10];
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

int add_stockpile(int x, int y, int w, int h);
void add_material_to_stockpile(int sid, ProductionMaterial p, int count);

int add_machine(enum MachineType type, char *name, int x, int y);
void add_output_stockpile_to_machine(int machine_id, int stockpile_id);
void add_input_stockpile_to_machine(int machine_id, int stockpile_id);

void assign_machine_production_job(int machine_id, RecipeName rn);

int add_worker(void);
void tick_game(void);
