#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

struct Unknown { };

int main() {
  hfsm::state_machine<Unknown> sm;
  (void)sm;
}