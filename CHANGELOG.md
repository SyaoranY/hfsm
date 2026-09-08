# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-09-07

### Added

* Header-only C++14 hierarchical finite state machine library.
* Strict hierarchical state-machine composition.
* Compile-time transition-table definitions.
* Static storage for states and nested state machines.
* Enum-based state identity through `state_entry`.
* Guard-driven transitions without a built-in event system.
* Deterministic transition priority based on transition-table order.
* Transition actions.
* State-level lifecycle callbacks:

  * `on_entry()`
  * `on_update()`
  * `on_exit()`
* State-machine-level lifecycle callbacks.
* Step-driven execution for fixed-cycle control systems.
* Recursive hierarchical update and lifecycle propagation.
* Transient pseudo states with immediate transition resolution.
* Detection of invalid pseudo-state transitions and pseudo-state cycles.
* Support for initial states that do not appear in the transition table.
* Support for empty transition tables.
* Runtime state inspection through `current_state()` and `is_started()`.
* Access to stored state instances through `get_state()`.
* Copy and move semantics based on complete runtime state-machine snapshots.
* Compile-time validation of default-constructibility requirements.
* CMake installation and `find_package(hfsm)` support.
* Runtime unit tests and compile-fail tests.
* Linux CI coverage with GCC and Clang.
* Installed-package consumer tests.
* Examples for:

  * simple state machines
  * hierarchical state machines
  * pseudo states

### Platform Support

* Linux
* GCC
* Clang

Other C++14-conforming compilers and platforms may work but are not officially tested or supported in v1.0.
