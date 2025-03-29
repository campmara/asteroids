#include "asteroids.h"

#include <math.h>

// =================================================================================================
// MATHEMATICS UTILITIES
// =================================================================================================

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

// =================================================================================================
// DRAWING
// =================================================================================================

internal void DrawPixel(GameOffscreenBuffer *buffer,
                        int32 x, int32 y, int32 color)
{
    uint8 *byte_pos = ((uint8 *)buffer->memory +
                       x * buffer->bytes_per_pixel +
                       y * buffer->pitch); // Get the pixel pointer in the starting position.

    uint32 *pixel = (uint32 *)byte_pos;
    *pixel = color;
}

// Bresenham's line algorithm.
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

// Jesko's midpoint circle algorithm.
internal void DrawCircle(GameOffscreenBuffer *buffer,
                         float32 real_x, float32 real_y, float32 real_radius,
                         float32 r, float32 g, float32 b)
{
    uint32 color = ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
                    (RoundFloat32ToUInt32(g * 255.0f) << 8) |
                    (RoundFloat32ToUInt32(b * 255.0f) << 0));

    int32 pos_x = RoundFloat32ToInt32(real_x);
    int32 pos_y = RoundFloat32ToInt32(real_y);

    int32 radius = RoundFloat32ToInt32(real_radius);

    int32 x = radius;
    int32 y = 0;
    int32 p = 1 - radius;

    // Draw the first point.
    int32 final_x = pos_x + x;
    int32 final_y = pos_y + y;
    ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
    DrawPixel(buffer, final_x, final_y, color);

    while (x > y)
    {
        y++;
        if (p <= 0)
        {
            p += 2 * y + 1;
        }
        else
        {
            x--;
            p += 2 * y - 2 * x + 1;
        }

        if (x < y)
        {
            break;
        }

        final_x = pos_x + x;
        final_y = pos_y + y;
        ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        final_x = pos_x + -x;
        final_y = pos_y + y;
        ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        final_x = pos_x + x;
        final_y = pos_y + -y;
        ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        final_x = pos_x + -x;
        final_y = pos_y + -y;
        ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        if (x != y)
        {
            final_x = pos_x + y;
            final_y = pos_y + x;
            ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);

            final_x = pos_x + -y;
            final_y = pos_y + x;
            ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);

            final_x = pos_x + y;
            final_y = pos_y + -x;
            ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);

            final_x = pos_x + -y;
            final_y = pos_y + -x;
            ConstrainInt32PointToBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);
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

// =================================================================================================
// COLLISION
// =================================================================================================



// =================================================================================================
// GAME-RELATED
// =================================================================================================

internal void ComputePlayerLines(Player *player)
{
    float32 forward_point_x = player->position.x + player->forward.x * 20.0f;
    float32 forward_point_y = player->position.y + player->forward.y * 20.0f;
    float32 left_point_x = player->position.x - player->right.x * 10.0f;
    float32 left_point_y = player->position.y - player->right.y * 10.0f;
    left_point_x -= player->forward.x * 10.0f;
    left_point_y -= player->forward.y * 10.0f;
    float32 right_point_x = player->position.x + player->right.x * 10.0f;
    float32 right_point_y = player->position.y + player->right.y * 10.0f;
    right_point_x -= player->forward.x * 10.0f;
    right_point_y -= player->forward.y * 10.0f;
    player->lines[0] = {};
    player->lines[0].a = { left_point_x, left_point_y };
    player->lines[0].b = { forward_point_x, forward_point_y };
    player->lines[1].a = { right_point_x, right_point_y };
    player->lines[1].b = { forward_point_x, forward_point_y };
    player->lines[2].a = { left_point_x, left_point_y };
    player->lines[2].b = { right_point_x, right_point_y };
}

internal void GenerateAsteroid(GameState *game_state, GameOffscreenBuffer *buffer, int32 asteroid_slot_index)
{
    game_state->asteroids[asteroid_slot_index] = {};
    Asteroid *asteroid = &game_state->asteroids[asteroid_slot_index];

    // Position.
    int32 rand_x = RandomInt32InRangeLCG(&game_state->random, 0, buffer->width);
    int32 rand_y = RandomInt32InRangeLCG(&game_state->random, 0, buffer->height);
    asteroid->position.x = (float32)rand_x;
    asteroid->position.y = (float32)rand_y;

    // Forward direction.
    int32 rand_direction_angle = RandomInt32InRangeLCG(&game_state->random, 0, 360);
    float32 direction_radians = (float32)DEG2RAD * (float32)rand_direction_angle;
    asteroid->forward.x = Cos(direction_radians);
    asteroid->forward.y = Sin(direction_radians);

    // Points.
    for (int point = 0; point < MAX_ASTEROID_POINTS; ++point)
    {
        asteroid->points[point] = {};

        float32 pct = (float32)point / (float32)MAX_ASTEROID_POINTS;
        float32 point_radians_around_circle = TWO_PI_32 * pct;

        int32 rand_offset_for_point = RandomInt32InRangeLCG(&game_state->random,
                                                            game_state->asteroid_size_tier_three,
                                                            game_state->asteroid_size_tier_four);
        asteroid->points[point].x = Cos(point_radians_around_circle) * (float32)rand_offset_for_point;
        asteroid->points[point].y = Sin(point_radians_around_circle) * (float32)rand_offset_for_point;
    }

    // Speed.
    int32 rand_speed = RandomInt32InRangeLCG(&game_state->random, 1, 128);
    asteroid->speed = (float32)rand_speed;

    asteroid->is_active = true;
}

// =================================================================================================
// GAME UPDATE AND RENDER (GUAR)
// =================================================================================================

// Function Signature:
// GameUpdateAndRender(GameMemory *memory, GameTime *time, GameInput *input, GameOffscreenBuffer *buffer)
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    ASSERT(sizeof(GameState) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    Player *player = &game_state->player;
    if (!memory->is_initialized)
    {
        player->position.x = 150.0f;
        player->position.y = 150.0f;
        player->forward.x = 1.0f;
        player->forward.y = 0.0f;
        player->right.x = 0.0f;
        player->right.y = 1.0f;
        player->velocity.x = 0.0f;
        player->velocity.y = 0.0f;

        player->rotation = 0.0f;
        player->rotation_speed = 7.0f;

        player->maximum_velocity = 6.0f;
        player->thrust_factor = 128.0f;
        player->acceleration = 0.15f;
        player->speed_damping_factor = 0.99f;

        player->color_r = 1.0f;
        player->color_g = 1.0f;
        player->color_b = 0.0f;

        ComputePlayerLines(player);

        game_state->bullet_speed = 512.0f;
        game_state->bullet_lifespan_seconds = 2.5f;

        // Seed the RNG.
        game_state->random = {};
        SeedRandomLCG(&game_state->random, 131181);

        // Generate the first asteroids.
        game_state->num_asteroids = 5;
        game_state->asteroid_size_tier_one = 5;
        game_state->asteroid_size_tier_two = 18;
        game_state->asteroid_size_tier_three = 25;
        game_state->asteroid_size_tier_four = 55;
        for (int i = 0; i < game_state->num_asteroids; ++i)
        {
            GenerateAsteroid(game_state, buffer, i);
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
    bool32 is_bullet_desired = false;
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
            if (controller->action_down.ended_down && controller->action_down.half_transition_count > 0)
            {
                is_bullet_desired = true;
            }
        }
    }

    // Calculate the player's position.
    move_input_x *= player->rotation_speed;

    player->rotation += move_input_x * (float32)time->delta_time;
    ClampAngleRadians(&player->rotation);

    // Get a local right vector by rotating the forward angle by pi / 2 clockwise.
    float32 right_angle_radians = player->rotation + (PI_32 / 2.0f);
    ClampAngleRadians(&right_angle_radians);

    player->forward.x = Cos(player->rotation);
    player->forward.y = Sin(player->rotation);
    player->right.x = Cos(right_angle_radians);
    player->right.y = Sin(right_angle_radians);

    float32 thrust = move_input_y * player->thrust_factor * delta_time;
    player->velocity.x -= player->forward.x * thrust * player->acceleration;
    player->velocity.y -= player->forward.y * thrust * player->acceleration;

    // Clamp the velocity to a maximum magnitude.
    if (player->velocity.x > player->maximum_velocity)
    {
        player->velocity.x = player->maximum_velocity;
    }
    if (player->velocity.x < -player->maximum_velocity)
    {
        player->velocity.x = -player->maximum_velocity;
    }
    if (player->velocity.y > player->maximum_velocity)
    {
        player->velocity.y = player->maximum_velocity;
    }
    if (player->velocity.y < -player->maximum_velocity)
    {
        player->velocity.y = -player->maximum_velocity;
    }

    player->velocity.x *= player->speed_damping_factor;
    player->velocity.y *= player->speed_damping_factor;

    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    ConstrainFloat32PointToBuffer(buffer, &player->position.x, &player->position.y);

    // Update Player & Asteroid cached line positions.
    ComputePlayerLines(player);

    // Test Collision Player - Asteroid
    bool32 is_player_colliding_with_asteroid = false;
    for (int player_line_index = 0; player_line_index < ARRAY_COUNT(player->lines); ++player_line_index)
    {
        Line *player_line = &player->lines[player_line_index];
        for (int asteroid_index = 0; asteroid_index < ARRAY_COUNT(game_state->asteroids); ++asteroid_index)
        {
            if (game_state->asteroids[asteroid_index].is_active)
            {
                Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                for (int asteroid_line_index = 0; asteroid_line_index < ARRAY_COUNT(asteroid->lines); ++asteroid_line_index)
                {
                    /*
                    if (TestLineIntersection(player_line, &asteroid->lines[asteroid_line_index]))
                    {
                        is_player_colliding_with_asteroid = true;
                        break;
                    }
                    */
                }
            }
        }
    }

    // Test Collision Asteroid - Bullet

    // Test Collision Player - Bullet

    // Clear the screen.
    DrawRectangle(buffer, 0.0f, 0.0f, (float32)buffer->width, (float32)buffer->height, 0.06f, 0.18f, 0.17f);

    // Figure out if we need bullets and generate them if so.
    if (is_bullet_desired)
    {
        // Find an available bullet slot, and do nothing if they're all active.
        for (int i = 0; i < MAX_BULLETS; ++i)
        {
            if (!game_state->bullets[i].is_active)
            {
                Bullet *bullet = &game_state->bullets[i];
                bullet->position.x = player->position.x + player->forward.x * 20.0f;
                bullet->position.y = player->position.y + player->forward.y * 20.0f;
                bullet->forward.x = player->forward.x;
                bullet->forward.y = player->forward.y;
                bullet->time_remaining = game_state->bullet_lifespan_seconds;
                bullet->is_active = true;
                break;
            }
        }

        is_bullet_desired = false;
    }

    // Calculate bullet positions and draw them all in the same loop.
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        Bullet *bullet = &game_state->bullets[i];
        if (bullet->is_active)
        {
            bullet->time_remaining -= delta_time;
            if (bullet->time_remaining <= 0)
            {
                bullet->is_active = false;
                continue;
            }

            bullet->position.x += bullet->forward.x * game_state->bullet_speed * delta_time;
            bullet->position.y += bullet->forward.y * game_state->bullet_speed * delta_time;
            ConstrainFloat32PointToBuffer(buffer, &bullet->position.x, &bullet->position.y);

            /*
            DrawLine(buffer,
                     bullet->position.x, bullet->position.y,
                     bullet->position.x + bullet->forward.x * 5.0f, bullet->position.y + bullet->forward.y * 5.0f,
                     0.45f, 0.9f, 0.76f);
            */
            DrawCircle(buffer,
                       bullet->position.x, bullet->position.y, 8.0f,
                       0.45f, 0.9f, 0.76f);
        }
    }

    // Calculate asteroid positions and draw them all in the same loop.
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        Asteroid *asteroid = &game_state->asteroids[i];
        if (asteroid->is_active)
        {
            asteroid->position.x += asteroid->forward.x * asteroid->speed * delta_time;
            asteroid->position.y += asteroid->forward.y * asteroid->speed * delta_time;
            ConstrainFloat32PointToBuffer(buffer, &asteroid->position.x, &asteroid->position.y);

            // Draw the lines that comprise the asteroid.
            for (int point = 0; point < MAX_ASTEROID_POINTS; ++point)
            {
                int next_point = (point + 1) % MAX_ASTEROID_POINTS;
                DrawLine(buffer,
                         asteroid->position.x + asteroid->points[point].x,
                         asteroid->position.y + asteroid->points[point].y,
                         asteroid->position.x + asteroid->points[next_point].x,
                         asteroid->position.y + asteroid->points[next_point].y,
                         0.95f, 0.95f, 0.95f);
            }
        }
    }

    // Draw the player ship.
    DrawLine(buffer,
             player->lines[0].a.x, player->lines[0].a.y,
             player->lines[0].b.x, player->lines[0].b.y,
             player->color_r, player->color_g, player->color_b);
    DrawLine(buffer,
             player->lines[1].a.x, player->lines[1].a.y,
             player->lines[1].b.x, player->lines[1].b.y,
             player->color_r, player->color_g, player->color_b);
    DrawLine(buffer,
             player->lines[2].a.x, player->lines[2].a.y,
             player->lines[2].b.x, player->lines[2].b.y,
             player->color_r, player->color_g, player->color_b);

    // Draw ship's thrust fire.
    float32 thrust_fire_pos_x = player->position.x - player->forward.x * 15.0f;
    float32 thrust_fire_pos_y = player->position.y - player->forward.y * 15.0f;

    if (move_input_x != 0 || move_input_y != 0)
    {
        DrawLine(buffer,
                 thrust_fire_pos_x + player->right.x * 5.0f, thrust_fire_pos_y + player->right.y * 5.0f,
                 thrust_fire_pos_x - player->right.x * 5.0f, thrust_fire_pos_y - player->right.y * 5.0f,
                 1.0f, 0.0f, 0.0f);
        DrawLine(buffer,
                 thrust_fire_pos_x + player->right.x * 5.0f, thrust_fire_pos_y + player->right.y * 5.0f,
                 thrust_fire_pos_x - player->forward.x * 10.0f, thrust_fire_pos_y - player->forward.y * 10.0f,
                 1.0f, 0.0f, 0.0f);
        DrawLine(buffer,
                 thrust_fire_pos_x - player->right.x * 5.0f, thrust_fire_pos_y - player->right.y * 5.0f,
                 thrust_fire_pos_x - player->forward.x * 10.0f, thrust_fire_pos_y - player->forward.y * 10.0f,
                 1.0f, 0.0f, 0.0f);
    }

#if 0
    // Draw the ship's forward vector.
    DrawLine(buffer,
             player->x, player->y,
             player->x + player->forward.x * 30.0f, player->y + player->forward.y * 30.0f,
             1.0f, 0.0f, 0.0f);
    DrawLine(buffer,
             player->x, player->y,
             player->x + player->right.x * 30.0f, player->y + player->right.y * 30.0f,
             1.0f, 0.0f, 0.0f);
#endif
}
