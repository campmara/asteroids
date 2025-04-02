#include "asteroids.h"

#include <math.h>
#include <ctime>
#include <stdio.h>

// =================================================================================================
// MATHEMATICS UTILITIES
// =================================================================================================

inline float32 Abs(float32 value)
{
    return fabsf(value);
}

inline float32 Sqrt(float32 value)
{
    return sqrtf(value);
}

inline float32 Cos(float32 radians)
{
    return cosf(radians);
}

inline float32 Sin(float32 radians)
{
    return sinf(radians);
}

inline float32 SqrMagnitude(Vector2 vector)
{
    return (vector.x * vector.x) + (vector.y * vector.y);
}

inline float32 Magnitude(Vector2 vector)
{
    return Sqrt(SqrMagnitude(vector));
}

inline Vector2 Normalize(Vector2 vector)
{
    float mag = Magnitude(vector);
    Vector2 result = vector;
    result.x /= mag;
    result.y /= mag;
    return result;
}

inline float32 Dot(Vector2 a, Vector2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline int32 RoundFloat32ToInt32(float32 value)
{
    return (int32)(value + 0.5f);
}

inline uint32 RoundFloat32ToUInt32(float32 value)
{
    return (uint32)(value + 0.5f);
}

inline int32 FloorFloat32ToInt32(float32 value)
{
    return (int32)floorf(value);
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

inline void WrapInt32PointAroundBuffer(GameOffscreenBuffer *buffer, int32 *x, int32 *y)
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

inline void WrapFloat32PointAroundBuffer(GameOffscreenBuffer *buffer, float32 *x, float32 *y)
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

inline uint32 MakeColor(float32 r, float32 g, float32 b)
{
    // BIT PATTERN: 0x AA RR GG BB
    return ((RoundFloat32ToUInt32(r * 255.0f) << 16) |
            (RoundFloat32ToUInt32(g * 255.0f) << 8) |
            (RoundFloat32ToUInt32(b * 255.0f) << 0));
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

    uint32 color = MakeColor(r, g, b);

    for (int32 x = start_x; x <= end_x; ++x)
    {
        int32 final_x = is_steep ? y : x;
        int32 final_y = is_steep ? x : y;
        WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);

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
    uint32 color = MakeColor(r, g, b);

    int32 pos_x = RoundFloat32ToInt32(real_x);
    int32 pos_y = RoundFloat32ToInt32(real_y);

    int32 radius = RoundFloat32ToInt32(real_radius);

    int32 x = radius;
    int32 y = 0;
    int32 p = 1 - radius;

    // Draw the first point.
    int32 final_x = pos_x + x;
    int32 final_y = pos_y + y;
    WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
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
        WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        final_x = pos_x + -x;
        final_y = pos_y + y;
        WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        final_x = pos_x + x;
        final_y = pos_y + -y;
        WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        final_x = pos_x + -x;
        final_y = pos_y + -y;
        WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
        DrawPixel(buffer, final_x, final_y, color);

        if (x != y)
        {
            final_x = pos_x + y;
            final_y = pos_y + x;
            WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);

            final_x = pos_x + -y;
            final_y = pos_y + x;
            WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);

            final_x = pos_x + y;
            final_y = pos_y + -x;
            WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);

            final_x = pos_x + -y;
            final_y = pos_y + -x;
            WrapInt32PointAroundBuffer(buffer, &final_x, &final_y);
            DrawPixel(buffer, final_x, final_y, color);
        }
    }
}

internal void DrawFilledRectangle(GameOffscreenBuffer *buffer,
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

    uint32 color = MakeColor(r, g, b);

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

internal void DrawUnfilledRectangle(GameOffscreenBuffer *buffer,
                                    float32 real_min_x, float32 real_min_y,
                                    float32 real_max_x, float32 real_max_y,
                                    float32 r, float32 g, float32 b)
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

    uint32 color = MakeColor(r, g, b);

    // Start with the top left pixel.
    uint8 *row = ((uint8 *)buffer->memory +
                    min_x * buffer->bytes_per_pixel +
                    min_y * buffer->pitch);
    for (int y = min_y; y < max_y; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = min_x; x < max_x; ++x)
        {
            if (x == min_x || y == min_y || x == max_x - 1 || y == max_y - 1)
            {
                *pixel++ = color;
            }
            else
            {
                pixel++;
            }
        }
        row += buffer->pitch;
    }
}

internal void DrawLetterFromFont(GameOffscreenBuffer *buffer, FontData *font,
                                 char letter, float32 scale, int32 ascent, int32 descent, int32 kern,
                                 float32 real_x, float32 real_y,
                                 float32 r, float32 g, float32 b)
{
    int32 advance_width, left_side_bearing;
    stbtt_GetCodepointHMetrics(&font->stb_font_info, letter, &advance_width, &left_side_bearing);

    int width, height, x_offset, y_offset;
    uchar8 *bitmap = stbtt_GetCodepointBitmap(&font->stb_font_info, 0, scale,
                                              letter, &width, &height, &x_offset, &y_offset);

    int32 x = RoundFloat32ToInt32(real_x) + x_offset;
    int32 y = RoundFloat32ToInt32(real_y) + y_offset;

    // Adjust x by advance width and kerning.
    x += RoundFloat32ToInt32((float32)advance_width * scale);
    x += RoundFloat32ToInt32((float32)kern * scale);

    // Adjust y by ascent.
    y += ascent;

    int32 max_x = x + width;
    int32 max_y = y + height;

    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (max_x > buffer->width)
    {
        max_x = buffer->width;
    }
    if (max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    uint32 color = MakeColor(r, g, b);

    // Start with the top left pixel.
    uchar8 *src = bitmap;
    uint8 *dest_row = ((uint8 *)buffer->memory +
                    x * buffer->bytes_per_pixel +
                    y * buffer->pitch);
    for (int this_y = y; this_y < max_y; ++this_y)
    {
        uint32 *dest = (uint32 *)dest_row;
        for (int this_x = x; this_x < max_x; ++this_x)
        {
            uchar8 alpha = *src++;

            if (alpha != '\0')
            {
                *dest++ = (alpha << 24) | color;
            }
            else
            {
                dest++;
            }
        }
        dest_row += buffer->pitch;
    }

    stbtt_FreeBitmap(bitmap, 0);
}

internal void DrawString(GameOffscreenBuffer *buffer, FontData *font,
                         char *str, uint32 str_length, float32 pixel_height,
                         float32 start_x, float32 start_y, float32 letter_spacing,
                         float32 r, float32 g, float32 b)
{
    float32 scale = stbtt_ScaleForPixelHeight(&font->stb_font_info, pixel_height);
    int32 ascent = RoundFloat32ToInt32((float32)font->ascent * scale);
    int32 descent = RoundFloat32ToInt32((float32)font->descent * scale);

    for (uint32 i = 0; i < str_length; ++i)
    {
        if (str[i] == '\0')
        {
            break;
        }

        int32 kern = stbtt_GetCodepointKernAdvance(&font->stb_font_info, str[i], str[i + 1]);

        DrawLetterFromFont(buffer, font,
                           str[i], scale, ascent, descent, kern,
                           start_x + (i * letter_spacing), start_y,
                           r, g, b);
    }
}

// =================================================================================================
// COLLISION GRID & INTERSECTION TESTS
// =================================================================================================

internal void ConstructGridPartition(Grid *grid, GameOffscreenBuffer *buffer)
{
    grid->space_width = (float32)buffer->width / (float32)NUM_GRID_SPACES_H;
    grid->space_height = (float32)buffer->height / (float32)NUM_GRID_SPACES_V;
    uint32 total_spaces = NUM_GRID_SPACES_H * NUM_GRID_SPACES_V;
    for (int i = 0; i < ARRAY_COUNT(grid->spaces); ++i)
    {
        uint32 north_index = (i - NUM_GRID_SPACES_H) % total_spaces;
        uint32 east_index = (i + 1) % NUM_GRID_SPACES_H;
        uint32 south_index = (i + NUM_GRID_SPACES_H) % total_spaces;
        uint32 west_index = (i - 1) < 0 ? (i - 1) + NUM_GRID_SPACES_H : (i - 1) % NUM_GRID_SPACES_H;

        grid->spaces[i].north = &grid->spaces[north_index];
        grid->spaces[i].east = &grid->spaces[east_index];
        grid->spaces[i].south = &grid->spaces[south_index];
        grid->spaces[i].west = &grid->spaces[west_index];
    }
}

inline uint32 GetGridPosition(GameOffscreenBuffer *buffer, Grid *grid, float32 x, float32 y)
{
    WrapFloat32PointAroundBuffer(buffer, &x, &y);

    uint32 grid_x = FloorFloat32ToInt32(x / grid->space_width);
    uint32 grid_y = FloorFloat32ToInt32(y / grid->space_height);
    return grid_x + (grid_y * NUM_GRID_SPACES_H);
}

internal bool32 TestLineIntersection(Vector2 a, Vector2 b, Vector2 c, Vector2 d)
{
    float32 alpha_numerator = ((d.x - c.x) * (c.y - a.y)) - ((d.y - c.y) * (c.x - a.x));
    float32 beta_numerator = ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
    float32 denominator = ((d.x - c.x) * (b.y - a.y)) - ((d.y - c.y) * (b.x - a.x));

    float32 alpha = alpha_numerator / denominator;
    float32 beta = beta_numerator / denominator;

    /*
    if (denominator > -EPSILON && denominator < EPSILON) // denominator == 0.
    {
        // Parallel lines.

        if (alpha_numerator - denominator >= -EPSILON && alpha_numerator - denominator <= EPSILON)  // alpha_numerator == denominator == 0.
        {
            // Collinear lines. We count this as an intersection.
            //return true;
        }
    }
    */

    // Are alpha and beta are between 0 and 1?
    return alpha >= 0.0f && alpha <= 1.0f && beta >= 0.0f && beta <= 1.0f;
}

internal Vector2 ClosestPointOnSegmentToPoint(Vector2 line_a, Vector2 line_b, Vector2 check_point)
{
    Vector2 tangent = line_b - line_a;
    if (Dot((check_point - line_a), tangent) <= 0.0f)
    {
        return line_a;
    }
    if (Dot((check_point - line_b), tangent) >= 0.0f)
    {
        return line_b;
    }
    Vector2 norm_tangent = { tangent.x, tangent.y };
    float32 mag = Magnitude(tangent);
    norm_tangent.x *= (1.0f / mag);
    norm_tangent.y *= (1.0f / mag);
    Vector2 relative_pos = check_point - line_a;
    float32 dot = Dot(norm_tangent, relative_pos);
    Vector2 offset = { norm_tangent.x * dot, norm_tangent.y * dot };
    return line_a + offset;
}

internal bool32 TestLineCircleIntersection(Vector2 line_a, Vector2 line_b,
                                           Vector2 circle_pos, float32 circle_radius)
{
    Vector2 delta = circle_pos - ClosestPointOnSegmentToPoint(line_a, line_b, circle_pos);
    return Dot(delta, delta) <= (circle_radius * circle_radius);
}

// =================================================================================================
// LINE GENERATION FUNCTIONS
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

internal void ComputeAsteroidLines(Asteroid *asteroid)
{
    for (int point = 0; point < MAX_ASTEROID_POINTS; ++point)
    {
        int next_point = (point + 1) % MAX_ASTEROID_POINTS;
        asteroid->lines[point] = {
            {
                asteroid->position.x + asteroid->points[point].x,
                asteroid->position.y + asteroid->points[point].y,
            },
            {
                asteroid->position.x + asteroid->points[next_point].x,
                asteroid->position.y + asteroid->points[next_point].y,
            }
        };
    }
}

internal void ComputeUFOPoints(UFO *ufo)
{
    float32 ufo_width = ufo->is_small ? ufo->small_width : ufo->large_width;
    float32 ufo_inner_width = ufo->is_small ? ufo->small_inner_width : ufo->large_inner_width;
    float32 ufo_top_width = ufo->is_small ? ufo->small_top_width : ufo->large_top_width;
    float32 ufo_section_height = ufo->is_small ? ufo->small_section_height : ufo->large_section_height;

    ufo->points[0] = { ufo->position.x - (ufo_width / 2.0f), ufo->position.y };
    ufo->points[1] = { ufo->position.x - (ufo_inner_width / 2.0f), ufo->position.y - ufo_section_height };
    ufo->points[2] = { ufo->position.x - (ufo_top_width / 2.0f), ufo->position.y - (ufo_section_height * 2.0f) };
    ufo->points[3] = { ufo->points[2].x + ufo_top_width, ufo->points[2].y };
    ufo->points[4] = { ufo->points[1].x + ufo_inner_width, ufo->points[1].y };
    ufo->points[5] = { ufo->points[0].x + ufo_width, ufo->points[0].y };
    ufo->points[6] = { ufo->position.x + (ufo_inner_width / 2.0f), ufo->position.y + ufo_section_height };
    ufo->points[7] = { ufo->points[6].x - ufo_inner_width, ufo->points[6].y };
}

// =================================================================================================
// ASTEROID-RELATED
// =================================================================================================

internal int32 GenerateAsteroid(GameState *game_state,
                                GameOffscreenBuffer *buffer,
                                int32 phase_index)
{
    // Find a free slot to generate the asteroid in.
    int32 asteroid_slot_index = -1;
    for (int i = 0; i < ARRAY_COUNT(game_state->asteroids); ++i)
    {
        if (game_state->asteroids[i].is_active == false)
        {
            asteroid_slot_index = i;
            break;
        }
    }

    if (asteroid_slot_index < 0)
    {
        return -1;
    }

    game_state->asteroids[asteroid_slot_index] = {};
    Asteroid *asteroid = &game_state->asteroids[asteroid_slot_index];

    // Position.
    int32 rand_x = RandomInt32InRangeLCG(&game_state->random, 0, buffer->width);
    int32 rand_y = RandomInt32InRangeLCG(&game_state->random, 0, buffer->height);
    asteroid->position.x = (float32)rand_x;
    asteroid->position.y = (float32)rand_y;

    // Adjust if too close to player.
    Vector2 difference = asteroid->position - game_state->player.position;
    float32 sqr_distance = SqrMagnitude(difference);
    float32 min_distance = game_state->asteroid_player_min_spawn_distance;
    if (sqr_distance <= (min_distance * min_distance))
    {
        Vector2 offset_diff = Normalize(difference);
        offset_diff.x *= min_distance;
        offset_diff.y *= min_distance;
        asteroid->position = asteroid->position + offset_diff;
    }

    // Forward direction.
    int32 rand_direction_angle = RandomInt32InRangeLCG(&game_state->random, 0, 360);
    float32 direction_radians = (float32)DEG2RAD * (float32)rand_direction_angle;
    asteroid->forward.x = Cos(direction_radians);
    asteroid->forward.y = Sin(direction_radians);

    // Phase size bounds.
    ASSERT(phase_index < 3 && phase_index >= 0); // Should never be above two for as long as we only have three phases.
    asteroid->phase_index = phase_index;
    int32 lower_size_bound = game_state->asteroid_phases[phase_index];
    int32 upper_size_bound = game_state->asteroid_phases[phase_index + 1];

    // Points.
    for (int point = 0; point < MAX_ASTEROID_POINTS; ++point)
    {
        asteroid->points[point] = {};

        float32 pct = (float32)point / (float32)MAX_ASTEROID_POINTS;
        float32 point_radians_around_circle = TWO_PI_32 * pct;

        int32 rand_offset_for_point = RandomInt32InRangeLCG(&game_state->random,
                                                            lower_size_bound,
                                                            upper_size_bound);
        asteroid->points[point].x = Cos(point_radians_around_circle) * (float32)rand_offset_for_point;
        asteroid->points[point].y = Sin(point_radians_around_circle) * (float32)rand_offset_for_point;
    }

    // Speed.
    int32 rand_speed = RandomInt32InRangeLCG(&game_state->random, 25, 256 * (1 / (asteroid->phase_index + 1)));
    asteroid->speed = (float32)rand_speed;

    // Color.
    asteroid->color_r = 0.94f;
    asteroid->color_g = 0.94f;
    asteroid->color_b = 0.94f;

    asteroid->is_active = true;
    return asteroid_slot_index;
}

internal void BreakAsteroid(GameState *game_state, GameOffscreenBuffer *buffer, Asteroid *asteroid)
{
    Vector2 original_position = asteroid->position;

    if (asteroid->phase_index > 0)
    {
        int32 a_slot = GenerateAsteroid(game_state, buffer, asteroid->phase_index - 1);
        int32 b_slot = GenerateAsteroid(game_state, buffer, asteroid->phase_index - 1);

        // Reposition where the broken asteroid was located.
        game_state->asteroids[a_slot].position.x = original_position.x;
        game_state->asteroids[a_slot].position.y = original_position.y;
        game_state->asteroids[b_slot].position.x = original_position.x;
        game_state->asteroids[b_slot].position.y = original_position.y;
    }

    asteroid->is_active = false; // Setting this to false means it won't get drawn.
}

// =================================================================================================
// GAME UPDATE AND RENDER (GUAR)
// =================================================================================================

// Function Signature:
// GameUpdateAndRender(GameMemory *memory, GameTime *time, GameInput *input, GameOffscreenBuffer *buffer)
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    global_platform = memory->platform_api;

    ASSERT(sizeof(GameState) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    Player *player = &game_state->player;
    UFO *ufo = &game_state->ufo;
    Grid *grid = &game_state->grid;

    if (!memory->is_initialized)
    {
        ConstructGridPartition(grid, buffer);

        game_state->random = {};
        SeedRandomLCG(&game_state->random, std::time(NULL));

        game_state->font = LoadFont();

        game_state->num_lives_at_start = 3;

        // Bullet configuration.
        game_state->bullet_speed = 612.0f;
        game_state->bullet_lifespan_seconds = 2.5f;
        game_state->bullet_size = 8.0f;

        // Asteroid configuration.
        game_state->asteroid_player_min_spawn_distance = 80.0f;
        game_state->num_asteroids_at_start = 6;
        game_state->asteroid_phases[0] = 25;
        game_state->asteroid_phases[1] = 32;
        game_state->asteroid_phases[2] = 45;
        game_state->asteroid_phases[3] = 75;
        game_state->asteroid_phase_score_amounts[0] = 100;
        game_state->asteroid_phase_score_amounts[1] = 50;
        game_state->asteroid_phase_score_amounts[2] = 20;

        for (int i = 0; i < game_state->num_asteroids_at_start; ++i)
        {
            GenerateAsteroid(game_state, buffer, 2);
        }

        // UFO configuration.
        game_state->ufo_large_point_value = 200;
        game_state->ufo_small_point_value = 1000;
        game_state->ufo_spawn_time_min = 5.0f;
        game_state->ufo_spawn_time_max = 15.0f;
        game_state->ufo_direction_change_time_min = 0.5f;
        game_state->ufo_direction_change_time_max = 2.0f;
        ufo->speed = 128.0f;
        ufo->time_to_next_spawn = RandomFloat32InRangeLCG(&game_state->random,
                                                          game_state->ufo_spawn_time_min,
                                                          game_state->ufo_spawn_time_max);
        ufo->large_width = 44.0f;
        ufo->large_inner_width = 18.0f;
        ufo->large_top_width = 8.0f;
        ufo->large_section_height = 8.0f;
        ufo->small_width = 27.5f;
        ufo->small_inner_width = 11.25f;
        ufo->small_top_width = 5.0f;
        ufo->small_section_height = 5.0f;

        ufo->color_r = 0.94f;
        ufo->color_g = 0.94f;
        ufo->color_b = 0.94f;

        // Player configuration.
        player->position.x = (float32)buffer->width / 2.0f;
        player->position.y = (float32)buffer->height / 2.0f;
        player->forward.x = 0.0f;
        player->forward.y = 1.0f;
        player->right.x = 1.0f;
        player->right.y = 0.0f;

        player->rotation = (TWO_PI_32 / 4.0f) * 3.0f; // Face the player upwards.
        player->rotation_speed = 7.0f;

        player->maximum_velocity = 5.0f;
        player->thrust_factor = 128.0f;
        player->acceleration = 0.14f;
        player->speed_damping_factor = 0.98f;

        player->color_r = 1.0f;
        player->color_g = 1.0f;
        player->color_b = 0.0f;

        player->lives = game_state->num_lives_at_start;

        ComputePlayerLines(player);

        memory->is_initialized = true;
    }

    if (!memory->is_initialized)
    {
        return;
    }

    float32 delta_time = (float32)time->delta_time;

    // =============================================================================================
    // INPUT PROCESSING
    // =============================================================================================
    float32 move_input_x = 0.0f;
    float32 move_input_y = 0.0f;
    bool32 is_bullet_desired = false;
    bool32 is_hyperspace_desired = false;
    for (int controller_index = 0; controller_index < ARRAY_COUNT(input->controllers); ++controller_index)
    {
        GameControllerInput *controller = GetController(input, controller_index);
        if (!controller->is_analog)
        {
            if (controller->move_up.ended_down)
            {
                move_input_y = -1.0f;
            }
            if (controller->move_down.ended_down)
            {
                move_input_y = 0.0f;
                if (controller->move_down.half_transition_count > 0)
                {
                    is_hyperspace_desired = true;
                }
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

    // =============================================================================================
    // HYPERSPACE
    // =============================================================================================
    if (is_hyperspace_desired)
    {
        // Find a place on the screen that's unoccupied.
        int32 total_spaces = NUM_GRID_SPACES_H * NUM_GRID_SPACES_V;
        int32 rand_space_index = RandomInt32InRangeLCG(&game_state->random, 0, total_spaces - 1);

        // Keep walking up the indices from this point until we find a free space in the grid.
        while (grid->spaces[rand_space_index].num_asteroid_line_points > 0)
        {
            rand_space_index = (rand_space_index + 1) % total_spaces;
        }

        int32 col = rand_space_index % NUM_GRID_SPACES_H;
        int32 row = rand_space_index / NUM_GRID_SPACES_H;

        float32 x = (float32)col * grid->space_width;
        float32 y = (float32)row * grid->space_height;

        player->position.x = x;
        player->position.y = y;

        is_hyperspace_desired = false;
    }

    // =============================================================================================
    // PLAYER UPDATE
    // =============================================================================================

    // Calculate the player's position.
    move_input_x *= player->rotation_speed;

    player->rotation += move_input_x * delta_time;
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
    if (SqrMagnitude(player->velocity) > (player->maximum_velocity * player->maximum_velocity))
    {
        player->velocity = Normalize(player->velocity);
        player->velocity.x *= player->maximum_velocity;
        player->velocity.y *= player->maximum_velocity;
    }

    player->velocity.x *= player->speed_damping_factor;
    player->velocity.y *= player->speed_damping_factor;

    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    WrapFloat32PointAroundBuffer(buffer, &player->position.x, &player->position.y);

    // Update Player & Asteroid cached line positions.
    ComputePlayerLines(player);

    // =============================================================================================
    // Collision Testing
    // =============================================================================================

    // Clear the grid.
    for (int i = 0; i < ARRAY_COUNT(game_state->grid.spaces); ++i)
    {
        game_state->grid.spaces[i].num_asteroid_line_points = 0;
        game_state->grid.spaces[i].num_bullets = 0;
        game_state->grid.spaces[i].num_ufo_points = 0;
        game_state->grid.spaces[i].has_player = false;
    }

    // Update grid spaces with player positions.
    uint32 space_index = 0;
    for (int player_line_index = 0; player_line_index < ARRAY_COUNT(player->lines); ++player_line_index)
    {
        Line *line = &player->lines[player_line_index];
        space_index = GetGridPosition(buffer, &game_state->grid, line->a.x, line->a.y);
        game_state->grid.spaces[space_index].has_player = true;
        space_index = GetGridPosition(buffer, &game_state->grid, line->b.x, line->b.y);
        game_state->grid.spaces[space_index].has_player = true;
    }

    // Update grid spaces with asteroid positions.
    for (int asteroid_index = 0; asteroid_index < ARRAY_COUNT(game_state->asteroids); ++asteroid_index)
    {
        Asteroid *asteroid = &game_state->asteroids[asteroid_index];

        if (asteroid->is_active)
        {
            for (int asteroid_line_index = 0; asteroid_line_index < ARRAY_COUNT(asteroid->lines); ++asteroid_line_index)
            {
                Line *line = &asteroid->lines[asteroid_line_index];
                space_index = GetGridPosition(buffer, &game_state->grid, line->a.x, line->a.y);
                game_state->grid.spaces[space_index].num_asteroid_line_points++;
                space_index = GetGridPosition(buffer, &game_state->grid, line->b.x, line->b.y);
                game_state->grid.spaces[space_index].num_asteroid_line_points++;
            }
        }
    }

    // Update grid spaces with bullet positions.
    for (int bullet_index = 0; bullet_index < ARRAY_COUNT(game_state->bullets); ++bullet_index)
    {
        Bullet *bullet = &game_state->bullets[bullet_index];
        if (bullet->is_active)
        {
            space_index = GetGridPosition(buffer, &game_state->grid, bullet->position.x, bullet->position.y);
            game_state->grid.spaces[space_index].num_bullets++;
        }
    }

    // Update grid spaces with ufo positions.
    if (ufo->is_active)
    {
        for (int point_index = 0; point_index < ARRAY_COUNT(ufo->points); ++point_index)
        {
            space_index = GetGridPosition(buffer, &game_state->grid,
                                          ufo->points[point_index].x, ufo->points[point_index].y);
            game_state->grid.spaces[space_index].num_ufo_points++;
        }
    }

    // Iterate through the spaces and perform discrete collision testing ONLY if spaces within a
    // certain distance contain objects that are concerned with each other.
    int32 total_spaces = ARRAY_COUNT(game_state->grid.spaces);
    for (int i = 0; i < total_spaces; ++i)
    {
        GridSpace *space = &grid->spaces[i];

        if (space->has_player)
        {
            // TODO(mara): This can eventually be compressed into a function, but wait until we're
            // doing the querying logic for the actual adjacent asteroids.
            int num_close_asteroids = space->num_asteroid_line_points;
            num_close_asteroids += space->north->num_asteroid_line_points;
            num_close_asteroids += space->north->east->num_asteroid_line_points;
            num_close_asteroids += space->east->num_asteroid_line_points;
            num_close_asteroids += space->south->east->num_asteroid_line_points;
            num_close_asteroids += space->south->num_asteroid_line_points;
            num_close_asteroids += space->south->west->num_asteroid_line_points;
            num_close_asteroids += space->west->num_asteroid_line_points;
            num_close_asteroids += space->north->west->num_asteroid_line_points;

            if (num_close_asteroids > 0)
            {
                // Test Collision Player - Asteroid
                for (int player_line_index = 0; player_line_index < ARRAY_COUNT(player->lines); ++player_line_index)
                {
                    for (int asteroid_index = 0; asteroid_index < ARRAY_COUNT(game_state->asteroids); ++asteroid_index)
                    {
                        if (game_state->asteroids[asteroid_index].is_active)
                        {
                            Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                            for (int asteroid_line_index = 0; asteroid_line_index < ARRAY_COUNT(asteroid->lines); ++asteroid_line_index)
                            {
                                if (TestLineIntersection(player->lines[player_line_index].a, player->lines[player_line_index].b,
                                                         asteroid->lines[asteroid_line_index].a, asteroid->lines[asteroid_line_index].b))
                                {
                                    // Increment our score.
                                    game_state->score += game_state->asteroid_phase_score_amounts[asteroid->phase_index];
                                    // Decrement the life counter.
                                    game_state->player.lives--;

                                    BreakAsteroid(game_state, buffer, asteroid);

                                    player->position.x = (float32)buffer->width / 2.0f;
                                    player->position.y = (float32)buffer->height / 2.0f;

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (space->num_bullets > 0)
        {
            // TODO(mara): This can eventually be compressed into a function, but wait until we're
            // doing the querying logic for the actual adjacent asteroids.
            int num_close_asteroids = space->num_asteroid_line_points;
            num_close_asteroids += space->north->num_asteroid_line_points;
            num_close_asteroids += space->north->east->num_asteroid_line_points;
            num_close_asteroids += space->east->num_asteroid_line_points;
            num_close_asteroids += space->south->east->num_asteroid_line_points;
            num_close_asteroids += space->south->num_asteroid_line_points;
            num_close_asteroids += space->south->west->num_asteroid_line_points;
            num_close_asteroids += space->west->num_asteroid_line_points;
            num_close_asteroids += space->north->west->num_asteroid_line_points;

            if (num_close_asteroids > 0)
            {
                // Test Collision Asteroid - Bullet
                for (int bullet_index = 0; bullet_index < ARRAY_COUNT(game_state->bullets); ++bullet_index)
                {
                    if (game_state->bullets[bullet_index].is_active)
                    {
                        Bullet *bullet = &game_state->bullets[bullet_index];
                        for (int asteroid_index = 0; asteroid_index < ARRAY_COUNT(game_state->asteroids); ++asteroid_index)
                        {
                            if (game_state->asteroids[asteroid_index].is_active)
                            {
                                Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                                for (int asteroid_line_index = 0; asteroid_line_index < ARRAY_COUNT(asteroid->lines); ++asteroid_line_index)
                                {
                                    if (TestLineCircleIntersection(asteroid->lines[asteroid_line_index].a, asteroid->lines[asteroid_line_index].b,
                                                                   bullet->position, game_state->bullet_size))
                                    {
                                        // Increment our score.
                                        game_state->score += game_state->asteroid_phase_score_amounts[asteroid->phase_index];

                                        BreakAsteroid(game_state, buffer, asteroid);

                                        // Immediately kill the bullet.
                                        bullet->is_active = false;

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (space->num_ufo_points > 0)
        {
            int32 num_ufo_points = ARRAY_COUNT(ufo->points);

            // TODO(mara): This can eventually be compressed into a function, but wait until we're
            // doing the querying logic for the actual adjacent asteroids.
            int32 num_close_asteroids = space->num_asteroid_line_points;
            num_close_asteroids += space->north->num_asteroid_line_points;
            num_close_asteroids += space->north->east->num_asteroid_line_points;
            num_close_asteroids += space->east->num_asteroid_line_points;
            num_close_asteroids += space->south->east->num_asteroid_line_points;
            num_close_asteroids += space->south->num_asteroid_line_points;
            num_close_asteroids += space->south->west->num_asteroid_line_points;
            num_close_asteroids += space->west->num_asteroid_line_points;
            num_close_asteroids += space->north->west->num_asteroid_line_points;

            if (num_close_asteroids > 0)
            {
                // Test Collision UFO - Asteroid
                for (int point_index = 0; point_index < num_ufo_points; ++point_index)
                {
                    int32 next_point_index = (point_index + 1) % num_ufo_points;

                    if (ufo->is_active)
                    {
                        for (int asteroid_index = 0; asteroid_index < ARRAY_COUNT(game_state->asteroids); ++asteroid_index)
                        {
                            if (game_state->asteroids[asteroid_index].is_active)
                            {
                                Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                                for (int asteroid_line_index = 0; asteroid_line_index < ARRAY_COUNT(asteroid->lines); ++asteroid_line_index)
                                {
                                    if (TestLineIntersection(ufo->points[point_index], ufo->points[next_point_index],
                                                             asteroid->lines[asteroid_line_index].a,
                                                             asteroid->lines[asteroid_line_index].b))
                                    {
                                        BreakAsteroid(game_state, buffer, asteroid);

                                        ufo->is_active = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            int32 num_close_bullets = space->num_bullets;
            num_close_bullets += space->north->num_asteroid_line_points;
            num_close_bullets += space->north->east->num_asteroid_line_points;
            num_close_bullets += space->east->num_asteroid_line_points;
            num_close_bullets += space->south->east->num_asteroid_line_points;
            num_close_bullets += space->south->num_asteroid_line_points;
            num_close_bullets += space->south->west->num_asteroid_line_points;
            num_close_bullets += space->west->num_asteroid_line_points;
            num_close_bullets += space->north->west->num_asteroid_line_points;

            if (num_close_bullets > 0)
            {
                // Test Collision UFO - Bullet
                for (int point_index = 0; point_index < num_ufo_points; ++point_index)
                {
                    int32 next_point_index = (point_index + 1) % num_ufo_points;

                    if (ufo->is_active)
                    {
                        for (int bullet_index = 0; bullet_index < ARRAY_COUNT(game_state->bullets); ++bullet_index)
                        {
                            if (game_state->bullets[bullet_index].is_active)
                            {
                                Bullet *bullet = &game_state->bullets[bullet_index];
                                if (TestLineCircleIntersection(ufo->points[point_index],
                                                               ufo->points[next_point_index],
                                                               bullet->position, game_state->bullet_size))
                                {
                                    game_state->score += ufo->is_small ? game_state->ufo_small_point_value : game_state->ufo_large_point_value;

                                    ufo->is_active = false;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            int32 is_player_close = space->has_player;
            is_player_close |= space->north->has_player;
            is_player_close |= space->north->east->has_player;
            is_player_close |= space->east->has_player;
            is_player_close |= space->south->east->has_player;
            is_player_close |= space->south->has_player;
            is_player_close |= space->south->west->has_player;
            is_player_close |= space->west->has_player;
            is_player_close |= space->north->west->has_player;

            if (is_player_close)
            {
                for (int point_index = 0; point_index < num_ufo_points; ++point_index)
                {
                    int32 next_point_index = (point_index + 1) % num_ufo_points;

                    if (ufo->is_active)
                    {
                        for (int player_line_index = 0; player_line_index < ARRAY_COUNT(player->lines); ++player_line_index)
                        {
                            if (TestLineIntersection(ufo->points[point_index], ufo->points[next_point_index],
                                                     player->lines[player_line_index].a,
                                                     player->lines[player_line_index].b))
                            {
                                game_state->score += ufo->is_small ? game_state->ufo_small_point_value : game_state->ufo_large_point_value;
                                game_state->player.lives--;

                                player->position.x = (float32)buffer->width / 2.0f;
                                player->position.y = (float32)buffer->height / 2.0f;

                                ufo->is_active = false;
                                break;
                            }
                        }
                    }
                }

            }
        }

        // TODO(mara): Test Collision Player - Bullet?
    }

    // Clear the screen.
    DrawFilledRectangle(buffer, 0.0f, 0.0f, (float32)buffer->width, (float32)buffer->height, 0.06f, 0.18f, 0.17f);

#if 1
    // DEBUG: Draw grid spaces either filled or unfilled if objects are present.
    for (int i = 0; i < ARRAY_COUNT(grid->spaces); ++i)
    {
        int32 col = i % NUM_GRID_SPACES_H;
        int32 row = i / NUM_GRID_SPACES_H;

        float32 x = (float32)col * grid->space_width;
        float32 y = (float32)row * grid->space_height;

        if (grid->spaces[i].num_asteroid_line_points > 0)
        {
            DrawFilledRectangle(buffer,
                                x, y,
                                x + grid->space_width, y + grid->space_height,
                                1.0f, 0.0f, 0.0f);
        }
        else if (grid->spaces[i].num_bullets > 0)
        {
            DrawFilledRectangle(buffer,
                                x, y,
                                x + grid->space_width, y + grid->space_height,
                                1.0f, 0.0f, 1.0f);
        }
        else if (grid->spaces[i].has_player)
        {
            DrawFilledRectangle(buffer,
                                x, y,
                                x + grid->space_width, y + grid->space_height,
                                0.0f, 0.0f, 1.0f);
        }
        else if (grid->spaces[i].num_ufo_points > 0)
        {
            DrawFilledRectangle(buffer,
                                x, y,
                                x + grid->space_width, y + grid->space_height,
                                0.0f, 1.0f, 1.0f);
        }
        else
        {
            DrawUnfilledRectangle(buffer,
                                  x, y,
                                  x + grid->space_width, y + grid->space_height,
                                  1.0f, 0.0f, 0.0f);
        }
        WrapFloat32PointAroundBuffer(buffer, &x, &y);
    }
#endif

    // =============================================================================================
    // PLAYER DRAW
    // =============================================================================================

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

    // =============================================================================================
    // BULLET UPDATE & DRAW
    // =============================================================================================

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
            WrapFloat32PointAroundBuffer(buffer, &bullet->position.x, &bullet->position.y);

            DrawCircle(buffer,
                       bullet->position.x, bullet->position.y, game_state->bullet_size,
                       0.45f, 0.9f, 0.76f);
        }
        else if (is_bullet_desired)
        {
            // Create the bullet in this index instead if we need one.
            bullet->position.x = player->position.x + player->forward.x * 20.0f;
            bullet->position.y = player->position.y + player->forward.y * 20.0f;
            bullet->forward.x = player->forward.x;
            bullet->forward.y = player->forward.y;
            bullet->time_remaining = game_state->bullet_lifespan_seconds;
            bullet->is_active = true;
            is_bullet_desired = false;
        }
    }

    // =============================================================================================
    // ASTEROID UPDATE & DRAW
    // =============================================================================================

    // Calculate asteroid positions and draw them all in the same loop.
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        Asteroid *asteroid = &game_state->asteroids[i];
        if (asteroid->is_active)
        {
            asteroid->position.x += asteroid->forward.x * asteroid->speed * delta_time;
            asteroid->position.y += asteroid->forward.y * asteroid->speed * delta_time;
            WrapFloat32PointAroundBuffer(buffer, &asteroid->position.x, &asteroid->position.y);

            ComputeAsteroidLines(asteroid);

            // Draw the lines that comprise the asteroid.
            for (int line_index = 0; line_index < MAX_ASTEROID_POINTS; ++line_index)
            {
                DrawLine(buffer,
                         asteroid->lines[line_index].a.x, asteroid->lines[line_index].a.y,
                         asteroid->lines[line_index].b.x, asteroid->lines[line_index].b.y,
                         asteroid->color_r, asteroid->color_g, asteroid->color_b);
            }
        }
    }

    // =============================================================================================
    // UFO UPDATE & DRAW
    //
    // UFO Points are constructed in the following order:
    //
    //       2 o---o 3
    //        /     \
    //    1 o---------o 4
    //     /           \
    // 0 o-------o-------o 5
    //    \     pos     /
    //   7 o-----------o 6
    //
    // =============================================================================================
    if (ufo->is_active)
    {
        ufo->time_to_next_direction_change -= delta_time;
        if (ufo->time_to_next_direction_change <= 0.0f)
        {
            ufo->forward.y = RandomFloat32InRangeLCG(&game_state->random, -1.0f, 1.0f);
            ufo->forward = Normalize(ufo->forward);
            ufo->time_to_next_direction_change = RandomFloat32InRangeLCG(&game_state->random,
                                                                         game_state->ufo_direction_change_time_min,
                                                                         game_state->ufo_direction_change_time_max);
        }

        ufo->position.x += ufo->forward.x * ufo->speed * delta_time;
        ufo->position.y += ufo->forward.y * ufo->speed * delta_time;
        WrapFloat32PointAroundBuffer(buffer, &ufo->position.x, &ufo->position.y);

        ComputeUFOPoints(ufo);

        // Draw the exterior lines of the UFO.
        for (int i = 0; i < 8; ++i)
        {
            int32 next_index = (i + 1) % 8;

            DrawLine(buffer,
                     ufo->points[i].x, ufo->points[i].y,
                     ufo->points[next_index].x, ufo->points[next_index].y,
                     ufo->color_r, ufo->color_g, ufo->color_b);
        }

        // Now draw the interior lines.
        DrawLine(buffer,
                 ufo->points[0].x, ufo->points[0].y,
                 ufo->points[5].x, ufo->points[5].y,
                 ufo->color_r, ufo->color_g, ufo->color_b);
        DrawLine(buffer,
                 ufo->points[1].x, ufo->points[1].y,
                 ufo->points[4].x, ufo->points[4].y,
                 ufo->color_r, ufo->color_g, ufo->color_b);
    }
    else
    {
        // Countdown to the next spawn!
        ufo->time_to_next_spawn -= delta_time;
        if (ufo->time_to_next_spawn <= 0.0f)
        {
            bool32 coin_flip = RandomInt32InRangeLCG(&game_state->random, 0, 1);
            ufo->position.x = coin_flip ? 0.0f : buffer->width;
            ufo->position.y = RandomFloat32InRangeLCG(&game_state->random, 10.0f, buffer->height - 10.0f);
            ufo->forward.x = coin_flip ? 1.0f : -1.0f;
            ufo->forward.y = 0.0f;
            ufo->time_to_next_direction_change = RandomFloat32InRangeLCG(&game_state->random,
                                                                         game_state->ufo_direction_change_time_min,
                                                                         game_state->ufo_direction_change_time_max);

            // Determine whether this will be a small or large asteroid.
            if (game_state->score >= 30000)
            {
                // ufo will always be small.
                ufo->is_small = true;
            }
            else
            {
                float32 pct = RandomFloat32InRangeLCG(&game_state->random, 0.0f, 1.0f);
                ufo->is_small = pct > 0.85f; // Smaller is rarer.
            }

            ufo->time_to_next_spawn = RandomFloat32InRangeLCG(&game_state->random,
                                                              game_state->ufo_spawn_time_min,
                                                              game_state->ufo_spawn_time_max);

            ufo->is_active = true;
        }
    }

    // =============================================================================================
    // UI DRAWING
    // =============================================================================================

    char score_string[24];
    sprintf_s(score_string, "%ld", game_state->score);
    DrawString(buffer, &game_state->font,
               score_string, 24, 48.0f,
               300.0f, 25.0f, 24.0f,
               0.3f, 0.5f, 1.0f);

    DrawString(buffer, &game_state->font,
               "This game is brought to you by the Lachlan Mouse Brothers.", 256, 32.0f,
               16.0f, buffer->height - 32.0f, 16.0f,
               0.5f, 0.5f, 0.5f);
}
