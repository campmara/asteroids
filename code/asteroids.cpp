#include "asteroids.h"

inline int32 RoundFloat32ToInt32(float32 value)
{
    return (int32)(value + 0.5f);
}

inline uint32 RoundFloat32ToUInt32(float32 value)
{
    return (uint32)(value + 0.5f);
}

internal void DrawLine(GameOffscreenBuffer *buffer,
                       float32 float_start_x, float32 float_start_y,
                       float32 float_end_x, float32 float_end_y,
                       float32 r, float32 g, float32 b)
{
    int32 start_x = RoundFloat32ToInt32(float_start_x);
    int32 start_y = RoundFloat32ToInt32(float_start_y);
    int32 end_x = RoundFloat32ToInt32(float_end_x);
    int32 end_y = RoundFloat32ToInt32(float_end_y);

    if (start_x < 0)
    {
        start_x = 0;
    }
    if (start_y < 0)
    {
        start_y = 0;
    }
    if (end_x > buffer->width)
    {
        end_x = buffer->width;
    }
    if (end_y > buffer->height)
    {
        end_y = buffer->height;
    }

    uint32 color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                    (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                    (RoundFloat32ToUInt32(b * 255.0f) << 0));

    int32 delta_x = end_x - start_x;
    int32 delta_y = end_y - start_y;
    int32 slope = (2 * delta_y) - delta_x;

    int y_pos = start_y;
    uint8 *byte_pos = ((uint8 *)buffer->memory +
                    start_x * buffer->bytes_per_pixel +
                    start_y * buffer->pitch); // Get the pixel pointer in the starting position.

    for (int x_pos = start_x; x_pos <= end_x; ++x_pos)
    {
        uint32 *pixel = (uint32 *)byte_pos;
        *pixel = color;

        if (slope > 0)
        {
            y_pos++;
            slope -= 2 * delta_x;
            byte_pos += buffer->pitch;
        }
        slope += 2 * delta_y;

        byte_pos += buffer->bytes_per_pixel;
    }
}

internal void DrawRectangle(GameOffscreenBuffer *buffer,
                            float32 real_min_x, float32 real_min_y,
                            float32 real_max_x, float32 real_max_y,
                            float32 r, float32 g, float32 b) // 0 - 1 values.
{
    int32 min_x = RoundFloat32ToInt32(real_min_x);
    int32 min_y = RoundFloat32ToInt32(real_min_y);
    int32 max_x = RoundFloat32ToInt32(real_max_x);
    int32 max_y = RoundFloat32ToInt32(real_max_y);

    if (min_x < 0)
    {
        min_x = 0;
    }

    if (min_y < 0)
    {
        min_y = 0;
    }

    if (max_x > buffer->width)
    {
        max_x = buffer->width;
    }

    if (max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    // BIT PATTERN: 0x AA RR GG BB
    uint32 color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                    (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                    (RoundFloat32ToUInt32(b * 255.0f) << 0));

    // Start with the top left pixel.
    uint8 *row = ((uint8 *)buffer->memory +
                    min_x * buffer->bytes_per_pixel +
                    min_y * buffer->pitch);
    for (int y = min_y; y < max_y; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = min_x; x < max_x; ++x)
        {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

// Function Signature:
// GameUpdateAndRender(GameMemory *memory, GameTime *time, GameInput *input, GameOffscreenBuffer *buffer)
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    ASSERT(sizeof(GameState) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    if (!memory->is_initialized)
    {
        game_state->player_x = 150;
        game_state->player_y = 150;

        memory->is_initialized = true;
    }

    for (int controller_index = 0; controller_index < ARRAY_COUNT(input->controllers); ++controller_index)
    {
        GameControllerInput *controller = GetController(input, controller_index);
        if (controller->is_analog)
        {

        }
        else
        {
            // TODO(mara): asteroids movement.
            float32 d_player_x = 0.0f;
            float32 d_player_y = 0.0f;

            if (controller->move_up.ended_down)
            {
                d_player_y = -1.0f;
            }
            if (controller->move_down.ended_down)
            {
                d_player_y = 1.0f;
            }
            if (controller->move_left.ended_down)
            {
                d_player_x = -1.0f;
            }
            if (controller->move_right.ended_down)
            {
                d_player_x = 1.0f;
            }
            d_player_x *= 128.0f;
            d_player_y *= 128.0f;

            float32 new_player_x = game_state->player_x + d_player_x * (float32)time->delta_time;
            float32 new_player_y = game_state->player_y + d_player_y * (float32)time->delta_time;
            game_state->player_x = new_player_x;
            game_state->player_y = new_player_y;
        }
    }

    // Clear the screen.
    DrawRectangle(buffer, 0.0f, 0.0f, (float32)buffer->width, (float32)buffer->height, 0.06f, 0.18f, 0.17f);

    float32 dest_x = game_state->player_x + 300.0f;
    float32 dest_y = game_state->player_y + 300.0f;

    DrawLine(buffer,
             game_state->player_x, game_state->player_y,
             dest_x, dest_y,
             1.0f, 1.0f, 0.0f);
}
