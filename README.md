# hfsm

A small, header-only hierarchical finite state machine library for C++14, designed for fixed-cycle control systems.

`hfsm` is built around a few deliberate constraints:

* hierarchy boundaries are strict;
* state-machine structure is defined statically;
* every state has an explicit enum identity;
* execution advances one step at a time;
* transitions are driven by guards rather than events.

The goal is not to provide every possible state-machine feature. The goal is to make large state-machine architectures explicit, predictable, and easy to divide across modules and teams.

## Design Philosophy

### Strict hierarchy

Each state machine is an independent architectural layer with its own:

* state enum;
* initial state;
* transition table;
* guards and actions;
* lifecycle callbacks.

A nested state machine appears to its parent as a single state.

For example:

```text
Vehicle
├── Off
├── Manual
└── Autonomous
    ├── Standby
    ├── Driving
    └── Parking
```

The `Vehicle` layer knows that `Autonomous` exists, but it does not control transitions between `Standby`, `Driving`, and `Parking`.

The parent can only transition into `Autonomous` as a whole:

```text
Manual -> Autonomous
```

When `Autonomous` is entered, it always enters through its own `initial_state`.

The parent does not jump directly to:

```text
Manual -> Autonomous::Parking
```

This keeps hierarchy boundaries explicit.

In source code, a parent state machine only needs to include the definition of its child state machine and use that child machine as one of its states. The child's internal states and transition logic remain inside the child module.

This makes large systems easier to decompose:

```text
vehicle/
  vehicle_state_machine.h

autonomous/
  autonomous_state_machine.h

parking/
  parking_state_machine.h
```

Each layer can be implemented and tested independently while exposing only the state-machine definition required by its parent.

This model is particularly useful for large control architectures where different subsystems may be owned by different modules or teams.

---

### Static structure

The state-machine architecture is defined using templates:

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

Template metaprogramming is used to derive the state hierarchy from the initial state and transition table at compile time.

The runtime state objects and nested state machines are stored directly by value.

Conceptually:

```text
state_machine
├── MachineDef
├── StateA
├── StateB
├── ChildStateMachine
│   ├── ChildStateA
│   └── ChildStateB
└── fixed transition table
```

`hfsm` itself does not dynamically allocate state objects or transition nodes.

The complete hierarchy is constructed when the root `state_machine` is constructed and remains alive for the lifetime of that object.

This gives the machine a fixed runtime structure that closely reflects its compile-time architecture.

---

### Every state has an enum identity

A state is represented by both:

1. a C++ type containing its behavior;
2. an enum value identifying its architectural state.

For example:

```cpp
enum class PlayerState {
  Stopped,
  Playing,
  Paused,
};

struct Playing : hfsm::state<Playing> {
  void on_entry() {}
  void on_update() {}
  void on_exit() {}
};

using PlayingState =
    state_entry<Playing, PlayerState::Playing>;
```

The type and enum have different responsibilities:

```text
Playing                    PlayerState::Playing
   │                                │
   │ behavior                       │ architectural identity
   │                                │
   ├─ on_entry()                    ├─ transition table
   ├─ on_update()                   ├─ current_state()
   └─ on_exit()                     └─ system-level state representation
```

This makes the state-machine architecture explicit instead of relying entirely on C++ types as state identities.

The currently active state can therefore be queried directly:

```cpp
if (sm.current_state() == PlayerState::Playing) {
  // ...
}
```

---

### Step-driven execution

`hfsm` is designed for systems that execute state-machine logic periodically.

For example:

```cpp
hfsm::state_machine<Vehicle> sm;

sm.start();

while (running) {
  update_inputs();
  sm.step();
  sleep_until_next_cycle();
}
```

A state machine advances only when:

```cpp
sm.step();
```

is called.

This is intentionally different from state-machine implementations that continuously follow every available transition until no more transitions can be taken.

For ordinary states, a single root `step()` evaluates the active hierarchy from outer to inner.

At each level:

```text
machine on_update()
        │
        v
evaluate transitions
        │
        ├── transition available
        │       │
        │       └── take transition and stop propagating inward
        │
        └── no transition
                │
                └── update active child
```

For example:

```text
Vehicle::on_update()
        │
        ├── no Vehicle transition
        v
Autonomous::on_update()
        │
        ├── no Autonomous transition
        v
Driving::on_update()
```

If `Vehicle` takes a transition during that step:

```text
Vehicle::on_update()
        │
        v
Vehicle transition
```

the update is not propagated to `Autonomous`.

This gives outer architectural layers priority over inner layers.

It also makes execution naturally fit fixed-frequency control loops used in robotics, automation, vehicle control, and similar systems.

Pseudo states are the exception: they are transient decision points and are resolved immediately within the same step.

---

### Guard-driven transitions, without an event system

`hfsm` intentionally does not define an event type or event-dispatch mechanism.

Transitions are controlled directly by guards:

```cpp
bool should_start();
bool should_stop();
bool should_enter_autonomous();
```

A transition is defined as:

```cpp
transition<
    ManualState,
    AutonomousState,
    &Vehicle::should_enter_autonomous,
    &Vehicle::enter_autonomous>
```

This design is useful for large periodic control systems where transitions are usually determined from the current system state rather than from isolated event objects.

For example:

```cpp
bool should_enter_autonomous() {
  return driver_request &&
         localization_ready &&
         planning_ready &&
         vehicle_speed < max_engage_speed;
}
```

External events can still be represented as application data:

```cpp
bool button_pressed;
bool command_received;
bool timeout;
```

but `hfsm` does not require every condition to become a distinct event type or maintain an event queue.

This keeps the state-machine interface smaller and avoids an event vocabulary growing together with system complexity.

#### Transition priority

More than one guard may be true during the same update.

For example:

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

If both guards return `true`, the first matching transition wins.

Transition priority is therefore explicit:

```text
higher priority
      │
      v

Driving -> Emergency
Driving -> Parking
Driving -> Idle

      ^
      │
lower priority
```

The order of `transition_table` is part of the state-machine behavior.

This is useful when a single system state may satisfy several transition conditions simultaneously and the architecture needs a deterministic priority between them.

## Features

* Header-only
* C++14
* Strict hierarchical state machines
* Compile-time state and transition structure
* Static state storage
* Guard-based transitions
* Ordered transition priority
* Transition actions
* State lifecycle callbacks
* State-machine lifecycle callbacks
* Transient pseudo states
* Enum identity for every state
* Explicit single-step execution
* Runtime state inspection
* State instance access through `get_state()`
* Copyable and movable runtime snapshots
* Empty transition tables
* No external runtime dependencies

## Requirements

* C++14-compatible compiler
* CMake 3.20 or later when using the provided CMake build

### Platform support

Version 1.0 is developed and tested on Linux with:

* GCC
* Clang

The implementation uses standard C++14 and does not intentionally depend on GCC- or Clang-specific language extensions.

Other conforming compilers and platforms may work, but they are not officially tested or supported in v1.0.

## Quick Start

A state machine consists of:

1. a state enum;
2. state types;
3. a state-machine definition;
4. an initial state;
5. a transition table.

```cpp
#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

#include <tuple>

enum class PlayerState {
  Stopped,
  Playing,
  Paused,
};

struct PlayerData {
  bool play_requested{false};
  bool pause_requested{false};
  bool resume_requested{false};
  bool stop_requested{false};
};

PlayerData data;

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

struct Paused : hfsm::state<Paused> {
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

  using PausedState =
      state_entry<Paused, PlayerState::Paused>;

  bool should_play() {
    return data.play_requested;
  }

  bool should_pause() {
    return data.pause_requested;
  }

  bool should_resume() {
    return data.resume_requested;
  }

  bool should_stop() {
    return data.stop_requested;
  }

  void start_playback() {}
  void pause_playback() {}
  void resume_playback() {}
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
          PausedState,
          &Player::should_pause,
          &Player::pause_playback>,

      transition<
          PlayingState,
          StoppedState,
          &Player::should_stop,
          &Player::stop_playback>,

      transition<
          PausedState,
          PlayingState,
          &Player::should_resume,
          &Player::resume_playback>,

      transition<
          PausedState,
          StoppedState,
          &Player::should_stop,
          &Player::stop_playback>>;
};

int main() {
  hfsm::state_machine<Player> sm;

  sm.start();

  data.play_requested = true;
  sm.step();
  data.play_requested = false;

  data.pause_requested = true;
  sm.step();

  return 0;
}
```

The repository contains complete examples in:

```text
examples/simple_state_machine.cpp
examples/hierarchical_state_machine.cpp
examples/pseudo_state.cpp
```

## Defining States

A regular state derives from:

```cpp
hfsm::state<T>
```

For example:

```cpp
struct Running : hfsm::state<Running> {
  void on_entry() {
    // Enter Running.
  }

  void on_update() {
    // Running remains active during this step.
  }

  void on_exit() {
    // Leave Running.
  }
};
```

All lifecycle callbacks are optional.

States are bound to enum values through `state_entry`:

```cpp
using RunningState =
    state_entry<Running, MachineState::Running>;
```

### State lifetime

State objects are not constructed when they are entered.

The complete set of states is constructed together with the containing state machine.

Therefore:

```text
construct state_machine
    │
    ├── construct StateA
    ├── construct StateB
    └── construct ChildMachine

start
    │
    └── StateA::on_entry()

StateA -> StateB
    │
    ├── StateA::on_exit()
    └── StateB::on_entry()

StateB -> StateA
    │
    ├── StateB::on_exit()
    └── StateA::on_entry()
```

`StateA` is not reconstructed when it is entered again.

State-local data therefore persists across exits and re-entry:

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

### Construction requirement

State-machine definitions and all states in the hierarchy must be default constructible.

This requirement is checked at compile time.

## Defining Transitions

A transition consists of:

```text
source
target
guard
action
```

For example:

```cpp
transition<
    IdleState,
    RunningState,
    &Machine::should_start,
    &Machine::start>
```

Guards have the form:

```cpp
bool guard();
```

Actions have the form:

```cpp
void action();
```

When a transition is taken:

```text
source::on_exit()
        │
        v
transition action
        │
        v
target::on_entry()
```

Transitions for the current state are checked in transition-table order.

The first guard that evaluates to `true` wins.

If no transition is available, the active state receives `on_update()`.

The initial state does not need to appear in the transition table.

An empty transition table is also valid:

```cpp
using transition_table = std::tuple<>;
```

Such a machine can remain permanently in its initial state while receiving periodic updates.

## Hierarchical State Machines

A nested state machine is defined exactly like a root state machine.

For example:

```cpp
enum class AutonomousState {
  Standby,
  Driving,
  Parking,
};

struct Autonomous
    : hfsm::state_machine_def<
          Autonomous,
          AutonomousState> {
  // Autonomous states and transitions...
};
```

The parent treats it as one state:

```cpp
enum class VehicleState {
  Manual,
  Autonomous,
};

struct Vehicle
    : hfsm::state_machine_def<
          Vehicle,
          VehicleState> {
  using ManualState =
      state_entry<Manual, VehicleState::Manual>;

  using AutonomousStateEntry =
      state_entry<Autonomous, VehicleState::Autonomous>;

  // Vehicle-level transitions...
};
```

The architectural boundary is:

```text
Vehicle transition table
        │
        ├── Manual
        └── Autonomous
                  │
                  └── Autonomous transition table
                        ├── Standby
                        ├── Driving
                        └── Parking
```

The `Vehicle` transition table does not decide which internal autonomous state should be entered.

It enters `Autonomous`.

`Autonomous` then enters through its own `initial_state`.

This is the central hierarchy rule of `hfsm`.

### Entry order

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

### Update order

Updates also proceed from outer to inner while no outer transition is taken:

```text
Vehicle::on_update()
Autonomous::on_update()
Standby::on_update()
```

### Exit order

Exit proceeds from inner to outer:

```text
Standby::on_exit()
Autonomous::on_exit()
```

This recursive behavior works for arbitrary hierarchy depth.

## Machine-Level Lifecycle

A `state_machine_def` can itself define lifecycle callbacks:

```cpp
struct Machine
    : hfsm::state_machine_def<Machine, MachineState> {
  void on_entry() {}

  void on_update() {}

  void on_exit() {}

  // ...
};
```

These callbacks belong to the state-machine layer itself rather than to one individual state.

### `on_entry()`

For the root machine:

```cpp
sm.start();
```

invokes:

```text
Machine::on_entry()
initial_state::on_entry()
```

For a nested machine, its `on_entry()` is invoked when its parent enters that machine.

### `on_update()`

Every machine that receives an update first calls its machine-level:

```cpp
on_update()
```

before evaluating its transition guards.

This allows a machine layer to update data before deciding whether it should transition.

### `on_exit()`

A nested machine receives `on_exit()` when its parent leaves it.

The root machine currently has no `stop()` operation, so its machine-level `on_exit()` is not invoked during normal operation.

Destroying a root `state_machine` does not synthesize an FSM exit.

## `start()` and `step()`

A root machine has a one-shot lifecycle:

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
    │
    v
destruct
```

`start()` must be called exactly once.

After `start()`, the machine remains started for its lifetime.

Restart, reset, and stop operations are not currently provided.

### `current_state()`

The current enum state can be queried with:

```cpp
sm.current_state();
```

`current_state()` has a precondition:

```text
start() must already have been called
```

Use:

```cpp
sm.is_started();
```

if the caller needs to inspect whether the machine has started.

## Pseudo States

A pseudo state is a transient decision point:

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

Unlike a regular state, a pseudo state does not remain active until the next `step()`.

After entering it, `hfsm` immediately evaluates its outgoing transitions.

A single call to:

```cpp
sm.step();
```

may therefore perform:

```text
Idle
  |
  v
Decision
  |
  v
Fast
```

within the same step.

Pseudo states may receive:

```cpp
on_entry()
on_exit()
```

but they do not receive:

```cpp
on_update()
```

A pseudo state must have an available outgoing transition when it is entered.

If none is available, `std::logic_error` is thrown.

Pseudo-state transition cycles are also detected and reported with `std::logic_error`.

## Object Semantics

`hfsm::state_machine` owns its complete runtime hierarchy by value.

That includes:

```text
StateMachineDef
States
Nested StateMachines
Transition entries
```

### Copy

Copying a state machine copies its complete runtime snapshot.

This includes:

* whether the machine has started;
* the currently active state;
* current states of nested machines;
* state-machine-definition data;
* active state data;
* inactive state data.

For example:

```cpp
hfsm::state_machine<Machine> original;

original.start();
original.step();

auto copy = original;
```

`copy` continues from the same snapshot as `original`.

The two machine objects then operate independently according to the normal copy semantics of their contained objects.

Copying does not invoke:

```text
on_entry()
on_update()
on_exit()
```

Copy assignment similarly replaces the target runtime snapshot without synthesizing FSM lifecycle callbacks.

Pointers, references, smart pointers, and other members stored by user-defined states keep their normal C++ copy semantics.

### Move

Moving transfers the runtime snapshot according to the move semantics of the contained objects:

```cpp
auto moved = std::move(original);
```

The destination can continue executing from the transferred snapshot.

Move construction and move assignment do not invoke FSM lifecycle callbacks.

After a move, the runtime state of the source object is unspecified and should not be relied upon.

Copy and move support naturally depends on whether the contained state-machine definitions and state objects support the corresponding C++ operation.

## Application Data

`hfsm` intentionally does not define a context type.

The state-machine library is responsible for:

```text
architecture
states
hierarchy
transition evaluation
lifecycle
```

Application data ownership remains outside that model.

Guards, actions, and state callbacks can access data using whichever mechanism fits the application architecture.

For example:

```cpp
struct VehicleData {
  bool autonomous_requested;
  bool localization_ready;
  bool planning_ready;
};

VehicleData data;
```

and:

```cpp
bool should_enter_autonomous() {
  return data.autonomous_requested &&
         data.localization_ready &&
         data.planning_ready;
}
```

A future application may instead use references, dependency injection, subsystem interfaces, global system state, or another ownership model.

`hfsm` does not impose one.

## Installation

`hfsm` is header-only.

The exported CMake target is:

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

The target requests C++14 automatically.

### Install and `find_package`

Configure:

```bash
cmake -S . -B build \
  -DHFSM_BUILD_TESTS=OFF \
  -DHFSM_BUILD_EXAMPLES=OFF
```

Install:

```bash
cmake --install build --prefix /path/to/install
```

A consuming project can then use:

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

Configure:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DHFSM_BUILD_TESTS=ON \
  -DHFSM_BUILD_EXAMPLES=ON
```

Build:

```bash
cmake --build build --parallel
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

The test suite contains both runtime tests and compile-fail tests for invalid state-machine definitions.

## Examples

Three examples are provided:

```text
examples/simple_state_machine.cpp
```

Basic states, guards, actions, transition priority, and lifecycle.

```text
examples/hierarchical_state_machine.cpp
```

Strict parent/child state-machine hierarchy and recursive lifecycle behavior.

```text
examples/pseudo_state.cpp
```

Transient pseudo-state decision logic.

## Scope

`hfsm` intentionally keeps its scope small.

Version 1.0 focuses on:

```text
strict hierarchy
static structure
guard-driven transitions
fixed-cycle execution
predictable lifecycle
```

It intentionally does not currently provide:

* an event-dispatch framework;
* an event queue;
* dynamic state registration;
* runtime modification of the transition graph;
* stop/reset/restart semantics;
* a prescribed application context model;
* Conan or vcpkg packages.

These are deliberate scope decisions rather than requirements for the core state-machine model.

## License

`hfsm` is released under the MIT License.

See `LICENSE` for details.
