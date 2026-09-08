# hfsm

A small, header-only hierarchical finite state machine library for C++14, designed for fixed-cycle control systems.

`hfsm` favors **strict hierarchy, compile-time structure, deterministic transition priority, and step-driven execution** over a general-purpose event-driven model.

## Table of Contents

* [Design Philosophy](#design-philosophy)

  * [Strict Hierarchy](#strict-hierarchy)
  * [Compile-Time Structure](#compile-time-structure)
  * [Enum-Based State Identity](#enum-based-state-identity)
  * [Step-Driven Execution](#step-driven-execution)
  * [Guard-Driven Transitions](#guard-driven-transitions)
* [Features](#features)
* [Quick Start](#quick-start)
* [Hierarchical State Machines](#hierarchical-state-machines)
* [Execution Semantics](#execution-semantics)
* [Pseudo States](#pseudo-states)
* [Object and Lifetime Semantics](#object-and-lifetime-semantics)
* [Requirements](#requirements)
* [Installation](#installation)
* [Building Tests and Examples](#building-tests-and-examples)
* [Examples](#examples)
* [License](#license)

## Design Philosophy

### Strict Hierarchy

Each state machine represents an independent architectural layer with its own:

* state enum;
* initial state;
* transition table;
* guards and actions;
* lifecycle callbacks.

A nested state machine appears to its parent as **one state**.

```text
Vehicle
├── Off
├── Manual
└── Autonomous
    ├── Standby
    ├── Driving
    └── Parking
```

The parent can transition from:

```text
Manual -> Autonomous
```

but its transition table does not directly target:

```text
Manual -> Autonomous::Parking
```

Entering `Autonomous` always enters through the `Autonomous` machine's own `initial_state`.

This creates a strict architectural boundary between levels. A parent only needs to include the child state-machine definition; the child's internal states and transition logic remain inside the child module.

For a large project, this naturally supports separation such as:

```text
vehicle/
  vehicle_state_machine.h

autonomous/
  autonomous_state_machine.h

parking/
  parking_state_machine.h
```

Different layers can therefore be developed and tested independently, which is useful for large systems and team-based development.

### Compile-Time Structure

The state-machine architecture is defined statically with templates:

```cpp
using transition_table = std::tuple<
    transition<
        IdleState,
        RunningState,
        &Machine::should_start,
        &Machine::start>,
    transition<
        RunningState,
        IdleState,
        &Machine::should_stop,
        &Machine::stop>>;
```

Template metaprogramming derives the state set and hierarchy from the initial state and transition table.

States and nested state machines are stored directly by value:

```text
state_machine
├── StateMachineDef
├── StateA
├── StateB
└── ChildStateMachine
    ├── ChildStateA
    └── ChildStateB
```

`hfsm` does not dynamically allocate state objects or transition nodes.

The machine structure and storage requirements are therefore fixed when the type is compiled.

### Enum-Based State Identity

Every state has both:

* a C++ type containing its behavior;
* an enum value representing its architectural identity.

```cpp
enum class PlayerState {
  Stopped,
  Playing,
};

struct Playing : hfsm::state<Playing> {
  void on_entry() {}
  void on_update() {}
  void on_exit() {}
};

using PlayingState =
    state_entry<Playing, PlayerState::Playing>;
```

This intentionally separates behavior from state identity:

```text
Playing                    PlayerState::Playing
   │                                │
   │ implementation                 │ architecture
   │                                │
   ├── on_entry()                   ├── transition table
   ├── on_update()                  ├── current_state()
   └── on_exit()                    └── external state representation
```

The active state can therefore be inspected directly:

```cpp
if (sm.current_state() == PlayerState::Playing) {
  // ...
}
```

### Step-Driven Execution

`hfsm` is designed for state machines executed at a fixed or controlled frequency.

```cpp
hfsm::state_machine<Vehicle> sm;
sm.start();

while (running) {
  update_inputs();
  sm.step();
  wait_for_next_cycle();
}
```

A normal transition is evaluated only when `step()` is called.

Unlike state machines that continuously follow available transitions until the system becomes stable, `hfsm` advances the active hierarchy **one step at a time**.

For ordinary states, at most one transition is taken from the active hierarchy during a root `step()`.

Pseudo states are the exception: they are transient and may resolve multiple transitions immediately within the same step.

This execution model is well suited to periodic control systems such as robotics, automation, and vehicle-control software.

### Guard-Driven Transitions

`hfsm` intentionally does not provide an event system.

Transitions are selected directly by guards:

```cpp
bool should_enter_autonomous() {
  return autonomous_requested &&
         localization_ready &&
         planning_ready;
}
```

and declared as:

```cpp
transition<
    ManualState,
    AutonomousState,
    &Vehicle::should_enter_autonomous,
    &Vehicle::enter_autonomous>
```

In large systems, representing every condition as a separate event can create a large event vocabulary and additional event-management complexity.

With guard-driven transitions, the current system state itself determines whether a transition is available.

External events can still be represented as ordinary application data:

```cpp
bool button_pressed;
bool command_received;
bool timeout;
```

#### Transition Priority

Several transition conditions may be true at the same time.

Priority is expressed directly by the order of the transition table:

```cpp
using transition_table = std::tuple<
    transition<
        DrivingState,
        EmergencyState,
        &Machine::emergency_required,
        &Machine::enter_emergency>,

    transition<
        DrivingState,
        ParkingState,
        &Machine::parking_requested,
        &Machine::start_parking>>;
```

Transitions are checked from top to bottom.

If both guards are `true`, the transition to `EmergencyState` wins.

The order of `transition_table` is therefore part of the state-machine behavior.

## Features

* Header-only C++14 library
* Strict hierarchical state machines
* Compile-time state and transition structure
* No dynamic allocation by the library
* Enum identity for every state
* Guard-based transitions
* Ordered transition priority
* Explicit single-step execution
* Transition actions
* State-level lifecycle callbacks
* State-machine-level lifecycle callbacks
* Transient pseudo states
* Arbitrary hierarchy depth
* Runtime state inspection
* Access to stored state instances
* Copyable and movable runtime snapshots
* Empty transition tables
* CMake package support

## Quick Start

```cpp
#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

#include <tuple>

enum class PlayerState {
  Stopped,
  Playing,
};

struct Context {
  bool play_requested{false};
  bool stop_requested{false};
};

Context context;

struct Stopped : hfsm::state<Stopped> {
  void on_entry() {}
  void on_update() {}
  void on_exit() {}
};

struct Playing : hfsm::state<Playing> {
  void on_entry() {}
  void on_update() {}
  void on_exit() {}
};

struct Player
    : hfsm::state_machine_def<Player, PlayerState> {
  using StoppedState =
      state_entry<Stopped, PlayerState::Stopped>;

  using PlayingState =
      state_entry<Playing, PlayerState::Playing>;

  bool should_play() {
    return context.play_requested;
  }

  bool should_stop() {
    return context.stop_requested;
  }

  void start_playback() {}
  void stop_playback() {}

  using initial_state = StoppedState;

  using transition_table = std::tuple<
      transition<
          StoppedState,
          PlayingState,
          &Player::should_play,
          &Player::start_playback>,
      transition<
          PlayingState,
          StoppedState,
          &Player::should_stop,
          &Player::stop_playback>>;
};

int main() {
  hfsm::state_machine<Player> sm;

  sm.start();

  context.play_requested = true;
  sm.step();

  return sm.current_state() == PlayerState::Playing ? 0 : 1;
}
```

The basic lifecycle is:

```text
construct
    │
    v
start()
    │
    v
step()
    │
    v
step()
    │
    v
...
```

`start()` enters the initial state and may only be called once.

## Hierarchical State Machines

A state can itself be another state-machine definition.

```cpp
enum class AutonomousState {
  Standby,
  Driving,
};

struct Autonomous
    : hfsm::state_machine_def<Autonomous, AutonomousState> {
  // ...
};
```

The parent uses that machine exactly like another state:

```cpp
enum class VehicleState {
  Manual,
  Autonomous,
};

struct Vehicle
    : hfsm::state_machine_def<Vehicle, VehicleState> {
  using ManualState =
      state_entry<Manual, VehicleState::Manual>;

  using AutonomousStateEntry =
      state_entry<Autonomous, VehicleState::Autonomous>;

  // ...
};
```

When the parent enters `Autonomous`, the nested machine enters through its own initial state.

For:

```text
Vehicle
└── Autonomous
    └── Standby
```

entry proceeds from outer to inner:

```text
Autonomous::on_entry()
Standby::on_entry()
```

exit proceeds from inner to outer:

```text
Standby::on_exit()
Autonomous::on_exit()
```

## Execution Semantics

Each state machine can define machine-level callbacks:

```cpp
void on_entry();
void on_update();
void on_exit();
```

States provide the same lifecycle callbacks.

### `start()`

For the root machine:

```cpp
sm.start();
```

the order is:

```text
Machine::on_entry()
initial_state::on_entry()
```

### `step()`

Each machine level first updates itself and then evaluates its transitions:

```text
Machine::on_update()
        │
        v
evaluate transitions
        │
        ├── transition found
        │       │
        │       └── take transition
        │
        └── no transition
                │
                └── update active child/state
```

For a hierarchy with no available transition:

```text
Vehicle::on_update()
        │
        v
Autonomous::on_update()
        │
        v
Driving::on_update()
```

If `Vehicle` takes a transition, the update is not propagated into `Autonomous`.

Outer layers therefore have priority over their active inner hierarchy.

### Transition

For:

```text
A -> B
```

the order is:

```text
A::on_exit()
transition action
B::on_entry()
```

Transitions whose source is the current state are evaluated in declaration order. The first guard returning `true` wins.

If no transition is taken, the active state receives `on_update()`.

The initial state does not need to appear in the transition table.

An empty transition table is valid:

```cpp
using transition_table = std::tuple<>;
```

## Pseudo States

A pseudo state represents a transient decision point:

```cpp
struct Decision : hfsm::pseudo_state<Decision> {};
```

For example:

```text
              +--> Fast
              |
Idle --> Decision
              |
              +--> Slow
```

A single:

```cpp
sm.step();
```

may perform:

```text
Idle -> Decision -> Fast
```

within the same step.

Pseudo states may receive `on_entry()` and `on_exit()`, but never remain active long enough to receive `on_update()`.

Entering a pseudo state with no available outgoing transition throws `std::logic_error`.

Pseudo-state transition cycles are also detected.

## Object and Lifetime Semantics

The complete hierarchy is constructed with the root `state_machine` and stored by value.

State-machine definitions and all states must therefore be default constructible.

State objects persist for the lifetime of the machine. Re-entering a state does not reconstruct it, so state-local data is preserved unless explicitly reset.

```cpp
struct Running : hfsm::state<Running> {
  int counter{0};

  void on_update() {
    ++counter;
  }
};
```

A stored state can be accessed with:

```cpp
auto& running = sm.get_state<Running>();
```

### Copy and Move

A state machine has value semantics.

Copying it copies the complete runtime snapshot, including:

* whether it has started;
* active states;
* nested-machine states;
* state-machine-definition data;
* active and inactive state data.

```cpp
auto copy = sm;
```

The copy can then execute independently according to the normal copy semantics of its contained objects.

Moving transfers the runtime snapshot:

```cpp
auto moved = std::move(sm);
```

After a move, the source machine's runtime state is unspecified.

Copy and move operations are ordinary C++ object operations and do **not** invoke FSM `on_entry()`, `on_update()`, or `on_exit()` callbacks.

Their availability naturally depends on whether the contained states and state-machine definitions support the corresponding C++ operations.

### Application Data

`hfsm` intentionally does not prescribe a context or dependency-injection model.

Application data may be provided through whichever mechanism fits the surrounding architecture. Guards simply inspect that data when transitions are evaluated.

## Requirements

* C++14-compatible compiler
* CMake 3.20 or later when using the provided build system

### Platform Support

Version 1.0 is developed and tested on Linux with:

* GCC
* Clang

The implementation uses standard C++14 and does not intentionally rely on compiler-specific language extensions.

Other conforming compilers and platforms may work, but they are not officially tested or supported in v1.0.

## Installation

`hfsm` is header-only and exports the CMake target:

```cmake
hfsm::hfsm
```

### `add_subdirectory`

```cmake
add_subdirectory(path/to/hfsm)

target_link_libraries(
  your_target
  PRIVATE
    hfsm::hfsm
)
```

### Install and `find_package`

```bash
cmake -S . -B build
cmake --install build --prefix /path/to/install
```

Then:

```cmake
find_package(hfsm CONFIG REQUIRED)

target_link_libraries(
  your_target
  PRIVATE
    hfsm::hfsm
)
```

Configure the consumer with:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/install
```

## Building Tests and Examples

Tests and examples are disabled by default.

```bash
cmake -S . -B build \
  -DHFSM_BUILD_TESTS=ON \
  -DHFSM_BUILD_EXAMPLES=ON

cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Examples

The repository contains three examples:

* `examples/simple_state_machine.cpp`
  Basic states, guards, actions, and transitions.

* `examples/hierarchical_state_machine.cpp`
  Strict hierarchical composition and recursive lifecycle behavior.

* `examples/pseudo_state.cpp`
  Transient pseudo-state decision logic.

## License

`hfsm` is released under the MIT License. See `LICENSE` for details.
