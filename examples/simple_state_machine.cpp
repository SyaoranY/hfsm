#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>
#include <thread>

enum class MediaPlayerState { Stopped, Playing, Paused, Buffering, Error };

struct MediaPlayerContext {
    void reset() {
        playing_entry_count = 0;
        playing_update_count = 0;
        playing_exit_count = 0;
        play_requested = false;
        pause_requested = false;
        resume_requested = false;
        stop_requested = false;
        buffering_required = false;
        buffer_available = false;
        error_occurred = false;
        reset_requested = false;
    }
    int playing_entry_count{0};
    int playing_update_count{0};
    int playing_exit_count{0};
    bool play_requested{false};
    bool pause_requested{false};
    bool resume_requested{false};
    bool stop_requested{false};
    bool buffering_required{false};
    bool buffer_available{false};
    bool error_occurred{false};
    bool reset_requested{false};
};

MediaPlayerContext context;

struct Stopped : hfsm::state<Stopped> {};

struct Playing : hfsm::state<Playing> {
    void on_entry() { ++context.playing_entry_count; }

    void on_update() { ++context.playing_update_count; }

    void on_exit() { ++context.playing_exit_count; }
};

struct Paused : hfsm::state<Paused> {};

struct Buffering : hfsm::state<Buffering> {};

struct Error : hfsm::state<Error> {};

struct MediaPlayer : hfsm::state_machine_def<MediaPlayer, MediaPlayerState> {
    using StoppedState = state_entry<Stopped, MediaPlayerState::Stopped>;
    using PlayingState = state_entry<Playing, MediaPlayerState::Playing>;
    using PausedState = state_entry<Paused, MediaPlayerState::Paused>;
    using BufferingState = state_entry<Buffering, MediaPlayerState::Buffering>;
    using ErrorState = state_entry<Error, MediaPlayerState::Error>;

    bool can_play() { return context.play_requested; }
    bool should_pause() { return context.pause_requested; }
    bool should_resume() { return context.resume_requested; }
    bool should_stop() { return context.stop_requested; }
    bool buffer_empty() { return context.buffering_required; }
    bool buffer_ready() { return context.buffer_available; }
    bool has_error() { return context.error_occurred; }
    bool should_reset() { return context.reset_requested; }

    void start_playback() {}
    void pause_playback() {}
    void resume_playback() {}
    void stop_playback() {}
    void start_buffering() {}
    void handle_error() {}
    void reset() {}

    using initial_state = StoppedState;

    using transition_table = std::tuple<
        transition<StoppedState, PlayingState, &MediaPlayer::can_play, &MediaPlayer::start_playback>,
        transition<PlayingState, PausedState, &MediaPlayer::should_pause, &MediaPlayer::pause_playback>,
        transition<PausedState, PlayingState, &MediaPlayer::should_resume, &MediaPlayer::resume_playback>,
        transition<PlayingState, StoppedState, &MediaPlayer::should_stop, &MediaPlayer::stop_playback>,
        transition<PausedState, StoppedState, &MediaPlayer::should_stop, &MediaPlayer::stop_playback>,
        transition<PlayingState, BufferingState, &MediaPlayer::buffer_empty, &MediaPlayer::start_buffering>,
        transition<BufferingState, PlayingState, &MediaPlayer::buffer_ready, &MediaPlayer::resume_playback>,
        transition<PlayingState, ErrorState, &MediaPlayer::has_error, &MediaPlayer::handle_error>,
        transition<PausedState, ErrorState, &MediaPlayer::has_error, &MediaPlayer::handle_error>,
        transition<BufferingState, ErrorState, &MediaPlayer::has_error, &MediaPlayer::handle_error>,
        transition<ErrorState, StoppedState, &MediaPlayer::should_reset, &MediaPlayer::reset>>;
};

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  hfsm::state_machine<MediaPlayer> sm;
  sm.start();
  while (true) {
    sm.step();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  return 0;
}
