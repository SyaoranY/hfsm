#ifndef HFSM_STATE_MACHINE_H_
#define HFSM_STATE_MACHINE_H_

#include <hfsm/state_machine_def.h>
#include <hfsm/state_machine_mpl.h>
#include <array>

namespace hfsm {

template<typename StateMachineDef>
class state_machine {
  static_assert(detail::is_derived_from_state_machine_def<StateMachineDef>::value, "StateMachineDef must be derived from state_machine_def");
 public:
  using enum_type = typename StateMachineDef::enum_type;
  using guard_type = typename StateMachineDef::guard_type;
  using action_type = typename StateMachineDef::action_type;
  using transition_table = typename StateMachineDef::transition_table;
  using transition_entry_t = typename StateMachineDef::transition_entry_t;
  static constexpr std::size_t tt_size = hfsm::mpl::mp_size<transition_table>::value;
  using transition_entries_t = std::array<transition_entry_t, tt_size>;

//  private:
  template<template<typename...> class L, typename... Trans>
  constexpr auto construct_transition_entries(L<Trans...>*) -> transition_entries_t {
    return {Trans::make_transition_entry()...};
  }

  template<typename State, typename Tag = typename State::state_flag>
  struct sub_state_transform;

  template<typename State>
  struct sub_state_transform<State, normal_state_tag> {
    using type = State;
  };

  template<typename State>
  struct sub_state_transform<State, state_machine_tag> {
    using type = state_machine<State>;
  };

  template<typename State>
  using sub_state_transform_t = typename sub_state_transform<State>::type;

  template<typename L>
  struct sub_states_list;

  template<template<typename...> class L, typename... Trans>
  struct sub_states_list<L<Trans...>> {
    using source_state_list = hfsm::mpl::mp_list<typename Trans::source_state...>;
    using target_state_list = hfsm::mpl::mp_list<typename Trans::target_state...>;
    using state_list = hfsm::mpl::mp_set_union<hfsm::mpl::mp_list<>, source_state_list, target_state_list>;
    using type = hfsm::mpl::mp_apply<std::tuple, hfsm::mpl::mp_transform<sub_state_transform_t, state_list>>;
  };

  using sub_states_list_t = typename sub_states_list<transition_table>::type;

  static constexpr transition_entries_t transitions = construct_transition_entries(static_cast<transition_table*>(nullptr));

  enum_type current_;
  StateMachineDef state_machine_def_;
  sub_states_list_t sub_states_;
};

/**
 * 一、状态机的切换
 * 遍历所有source == 当前枚举值的guard，直到有一个guard() -> true,
 * if guard() == true:
 *    执行跳转：source状态的退出，执行action、target状态的进入
 * else (all guard() == false):
 *    执行当前状态的on_update
 * 
 * 二、子状态机的退出
 * 1）执行当前状态的on_exit
 * 2）执行子状态机前端的on_exit
 * 3）将当前状态设置为初始状态
 * 
 * 三、子状态机的进入
 *  1）执行子状态机前端的on_entry
 *  2) 执行初始状态的on_entry
 *  3) if 初始状态是伪状态，执行一个子状态机的运行
 *
 * 四、子状态机的on_update
 *  1) 执行子状态机前端的on_update,
 *  2) 执行一次子状态机的跳转
 */

/**
 * 五、状态机的对象保存
 * 
 */

} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_H_ */
