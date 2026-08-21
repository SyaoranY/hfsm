#ifndef HFSM_STATE_MACHINE_H_
#define HFSM_STATE_MACHINE_H_

#include <array>
#include <hfsm/state_machine_def.h>
#include <hfsm/state_machine_mpl.h>
#include <stdexcept>

namespace hfsm {

template<typename StateMachineDef>
class state_machine {
    static_assert(
        detail::is_derived_from_state_machine_def<StateMachineDef>::value,
        "template parameter must be derived from state_machine_def"
    );

public:
    using state_flag = state_machine_backend_tag;
    using enum_type = typename StateMachineDef::enum_type;
    using guard_type = typename StateMachineDef::guard_type;
    using action_type = typename StateMachineDef::action_type;
    using initial_state = typename StateMachineDef::initial_state;
    using transition_table = typename StateMachineDef::transition_table;
    using transition_entry_t = typename StateMachineDef::transition_entry_t;
    static constexpr std::size_t tt_size = hfsm::mpl::mp_size<transition_table>::value;
    using transition_entries_t = std::array<transition_entry_t, tt_size>;

    void start() {
        if (is_started_) {
            throw std::logic_error("start() cannot be called twice");
        }
        is_started_ = true;
        current_ = initial_state::enum_value;
        on_entry();
    }

    void step() {
        if (!is_started_) {
            throw std::logic_error("step() cannot be called before start()");
        }
        transition_entry_t trans;
        if (find_available_transition(trans)) {
            do_transition(trans);
        } else {
            do_sub_state_update();
        }
    }

    enum_type current_state() const noexcept { return current_; }

    template<typename T>
    auto& get_state() {
        using hfsm::mpl::mp_find_if;
        using hfsm::mpl::mp_size;
        constexpr size_t index = mp_find_if<sub_states_list_t, state_match<T>::template pred>::value;
        static_assert(index < mp_size<sub_states_list_t>::value, "template parameter T must be a valid state type");
        return std::get<index>(sub_states_);
    }

private:
    template<typename SubStateMachineDef>
    friend class state_machine;

    // as a sub state machine
    void on_entry() {
        is_started_ = true;
        current_ = initial_state::enum_value;
        state_machine_def_.on_entry();
        do_sub_state_entry(current_);
    }

    // as a sub state machine
    void on_exit() {
        do_sub_state_exit(current_);
        state_machine_def_.on_exit();
        current_ = initial_state::enum_value;
    }

    // as a sub state machine
    void on_update() {
        state_machine_def_.on_update();
        step();
    }

    template<template<typename...> class L, typename... Trans>
    static constexpr auto construct_transition_entries(L<Trans...>*) -> transition_entries_t {
        return {Trans::make_transition_entry()...};
    }

    template<typename StateEntry, typename Tag = typename StateEntry::state_flag>
    struct sub_state_transform;

    template<typename StateEntry>
    struct sub_state_transform<StateEntry, normal_state_tag> {
        using type = StateEntry;
    };

    template<typename StateEntry>
    struct sub_state_transform<StateEntry, state_machine_frontend_tag> {
        using state_type = typename StateEntry::state_type;
        using enum_type = typename StateEntry::enum_type;
        static constexpr enum_type enum_value = StateEntry::enum_value;
        using type = detail::state_entry<state_machine<state_type>, enum_type, enum_value>;
    };

    template<typename State>
    using sub_state_transform_t = typename sub_state_transform<State>::type;

    template<typename L>
    struct sub_states_list;

    template<template<typename...> class L, typename... Trans>
    struct sub_states_list<L<Trans...>> {
        using source_state_entry_list = hfsm::mpl::mp_list<typename Trans::source_state_entry_t...>;
        using target_state_entry_list = hfsm::mpl::mp_list<typename Trans::target_state_entry_t...>;
        using state_entry_list =
            hfsm::mpl::mp_set_union<hfsm::mpl::mp_list<>, source_state_entry_list, target_state_entry_list>;
        using type = hfsm::mpl::mp_apply<std::tuple, hfsm::mpl::mp_transform<sub_state_transform_t, state_entry_list>>;
    };

    using sub_states_list_t = typename sub_states_list<transition_table>::type;

    template<typename State>
    struct state_match {
        template<typename StateEntry>
        using pred = hfsm::mpl::mp_bool<
            std::is_same<typename StateEntry::state_type, State>::value ||
            std::is_same<typename StateEntry::state_type, state_machine<State>>::value>;
    };

    bool find_available_transition(transition_entry_t& trans) {
        for (auto const& trans_entry : transitions_) {
            if (trans_entry.source == current_ && (state_machine_def_.*trans_entry.guard)()) {
                trans = trans_entry;
                return true;
            }
        }
        return false;
    }

    void do_transition(transition_entry_t const& trans) {
        do_sub_state_exit(trans.source);
        (state_machine_def_.*trans.action)();
        do_sub_state_entry(trans.target);
        current_ = trans.target;
    }

    void do_sub_state_exit(enum_type const source) {
        hfsm::mpl::tuple_visit_if(
            sub_states_,
            [source](auto const& state_entry) { return state_entry.enum_value == source; },
            [](auto& state_entry) { state_entry.on_exit(); }
        );
    }

    void do_sub_state_entry(enum_type const target) {
        hfsm::mpl::tuple_visit_if(
            sub_states_,
            [target](auto const& state_entry) { return state_entry.enum_value == target; },
            [](auto& state_entry) { state_entry.on_entry(); }
        );
    }

    void do_sub_state_update() {
        hfsm::mpl::tuple_visit_if(
            sub_states_,
            [current = current_](auto const& state_entry) { return state_entry.enum_value == current; },
            [](auto& state_entry) { state_entry.on_update(); }
        );
    }

    bool is_started_ = false;
    enum_type current_;
    StateMachineDef state_machine_def_;
    sub_states_list_t sub_states_;
    transition_entries_t transitions_ = construct_transition_entries(static_cast<transition_table*>(nullptr));
};

/**
 * 一、状态机的切换 (Done)
 * 遍历所有source == 当前枚举值的guard，直到有一个guard() -> true,
 * if guard() == true:
 *    执行跳转：source状态的退出，执行action、target状态的进入
 * else (all guard() == false):
 *    执行当前状态的on_update
 *
 * 二、子状态机的退出 (Done)
 * 1）执行当前状态的on_exit
 * 2）执行子状态机前端的on_exit
 * 3）将当前状态设置为初始状态
 *
 * 三、子状态机的进入 (Done)
 *  1）执行子状态机前端的on_entry
 *  2) 执行初始状态的on_entry
 *  3) if 初始状态是伪状态，执行一个子状态机的运行
 *
 * 四、子状态机的on_update (Done)
 *  1) 执行子状态机前端的on_update,
 *  2) 执行一次子状态机的跳转
 */

/**
 * 五、状态机的对象保存 (Done)
 *
 * 六、支持伪状态
 *
 * 七、状态机的启动 (Done)
 *  is_start_ = true;
 *  current_ = initial_state
 *  执行一次子状态机的进入
 *
 * TODOLIST: 1) 测试用例、 2）伪状态支持 + 测试用例， 3）
 */

} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_H_ */
