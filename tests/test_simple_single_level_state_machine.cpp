#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

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

// ============================================================
// Test fixture
// ============================================================

class SingleLevelStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override { context.reset(); }
};

// ============================================================
// Initial state
// ============================================================

TEST_F(SingleLevelStateMachineTest, StartTwice) {
    hfsm::state_machine<MediaPlayer> sm;
    sm.start();
    EXPECT_THROW(sm.start(), std::logic_error);
}

TEST_F(SingleLevelStateMachineTest, StepBeforeStart) {
    hfsm::state_machine<MediaPlayer> sm;
    EXPECT_THROW(sm.step(), std::logic_error);
}

TEST_F(SingleLevelStateMachineTest, StartsFromInitialState) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

// ============================================================
// Guard
// ============================================================

TEST_F(SingleLevelStateMachineTest, DoesNotTransitionWhenGuardIsFalse) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);

    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

// ============================================================
// Stopped -> Playing
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromStoppedToPlaying) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;

    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Playing);
}

// ============================================================
// Playing -> Paused
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPlayingToPaused) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.pause_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Paused);
}

// ============================================================
// Paused -> Playing
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPausedToPlaying) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.pause_requested = true;
    sm.step();

    context.reset();
    context.resume_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Playing);
}

// ============================================================
// Playing -> Stopped
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPlayingToStopped) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.stop_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

// ============================================================
// Paused -> Stopped
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPausedToStopped) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.pause_requested = true;
    sm.step();

    context.reset();
    context.stop_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

// ============================================================
// Playing -> Buffering
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPlayingToBuffering) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.buffering_required = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Buffering);
}

// ============================================================
// Buffering -> Playing
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromBufferingToPlaying) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.buffering_required = true;
    sm.step();

    context.reset();
    context.buffer_available = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Playing);
}

// ============================================================
// Playing -> Error
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPlayingToError) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.error_occurred = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Error);
}

// ============================================================
// Paused -> Error
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromPausedToError) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.pause_requested = true;
    sm.step();

    context.reset();
    context.error_occurred = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Error);
}

// ============================================================
// Buffering -> Error
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromBufferingToError) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.buffering_required = true;
    sm.step();

    context.reset();
    context.error_occurred = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Error);
}

// ============================================================
// Error -> Stopped
// ============================================================

TEST_F(SingleLevelStateMachineTest, TransitionsFromErrorToStopped) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();
    context.error_occurred = true;
    sm.step();

    context.reset();
    context.reset_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

// ============================================================
// Wrong transition must not affect current state
// ============================================================

TEST_F(SingleLevelStateMachineTest, IgnoresTransitionsFromOtherStates) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    // resume is only meaningful in Paused
    context.resume_requested = true;

    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

// ============================================================
// Multiple outgoing transitions
// ============================================================

TEST_F(SingleLevelStateMachineTest, SelectsAvailableTransitionAmongMultipleOutgoingTransitions) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();

    // Playing has several outgoing transitions.
    context.buffering_required = true;

    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Buffering);
}

// ============================================================
// Transition priority
// ============================================================

TEST_F(SingleLevelStateMachineTest, SelectsFirstAvailableTransitionWhenMultipleGuardsAreTrue) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    context.play_requested = true;
    sm.step();

    context.reset();

    // According to transition_table:
    //
    // Playing -> Paused
    // comes before
    // Playing -> Error
    //
    context.pause_requested = true;
    context.error_occurred = true;

    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Paused);
}

// ============================================================
// Complete cycle
// ============================================================

TEST_F(SingleLevelStateMachineTest, SupportsMultipleSequentialTransitions) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);

    // Stopped -> Playing
    context.play_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Playing);

    // Playing -> Paused
    context.reset();
    context.pause_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Paused);

    // Paused -> Playing
    context.reset();
    context.resume_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Playing);

    // Playing -> Buffering
    context.reset();
    context.buffering_required = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Buffering);

    // Buffering -> Error
    context.reset();
    context.error_occurred = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Error);

    // Error -> Stopped
    context.reset();
    context.reset_requested = true;
    sm.step();

    EXPECT_EQ(sm.current_state(), MediaPlayerState::Stopped);
}

TEST_F(SingleLevelStateMachineTest, CallsOnEntryOnUpdateAndOnExit) {
    hfsm::state_machine<MediaPlayer> sm;

    sm.start();

    // Stopped -> Playing
    context.play_requested = true;
    sm.step();

    EXPECT_EQ(context.playing_entry_count, 1);
    EXPECT_EQ(context.playing_update_count, 0);
    EXPECT_EQ(context.playing_exit_count, 0);

    // Stay in Playing and execute on_update().
    context.play_requested = false;
    sm.step();

    EXPECT_EQ(context.playing_entry_count, 1);
    EXPECT_EQ(context.playing_update_count, 1);
    EXPECT_EQ(context.playing_exit_count, 0);

    // Playing -> Paused
    context.pause_requested = true;
    sm.step();

    EXPECT_EQ(context.playing_entry_count, 1);
    EXPECT_EQ(context.playing_update_count, 1);
    EXPECT_EQ(context.playing_exit_count, 1);
}