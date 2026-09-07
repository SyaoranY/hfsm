# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.0]

### Added

* Header-only C++14 hierarchical finite state machine library.
* Compile-time transition tables.
* Hierarchical state machines with nested state machines.
* Guarded transitions.
* Transition actions.
* State lifecycle callbacks:

  * `on_entry()`
  * `on_update()`
  * `on_exit()`
* Pseudo states for transient decision points.
* Detection of invalid pseudo-state transitions and transition cycles.
* State access through `get_state()`.
* Current-state inspection through `current_state()`.
* State-machine lifecycle inspection through `is_started()`.
* CMake installation and `find_package(hfsm)` support.
* Runtime and compile-time validation tests.
* Example programs for simple, hierarchical, and pseudo-state machines.

### Platform Support

* Linux
* GCC
* Clang

hfsm is written against the C++14 standard and does not intentionally rely on compiler-specific extensions. Other conforming compilers and platforms may work but are not officially tested or supported in v1.0.
