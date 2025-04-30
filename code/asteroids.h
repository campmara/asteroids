#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include "asteroids_platform.h"
global PlatformAPI global_platform;

#include "asteroids_math.h"
#include "asteroids_memory.h"
#include "asteroids_random.h"
#include "asteroids_font.h"
#include "asteroids_sound.h"

#define BITMAP_BYTES_PER_PIXEL 4

#define NUM_GRID_SPACES_H 16
#define NUM_GRID_SPACES_V 9

#define MAX_ASTEROIDS 26
#define MAX_ASTEROID_POINTS 8
#define MAX_SPEED_INCREASES 12

#define DEATH_TIME 2.0f
#define INVULN_TIME 1.5f
#define MAX_BULLETS 3

#define MAX_PARTICLES 100

#define NAME_ENTRY_MAX_LENGTH 3
#define NAME_ENTRY_MAX_ALLOWED_CHARS 36
#define MAX_HIGH_SCORES 10

#define SOUND_SAMPLES_PER_SECOND 48000
#define SOUND_BYTES_PER_SAMPLE sizeof(int16) * 2
#define SOUND_BYTES_PER_SECOND SOUND_SAMPLES_PER_SECOND / SOUND_BYTES_PER_SAMPLE
#define MAX_CONCURRENT_SOUNDS 16

// =================================================================================================
// HELPERS
// =================================================================================================

inline int GetStringLength(char *string)
{
    int length = 0;
    while (*string++)
    {
        ++length;
    }
    return length;
}

// Floored modulus operation so negative values wrap around the length.
inline int32 WrapIndex(int32 index, int32 array_length)
{
    return ((index % array_length) + array_length) % array_length;
}

// TODO(mara): Step through this function and make sure the lengths of these
// loops are set up properly.
void ConcatenateStrings(size_t src_a_count, char *src_a,
                        size_t src_b_count, char *src_b,
                        size_t dest_count, char *dest)
{
    // NOTE(mara): This function will only concatenate the two strings given
    // up to the dest_count limit, after which it will truncate the result.
    for (int i = 0; i < src_a_count && dest_count > 0; ++i)
    {
        *dest++ = *src_a++;

        --dest_count;
    }

    for (int i = 0; i < src_b_count && dest_count > 0; ++i)
    {
        *dest++ = *src_b++;

        --dest_count;
    }

    *dest++ = 0; // Insert the null terminator.
}

inline GameControllerInput *GetController(GameInput *input, uint32 controller_index)
{
    Assert(controller_index < ArrayCount(input->controllers));
    GameControllerInput *result = &input->controllers[controller_index];
    return result;
}

// =================================================================================================
// GAME CODE STATE
// =================================================================================================

// NOTE(mara): Fortunately, we can afford to make the particle system here very small
// and specific. There are two particle effects that can occur within Asteroids:
//     1. Bullet hit effects, a small instantaneous puff of of particle points that
//        expand outwards from a central point.
//     2. The lines of the player that float outwards when the player is killed.
struct Particle
{
    Vector2 position;
    Vector2 forward;

    float32 move_speed;

    float32 time_remaining;

    bool32 is_active;
};

struct ParticleSystem
{
    Vector2 position;

    float32 system_timer;

    float32 particle_lifetime_min;
    float32 particle_lifetime_max;

    float32 particle_move_speed_min;
    float32 particle_move_speed_max;

    int32 num_particles;
    Particle particles[MAX_PARTICLES];

    bool32 is_emitting;
};

struct GridSpace
{
    GridSpace *north;
    GridSpace *east;
    GridSpace *south;
    GridSpace *west;

    int32 num_asteroid_line_points;
    int32 num_bullets;
    int32 num_ufo_points;
    bool32 has_player;
};

struct Grid
{
    float32 space_width;
    float32 space_height;

    GridSpace spaces[NUM_GRID_SPACES_H * NUM_GRID_SPACES_V];
};

struct Player
{
    Vector2 position;
    Vector2 forward;
    Vector2 right;
    Vector2 velocity;

    float32 rotation;
    float32 rotation_speed;

    float32 maximum_velocity;
    float32 thrust_factor;
    float32 acceleration;
    float32 speed_damping_factor;

    float32 color_r;
    float32 color_g;
    float32 color_b;

    float32 death_timer;
    float32 invuln_timer;

    int32 lives;

    Vector2 points_local[5];
    Vector2 points_global[5];
};

struct Bullet
{
    Vector2 position;
    Vector2 forward;

    float32 time_remaining;

    bool32 is_friendly;
    bool32 is_active;
};

struct Asteroid
{
    Vector2 position;
    Vector2 forward;

    Vector2 points_local[MAX_ASTEROID_POINTS];
    Vector2 points_global[MAX_ASTEROID_POINTS];

    float32 speed;

    float32 color_r;
    float32 color_g;
    float32 color_b;

    int32 phase_index; // large = 2, medium = 1, small = 0

    bool32 is_active; // value that tracks whether or not this asteroid slot exists on the game screen.
};

struct UFO
{
    Vector2 position;
    Vector2 forward;

    Vector2 points[8]; // Global-space UFO points.

    bool32 started_on_left_side;

    float32 speed;
    float32 time_to_next_spawn;
    float32 time_to_next_bullet;
    float32 time_to_next_direction_change;

    bool32 is_small;
    float32 large_width;
    float32 large_inner_width;
    float32 large_top_width;
    float32 large_section_height;
    float32 small_width;
    float32 small_inner_width;
    float32 small_top_width;
    float32 small_section_height;

    float32 color_r;
    float32 color_g;
    float32 color_b;

    bool32 is_active;
};

enum GamePhase
{
    GAME_PHASE_ATTRACT_MODE = 0,
    GAME_PHASE_PLAY,
    GAME_PHASE_NAME_ENTRY
};

struct HighScore
{
    int32 score;
    char name[NAME_ENTRY_MAX_LENGTH + 1];
};

struct GameState
{
    GamePhase phase;

    Grid grid;

    Player player;
    int32 num_lives_at_start;
    int32 score;
    int32 level;

    Bullet bullets[MAX_BULLETS];
    float32 bullet_speed;
    float32 bullet_size;
    float32 bullet_lifespan_seconds;

    float32 asteroid_player_min_spawn_distance;

    Asteroid asteroids[MAX_ASTEROIDS];
    int32 asteroid_phase_sizes[4];
    int32 asteroid_phase_point_values[3];
    float32 asteroid_phase_speeds[3];
    float32 asteroid_speed_max;

    int32 asteroid_speed_increase_count;
    float32 asteroid_speed_increase_scalar;
    float32 time_until_next_speed_increase;

    // Chose to track this directly rather than iterate through asteroids every frame.
    int32 num_active_asteroids;
    int32 num_asteroids_at_start;

    UFO ufo;
    Bullet ufo_bullets[MAX_BULLETS];
    int32 ufo_large_point_value;
    int32 ufo_small_point_value;
    float32 ufo_bullet_time_min;
    float32 ufo_bullet_time_max;
    float32 ufo_bullet_speed;
    float32 ufo_bullet_size;
    float32 ufo_bullet_lifespan_seconds;
    float32 ufo_spawn_time_min;
    float32 ufo_spawn_time_max;
    float32 ufo_direction_change_time_min;
    float32 ufo_direction_change_time_max;

    ParticleSystem particle_system_splash[4];
    ParticleSystem particle_system_lines;

    RandomState random;

    FontData font;

    HighScore high_scores[MAX_HIGH_SCORES];
    char name_chars[NAME_ENTRY_MAX_ALLOWED_CHARS + 1];
    int32 name_index;
    int32 char_indices[NAME_ENTRY_MAX_LENGTH]; // Which char we're on in each specific entry space.
    char entered_name[NAME_ENTRY_MAX_LENGTH + 1];
    bool32 name_move_up_desired;
    bool32 name_move_down_desired;
    bool32 name_move_left_desired;
    bool32 name_move_right_desired;
    bool32 name_completion_desired;

    WAVESoundData sounds[SOUND_ASSET_COUNT];
    MemoryArena sound_arena;

    WAVESoundData test_wav;
    uint32 test_wav_sample_index;

    SoundStream *thrust_loop;
    SoundStream *ufo_loop;

    float32 beat_sound_countdown;
    float32 beat_sound_countdown_time;
    float32 beat_sound_countdown_time_min;
    float32 beat_sound_countdown_time_max;
    float32 beat_sound_countdown_decrement_amount;
    bool32 beat_sound_countdown_flip;
};

struct TransientState
{
    MemoryArena arena;
};

#endif
