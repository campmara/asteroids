#include "asteroids.h"

#include <math.h>

inline float32 Abs(float32 value)
{
    return fabsf(value);
}

inline float32 Cos(float32 radians)
{
    return cosf(radians);
}

inline float32 Sin(float32 radians)
{
    return sinf(radians);
}

inline int32 RoundFloat32ToInt32(float32 value)
{
    return (int32)(value + 0.5f);
}

inline uint32 RoundFloat32ToUInt32(float32 value)
{
    return (uint32)(value + 0.5f);
}

inline void ClampAngleRadians(float32 *radians)
{
    if (*radians < 0)
    {
        *radians += TWO_PI_32;
    }
    else if (*radians > TWO_PI_32)
    {
        *radians -= TWO_PI_32;
    }
}

inline void ConstrainInt32PointToBuffer(GameOffscreenBuffer *buffer, int32 *x, int32 *y)
{
    if (*x < 0)
    {
        *x += buffer->width;
    }
    else if (*x >= buffer->width)
    {
        *x -= buffer->width;
    }
    if (*y < 0)
    {
        *y += buffer->height;
    }
    else if (*y >= buffer->height)
    {
        *y -= buffer->height;
    }
}

inline void ConstrainFloat32PointToBuffer(GameOffscreenBuffer *buffer, float32 *x, float32 *y)
{
    float32 width = (float32)buffer->width;
    float32 height = (float32)buffer->height;

    if (*x < 0)
    {
        *x += width;
    }
    else if (*x >= width)
    {
        *x -= width;
    }
    if (*y < 0)
    {
        *y += height;
    }
    else if (*y >= height)
    {
        *y -= height;
    }
}

internal void DrawPixel(GameOffscreenBuffer *buffer,
                        int32 x, int32 y, int32 color)
{
    uint8 *byte_pos = ((uint8 *)buffer->memory +
                       x * buffer->bytes_per_pixel +
                       y * buffer->pitch); // Get the pixel pointer in the starting position.

    uint32 *pixel = (uint32 *)byte_pos;
    *pixel = color;
}

internal void DrawLine(GameOffscreenBuffer *buffer,
                       float32 x0, float32 y0,
                       float32 x1, float32 y1,
                       float32 r, float32 g, float32 b)
{
    bool32 is_steep = (Abs(y1 - y0) > Abs(x1 - x0));

    if (is_steep)
    {
        SWAP(float32, x0, y0);
        SWAP(float32, x1, y1);
    }

    // Flip values around if the start is greater than the end.
    if (x0 > x1)
    {
        SWAP(float32, x0, x1);
        SWAP(float32, y0, y1);
    }

    float32 dx = x1 - x0;
    float32 dy = Abs(y1 - y0);

    float32 error = dx / 2.0f;
    int32 y_step = (y0 < y1) ? 1 : -1;
    int32 y = RoundFloat32ToInt32(y0);

    int32 start_x = RoundFloat32ToInt32(x0);
    int32 end_x = RoundFloat32ToInt32(x1);

    uint32 color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                    (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                    (RoundFloat32ToUInt32(b * 255.0f) << 0));

    for (int32 x = start_x; x <= end_x; ++x)
    {
        int32 final_x = is_steep ? y : x;
        int32 final_y = is_steep ? x : y;
        ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);

        DrawPixel(buffer, final_x, final_y, color);

        error -= dy;
        if (error < 0)
        {
            y += y_step;
            error += dx;
        }
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
    PlayerState *player = &game_state->player;
    if (!memory->is_initialized)
    {
        player->x = 150.0f;
        player->y = 150.0f;
        player->rotation = 0.0f;
        player->forward_x = 0.0f;
        player->forward_y = 0.0f;
        player->velocity_x = 0.0f;
        player->velocity_y = 0.0f;

        player->maximum_velocity = 6.0f;
        player->thrust_factor = 128.0f;
        player->acceleration = 0.15f;
        player->speed_damping_factor = 0.99f;

        // Seed the RNG.
        game_state->random = {};
        SeedRandomLCG(&game_state->random, 131181);

        // Generate the first asteroids.
        game_state->num_asteroids = 5;
        for (int i = 0; i < game_state->num_asteroids; ++i)
        {
            game_state->asteroids[i] = {};
            Asteroid *asteroid = &game_state->asteroids[i];

            int32 rand_x = RandomInt32InRangeLCG(&game_state->random, 0, buffer->width);
            int32 rand_y = RandomInt32InRangeLCG(&game_state->random, 0, buffer->height);
            asteroid->x = (float32)rand_x;
            asteroid->y = (float32)rand_y;

            int32 rand_direction_angle = RandomInt32InRangeLCG(&game_state->random, 0, 360);
            float32 direction_radians = (float32)DEG2RAD * (float32)rand_direction_angle;
            asteroid->forward_x = Cos(direction_radians);
            asteroid->forward_y = Sin(direction_radians);

            int32 rand_speed = RandomInt32InRangeLCG(&game_state->random, 1, 128);
            asteroid->speed = (float32)rand_speed;

            asteroid->is_active = true;
        }

        memory->is_initialized = true;
    }

    if (!memory->is_initialized)
    {
        return;
    }

    float32 delta_time = (float32)time->delta_time;

    // Handle Input.
    float32 move_input_x = 0.0f;
    float32 move_input_y = 0.0f;
    for (int controller_index = 0; controller_index < ARRAY_COUNT(input->controllers); ++controller_index)
    {
        GameControllerInput *controller = GetController(input, controller_index);
        if (controller->is_analog)
        {

        }
        else
        {
            if (controller->move_up.ended_down)
            {
                move_input_y = -1.0f;
            }
            if (controller->move_down.ended_down)
            {
                move_input_y = 1.0f;
            }
            if (controller->move_left.ended_down)
            {
                move_input_x = -1.0f;
            }
            if (controller->move_right.ended_down)
            {
                move_input_x = 1.0f;
            }
        }
    }

    // Calculate the player's position.
    move_input_x *= 8.0f;

    player->rotation += move_input_x * (float32)time->delta_time;
    ClampAngleRadians(&player->rotation);

    // Get a local right vector by rotating the forward angle by pi / 2 clockwise.
    float32 right_angle_radians = player->rotation + (PI_32 / 2.0f);
    ClampAngleRadians(&right_angle_radians);

    player->forward_x = Cos(player->rotation);
    player->forward_y = Sin(player->rotation);
    player->right_x = Cos(right_angle_radians);
    player->right_y = Sin(right_angle_radians);

    float32 thrust = move_input_y * player->thrust_factor * delta_time;
    player->velocity_x -= player->forward_x * thrust * player->acceleration;
    player->velocity_y -= player->forward_y * thrust * player->acceleration;

    // Clamp the velocity to a maximum magnitude.
    if (player->velocity_x > player->maximum_velocity)
    {
        player->velocity_x = player->maximum_velocity;
    }
    if (player->velocity_x < -player->maximum_velocity)
    {
        player->velocity_x = -player->maximum_velocity;
    }
    if (player->velocity_y > player->maximum_velocity)
    {
        player->velocity_y = player->maximum_velocity;
    }
    if (player->velocity_y < -player->maximum_velocity)
    {
        player->velocity_y = -player->maximum_velocity;
    }

    player->velocity_x *= player->speed_damping_factor;
    player->velocity_y *= player->speed_damping_factor;

    player->x += player->velocity_x;
    player->y += player->velocity_y;
    ConstrainFloat32PointToBuffer(buffer, &player->x, &player->y);



    // Calculate the asteroid's position.
    //float32 new_asteroid_x = game_state->asteroid_x + 128.0f * delta_time;
    //float32 new_asteroid_y = game_state->asteroid_y + 128.0f * delta_time;
    //game_state->asteroid_x = new_asteroid_x;
    //game_state->asteroid_y = new_asteroid_y;
    //ConstrainFloat32PointToBuffer(buffer, &game_state->asteroid_x, &game_state->asteroid_y);

    // Clear the screen.
    DrawRectangle(buffer, 0.0f, 0.0f, (float32)buffer->width, (float32)buffer->height, 0.06f, 0.18f, 0.17f);

    // Calculate asteroid positions and draw them all in the same loop.
    for (int i = 0; i < MAX_NUM_ASTEROIDS; ++i)
    {
        Asteroid *asteroid = &game_state->asteroids[i];
        if (asteroid->is_active)
        {
            asteroid->x += asteroid->forward_x * asteroid->speed * delta_time;
            asteroid->y += asteroid->forward_y * asteroid->speed * delta_time;
            ConstrainFloat32PointToBuffer(buffer, &asteroid->x, &asteroid->y);

            DrawLine(buffer,
                     asteroid->x - 20.0f, asteroid->y - 5.0f,
                     asteroid->x - 5.0f, asteroid->y - 20.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x - 5.0f, asteroid->y - 20.0f,
                     asteroid->x + 5.0f, asteroid->y - 20.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x + 5.0f, asteroid->y - 20.0f,
                     asteroid->x + 20.0f, asteroid->y - 5.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x + 20.0f, asteroid->y - 5.0f,
                     asteroid->x + 20.0f, asteroid->y + 5.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x + 20.0f, asteroid->y + 5.0f,
                     asteroid->x + 5.0f, asteroid->y + 20.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x + 5.0f, asteroid->y + 20.0f,
                     asteroid->x - 5.0f, asteroid->y + 20.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x - 5.0f, asteroid->y + 20.0f,
                     asteroid->x - 20.0f, asteroid->y + 5.0f,
                     1.0f, 1.0f, 1.0f);
            DrawLine(buffer,
                     asteroid->x - 20.0f, asteroid->y + 5.0f,
                     asteroid->x - 20.0f, asteroid->y - 5.0f,
                     1.0f, 1.0f, 1.0f);
        }
    }

    // Draw an asteroid.
    /*
    DrawLine(buffer,
             game_state->asteroid_x - 20.0f, game_state->asteroid_y - 5.0f,
             game_state->asteroid_x - 5.0f, game_state->asteroid_y - 20.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x - 5.0f, game_state->asteroid_y - 20.0f,
             game_state->asteroid_x + 5.0f, game_state->asteroid_y - 20.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x + 5.0f, game_state->asteroid_y - 20.0f,
             game_state->asteroid_x + 20.0f, game_state->asteroid_y - 5.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x + 20.0f, game_state->asteroid_y - 5.0f,
             game_state->asteroid_x + 20.0f, game_state->asteroid_y + 5.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x + 20.0f, game_state->asteroid_y + 5.0f,
             game_state->asteroid_x + 5.0f, game_state->asteroid_y + 20.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x + 5.0f, game_state->asteroid_y + 20.0f,
             game_state->asteroid_x - 5.0f, game_state->asteroid_y + 20.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x - 5.0f, game_state->asteroid_y + 20.0f,
             game_state->asteroid_x - 20.0f, game_state->asteroid_y + 5.0f,
             1.0f, 1.0f, 1.0f);
    DrawLine(buffer,
             game_state->asteroid_x - 20.0f, game_state->asteroid_y + 5.0f,
             game_state->asteroid_x - 20.0f, game_state->asteroid_y - 5.0f,
             1.0f, 1.0f, 1.0f);
    */

    // Draw the ship.
    float32 forward_point_x = player->x + player->forward_x * 20.0f;
    float32 forward_point_y = player->y + player->forward_y * 20.0f;
    float32 left_point_x = player->x - player->right_x * 10.0f;
    float32 left_point_y = player->y - player->right_y * 10.0f;
    left_point_x -= player->forward_x * 10.0f;
    left_point_y -= player->forward_y * 10.0f;
    float32 right_point_x = player->x + player->right_x * 10.0f;
    float32 right_point_y = player->y + player->right_y * 10.0f;
    right_point_x -= player->forward_x * 10.0f;
    right_point_y -= player->forward_y * 10.0f;

    DrawLine(buffer,
             left_point_x, left_point_y,
             forward_point_x, forward_point_y,
             1.0f, 1.0f, 0.0f);
    DrawLine(buffer,
             right_point_x, right_point_y,
             forward_point_x, forward_point_y,
             1.0f, 1.0f, 0.0f);
    DrawLine(buffer,
             left_point_x, left_point_y,
             right_point_x, right_point_y,
             1.0f, 1.0f, 0.0f);

    // Draw ship's thrust fire.
    float32 thrust_fire_pos_x = player->x - player->forward_x * 15.0f;
    float32 thrust_fire_pos_y = player->y - player->forward_y * 15.0f;

    if (move_input_x != 0 || move_input_y != 0)
    {
        DrawLine(buffer,
                 thrust_fire_pos_x + player->right_x * 5.0f, thrust_fire_pos_y + player->right_y * 5.0f,
                 thrust_fire_pos_x - player->right_x * 5.0f, thrust_fire_pos_y - player->right_y * 5.0f,
                 1.0f, 0.0f, 0.0f);
        DrawLine(buffer,
                 thrust_fire_pos_x + player->right_x * 5.0f, thrust_fire_pos_y + player->right_y * 5.0f,
                 thrust_fire_pos_x - player->forward_x * 10.0f, thrust_fire_pos_y - player->forward_y * 10.0f,
                 1.0f, 0.0f, 0.0f);
        DrawLine(buffer,
                 thrust_fire_pos_x - player->right_x * 5.0f, thrust_fire_pos_y - player->right_y * 5.0f,
                 thrust_fire_pos_x - player->forward_x * 10.0f, thrust_fire_pos_y - player->forward_y * 10.0f,
                 1.0f, 0.0f, 0.0f);
    }

#if 0
    // Draw the ship's forward vector.
    DrawLine(buffer,
             player->x, player->y,
             player->x + player->forward_x * 30.0f, player->y + player->forward_y * 30.0f,
             1.0f, 0.0f, 0.0f);
    DrawLine(buffer,
             player->x, player->y,
             player->x + player->right_x * 30.0f, player->y + player->right_y * 30.0f,
             1.0f, 0.0f, 0.0f);
#endif
}
