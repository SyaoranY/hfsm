#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

#include <iostream>

struct VehicleContext {
    bool power_on{false};
    bool engage_autonomous{false};
    bool disengage_autonomous{false};

    bool autonomous_ready{false};
    bool parking_requested{false};
    bool parking_completed{false};
};

VehicleContext context;

// -----------------------------------------------------------------------------
// Autonomous sub state machine
// -----------------------------------------------------------------------------

enum class AutonomousState {
    Standby,
    Cruising,
    Parking,
};

struct Standby : hfsm::state<Standby> {
    void on_entry() { std::cout << "  enter Standby\n"; }
    void on_update() { std::cout << "  update Standby\n"; }
    void on_exit() { std::cout << "  exit Standby\n"; }
};

struct Cruising : hfsm::state<Cruising> {
    void on_entry() { std::cout << "  enter Cruising\n"; }
    void on_update() { std::cout << "  update Cruising\n"; }
    void on_exit() { std::cout << "  exit Cruising\n"; }
};

struct Parking : hfsm::state<Parking> {
    void on_entry() { std::cout << "  enter Parking\n"; }
    void on_update() { std::cout << "  update Parking\n"; }
    void on_exit() { std::cout << "  exit Parking\n"; }
};

struct Autonomous
    : hfsm::state_machine_def<Autonomous, AutonomousState> {
    using StandbyState =
        state_entry<Standby, AutonomousState::Standby>;

    using CruisingState =
        state_entry<Cruising, AutonomousState::Cruising>;

    using ParkingState =
        state_entry<Parking, AutonomousState::Parking>;

    bool can_start_cruising() {
        return context.autonomous_ready;
    }

    bool should_park() {
        return context.parking_requested;
    }

    bool parking_done() {
        return context.parking_completed;
    }

    void start_cruising() {
        std::cout << "  action: start cruising\n";
    }

    void start_parking() {
        std::cout << "  action: start parking\n";
    }

    void finish_parking() {
        std::cout << "  action: finish parking\n";
    }

    void on_entry() {
        std::cout << "enter Autonomous state machine\n";
    }

    void on_exit() {
        std::cout << "exit Autonomous state machine\n";
    }

    using initial_state = StandbyState;

    using transition_table = std::tuple<
        transition<
            StandbyState,
            CruisingState,
            &Autonomous::can_start_cruising,
            &Autonomous::start_cruising>,
        transition<
            CruisingState,
            ParkingState,
            &Autonomous::should_park,
            &Autonomous::start_parking>,
        transition<
            ParkingState,
            StandbyState,
            &Autonomous::parking_done,
            &Autonomous::finish_parking>>;
};

// -----------------------------------------------------------------------------
// Root state machine
// -----------------------------------------------------------------------------

enum class VehicleState {
    Off,
    Manual,
    Autonomous,
};

struct Off : hfsm::state<Off> {
    void on_entry() { std::cout << "enter Off\n"; }
    void on_exit() { std::cout << "exit Off\n"; }
};

struct Manual : hfsm::state<Manual> {
    void on_entry() { std::cout << "enter Manual\n"; }
    void on_update() { std::cout << "update Manual\n"; }
    void on_exit() { std::cout << "exit Manual\n"; }
};

struct Vehicle : hfsm::state_machine_def<Vehicle, VehicleState> {
    using OffState =
        state_entry<Off, VehicleState::Off>;

    using ManualState =
        state_entry<Manual, VehicleState::Manual>;

    using AutonomousStateRef =
        state_entry<Autonomous, VehicleState::Autonomous>;

    bool should_power_on() {
        return context.power_on;
    }

    bool should_engage_autonomous() {
        return context.engage_autonomous;
    }

    bool should_disengage_autonomous() {
        return context.disengage_autonomous;
    }

    void power_on() {
        std::cout << "action: power on\n";
    }

    void engage_autonomous() {
        std::cout << "action: engage autonomous mode\n";
    }

    void disengage_autonomous() {
        std::cout << "action: disengage autonomous mode\n";
    }

    using initial_state = OffState;

    using transition_table = std::tuple<
        transition<
            OffState,
            ManualState,
            &Vehicle::should_power_on,
            &Vehicle::power_on>,
        transition<
            ManualState,
            AutonomousStateRef,
            &Vehicle::should_engage_autonomous,
            &Vehicle::engage_autonomous>,
        transition<
            AutonomousStateRef,
            ManualState,
            &Vehicle::should_disengage_autonomous,
            &Vehicle::disengage_autonomous>>;
};

int main() {
    hfsm::state_machine<Vehicle> sm;

    std::cout << "start\n";
    sm.start();

    std::cout << "\npower on\n";
    context.power_on = true;
    sm.step();
    context.power_on = false;

    std::cout << "\nengage autonomous mode\n";
    context.engage_autonomous = true;
    sm.step();
    context.engage_autonomous = false;

    std::cout << "\nautonomous system ready\n";
    context.autonomous_ready = true;
    sm.step();
    context.autonomous_ready = false;

    std::cout << "\nrequest parking\n";
    context.parking_requested = true;
    sm.step();
    context.parking_requested = false;

    std::cout << "\nparking completed\n";
    context.parking_completed = true;
    sm.step();
    context.parking_completed = false;

    std::cout << "\nno transition\n";
    sm.step();

    std::cout << "\ndisengage autonomous mode\n";
    context.disengage_autonomous = true;
    sm.step();

    return 0;
}