#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

enum class MediaPlayerState { Stopped, Playing, Paused };

struct Stopped : hfsm::state<Stopped> {};

struct Playing : hfsm::state<Playing>  {};

struct Paused : hfsm::state<Paused> {};

struct MediaPlayer : hfsm::state_machine_def<MediaPlayer, MediaPlayerState> {
    using StoppedState = state_entry<Stopped, MediaPlayerState::Stopped>;
    using PlayingState = state_entry<Playing, MediaPlayerState::Playing>;
    using PausedState = state_entry<Paused, MediaPlayerState::Paused>;

    bool can_play() { return true; }
    bool should_pause() { return true; }
    bool should_resume() { return true; }
    bool should_stop() { return true; }

    void start_playback() {}
    void pause_playback() {}
    void resume_playback() {}
    void stop_playback() {}

    using initial_state = StoppedState;

    using transition_table = std::tuple<
        transition<StoppedState, PlayingState, &MediaPlayer::can_play, &MediaPlayer::start_playback>,
        transition<PlayingState, PausedState, &MediaPlayer::should_pause, &MediaPlayer::pause_playback>,
        transition<PausedState, PlayingState, &MediaPlayer::should_resume, &MediaPlayer::resume_playback>,
        transition<PlayingState, StoppedState, &MediaPlayer::should_stop, &MediaPlayer::stop_playback>,
        transition<PausedState, StoppedState, &MediaPlayer::should_stop, &MediaPlayer::stop_playback>
    >;
};

struct Unknown { };

int main() {
  hfsm::state_machine<MediaPlayer> sm;
  (void)sm.get_state<Unknown>();
}