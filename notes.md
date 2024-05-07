# Simulating a Machine

A machine turns inputs into outputs. It has a series of 'recipes'.
Each recipe requires a certain amount of inputs of various types and
produces an amount of outputs of various types. To run a 'batch' of a
recipe takes an amount of time. That includes the loading of the
materials, the operation of the machine, and the unloading of the
materials. This is subject to stochastic variation.

A recipe for a particular machine can thus be described as:

* set of inputs required
* set of outputs produced (as a function of inputs)
* a distribution of the amount of time for a batch to run
* the likelyhood that a particular output from the process will be
  defective
* A set up and a teardown time to prepare the machine to run the
  recipe

The machine itself has:

* A set of recipes
* A chance of breaking down
* [instance] an input and output inventory, described in floor space
  (area)
* The number of workers required to operate it

So the matchstick game could be considered as 6 machines running the
following recipe:

* input required: 1 match
* batch size: 1 match
* batch limit: 6
* operation time distribution function: U(1,6)

With each machine having an infinite input and output inventory.

The other operation to be considered is the process of moving
inventory from one machine's output inventory to another's output
inventory. This is done by an agent, which has a capacity and a
speed.In the matchstick game we operate on the assumption that the
capacity is unlimited and the speed is (effectively) infinite, such
that the time to transfer inventory from input to output is
instantaneous. We also assume there are enough agents for each move
job (i.e. no. of machines +1)

## The Pin factory

https://www.adamsmithworks.org/pin_factory.html

Room 1:

* Wire storage
* Washing and wrapping station
* Winding station

Room 2:

* Pulling lengths
* Cutting lengths1
* Cutting lengths2
* Grinding the point

Room 3:

* Bleach and shake
* Drying 1 - Rollings
* Drying 2 - shaking in valve
* Drying 3 - shaking in sheepskin
* Boiling the metal for the heads
* Molding the pin heads
* Striking the pins, attaching the pin heads

```
Material: Iron Wire Coil
    A coil of metal wire
    Has a length.
    Can be stored in a bin.
    
Storage: Bin
Storage: Small Bowl
Storage: Large Bowl
Storage: Heat Safe Container
    
Machine: Washing station
    Has a recipe Wash Wire which
        When manned by 1 person
        Takes a single Iron Wire Coil
        Outputs a Washed Iron Wire Coil of the same length
    
Machine: Wire Winder
    Has a recipe Wind Wire which
        When manned by a single person
            And having an Empty Spindle
        Takes a single Washed Iron Wire Coil
        Ouputs a Spindled Wire Coil of the same length
        
Machine: Wire Puller
    Has a recipe Pull Wire X which
        When manned by a single person
        Takes a single Spindled Wire Coil of Length L
        Outputs (L//X) Wires of Length X
        
Machine: Wire Cutter
    Has a recipe Cut Wire X which
        When manned by a single person
        Takes a batch of size B of Wires of Length L
        Outputs B*(L//X) Wires of Length X
            Which are stored in a Small Bowl
        
Machine: Grinder
    Has a recipe Grind Point which
        When manned by two people
        Takes a batch of size B of Wires
        Outputs a batch of size B of Unheaded Pins
            Which are stored in a Small Bowl or a Large Bowl
        
Machine: Bleaching station
    Has a recipe Bleach Pins which
        When manned by one person
        Takes a batch of Unheaded Pins
        Outputs a batch of Wet Bleached Unheaded Pins of the same size
            Which are stored in a Large Bowl

Machine: Sheepskin Dryer
    Has a recipe Dry Pins which
        When manned by two people
        Takes a batch of Wet Bleached Unheaded Pins
        Outputs a batch of Bleached Unheaded Pins
            Which are stored in a large bowl

Machine: Metal Melter
    Has a recipe Melt Tin which
        When manned by one person
        Takes a batch of Tin
        Outputs a batch of Molten Tin of the same weight
            Which are stored in a Heat Safe Container
        
Machine Pin Head Mold
    Has a recipe Mold Pin Heads
        when manned by one person
            And having an Empty Mold
        Takes a batch of Molten Tin
        Outputs a batch of Pin Heads
            Which are Stored in a Small Bowl

Machine Pin Striker (6 stations)
    Has a recipe Strike Pins
        which when manned by one person
        Takes between 1-10 Bleached Unheaded Pins
            and the same number of Pin Heads
        Outputs the same number of Finished Pins
            Which are Stored in a Small Bowl
```

## Worker decision loop:

JOBS (things worker takes off the job queue):
- none
- man machine
- fill input buffer
- empty output buffer

STATUS:
- moving
- fetching
- carrying
- producing
- idle

```
If worker has a target destination which is not their current location, move towards the destination

If the worker is at the destination:
    If worker job is empty output buffer
        If status is carrying:
            Drop what you're carrying at the stockpile:
            If machine output buffer is empty:
                set job to none, status to idle
            If machine output buffer is not empty:
                set status to moving, set target to machine location
    
        If status is moving:
            Pick up the first thing in the output
            Set the target to the output stockpile
            Set status to carrying
    
    If worker job is fill input buffer:
        If status is carrying:
            Drop what you're carrying at the machine
            If the machine has all required inputs:
                set job to man machine, set status to producing
            If the machine doesn't have all required inputs
                check stockpile has required material
                if it does
                    set status to moving, set target to stockpile
                if it doesn't
                    put job on hold
                    set job to none
                    set status to idle
  
        If status is moving:
            Pickup the next required input
            set the target to the machine
            set status to carrying
    
    If worker job is man machine
        If the status is producing
            Do nothing
        If the machine has all required inputs
            set the status to producing
        If the machine doesn't have all required inputs:
            set the job to fill input buffer
            set the status to moving
            set the target to the machine's input buffer.
```

## Replenishment orders

* Every turn, each stockpile should check what required materials it
  is short of.
* It should compare that list to the list of outstanding replenishment
  orders - including the ones that are marked as in progress.
* If the quantity of the of the shortfall of material is less than the
  outstanding replenishment material for that order, add a
  replenishment order for the difference

* Every turn, idle workers should check try to find a replenishment
  order that can be fulfilled - even partially.
* If they find one, take the job to move the materials.
* Mark the order with the approriate 'in-train' amount, and reduce the amount
* Go to the source stockpile and move the items to the target
* Reduce the replenishment order's amount_ordered by the amount delivered
* 
