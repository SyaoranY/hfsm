#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

#include <iostream>

enum class MediaPlayerState {
    Stopped,
    Playing,
    Paused,
};

// Application-owned data.
// hfsm does not prescribe how external application state is stored.
struct MediaPlayerContext {
    bool play_requested{false};
    bool pause_requested{false};
    bool resume_requested{false};
    bool stop_requested{false};
};

MediaPlayerContext context;

struct Stopped : hfsm::state<Stopped> {
    void on_entry() { std::cout << "enter Stopped\n"; }
    void on_update() { std::cout << "update Stopped\n"; }
    void on_exit() { std::cout << "exit Stopped\n"; }
};

struct Playing : hfsm::state<Playing> {
    void on_entry() { std::cout << "enter Playing\n"; }
    void on_update() { std::cout << "update Playing\n"; }
    void on_exit() { std::cout << "exit Playing\n"; }
};

struct Paused : hfsm::state<Paused> {
    void on_entry() { std::cout << "enter Paused\n"; }
    void on_update() { std::cout << "update Paused\n"; }
    void on_exit() { std::cout << "exit Paused\n"; }
};

struct MediaPlayer : hfsm::state_machine_def<MediaPlayer, MediaPlayerState> {
    using StoppedState = state_entry<Stopped, MediaPlayerState::Stopped>;
    using PlayingState = state_entry<Playing, MediaPlayerState::Playing>;
    using PausedState = state_entry<Paused, MediaPlayerState::Paused>;

    bool should_play() { return context.play_requested; }
    bool should_pause() { return context.pause_requested; }
    bool should_resume() { return context.resume_requested; }
    bool should_stop() { return context.stop_requested; }

    void start_playback() { std::cout << "action: start playback\n"; }
    void pause_playback() { std::cout << "action: pause playback\n"; }
    void resume_playback() { std::cout << "action: resume playback\n"; }
    void stop_playback() { std::cout << "action: stop playback\n"; }

    using initial_state = StoppedState;

    using transition_table = std::tuple<
        transition<
            StoppedState,
            PlayingState,
            &MediaPlayer::should_play,
            &MediaPlayer::start_playback>,
        transition<
            PlayingState,
            PausedState,
            &MediaPlayer::should_pause,
            &MediaPlayer::pause_playback>,
        transition<
            PlayingState,
            StoppedState,
            &MediaPlayer::should_stop,
            &MediaPlayer::stop_playback>,
        transition<
            PausedState,
            PlayingState,
            &MediaPlayer::should_resume,
            &MediaPlayer::resume_playback>,
        transition<
            PausedState,
            StoppedState,
            &MediaPlayer::should_stop,
            &MediaPlayer::stop_playback>>;
};

int main() {
    hfsm::state_machine<MediaPlayer> sm;

    std::cout << "start\n";
    sm.start();

    std::cout << "\nstep without transition\n";
    sm.step();

    std::cout << "\nrequest play\n";
    context.play_requested = true;
    sm.step();
    context.play_requested = false;

    std::cout << "\nrequest pause\n";
    context.pause_requested = true;
    sm.step();
    context.pause_requested = false;

    std::cout << "\nrequest resume\n";
    context.resume_requested = true;
    sm.step();
    context.resume_requested = false;

    std::cout << "\nrequest stop\n";
    context.stop_requested = true;
    sm.step();

    return 0;
}