#include "asteroids.h"

#include <ctime>
#include <stdio.h>

// =================================================================================================
// FONT
// =================================================================================================

internal FontData LoadFont()
{
    FontData font_data = {};
    font_data.ttf_file = global_platform.ReadEntireFile("fonts/MapleMono-Regular.ttf");

    font_data.stb_font_info = {};
    stbtt_InitFont(&font_data.stb_font_info, (uchar8 *)font_data.ttf_file.content,
                   stbtt_GetFontOffsetForIndex((uchar8 *)font_data.ttf_file.content, 0));

    stbtt_GetFontVMetrics(&font_data.stb_font_info,
                          &font_data.ascent,
                          &font_data.descent,
                          &font_data.line_gap);

    return font_data;
}

// =================================================================================================
// SOUND
// =================================================================================================

internal SoundData LoadSound(MemoryArena *sound_arena, char *filename)
{
    SoundData result = {};

    int32 error;
    result.ogg_stream = stb_vorbis_open_filename(filename, &error, 0);
    result.ogg_info = stb_vorbis_get_info(result.ogg_stream);

    uint32 num_samples = stb_vorbis_stream_length_in_samples(result.ogg_stream);
    result.sample_count = num_samples;
    result.channel_count = result.ogg_info.channels;

    result.full_buffer_size = num_samples * sizeof(int16) * result.channel_count;
    result.samples = (int16 *)PushSize(sound_arena, result.full_buffer_size);

    int32 num_samples_filled = stb_vorbis_get_samples_short_interleaved(result.ogg_stream,
                                                                        result.channel_count,
                                                                        result.samples,
                                                                        result.sample_count * result.channel_count);

    Assert(num_samples_filled == result.sample_count);

    return result;
}

internal SoundStream *PlaySound(GameState *game_state, SoundID sound_id, bool is_loop = false)
{
    if (!game_state->first_free_playing_sound)
    {
        game_state->first_free_playing_sound = PushStruct(&game_state->sound_arena, SoundStream);
        game_state->first_free_playing_sound->next = 0;
    }

    SoundStream *sound_stream = game_state->first_free_playing_sound;
    game_state->first_free_playing_sound = sound_stream->next;

    sound_stream->samples_played = 0;
    sound_stream->volume[0] = 1.0f;
    sound_stream->volume[1] = 1.0f;
    sound_stream->loaded_sound_id = sound_id;
    sound_stream->is_loop = is_loop;

    sound_stream->next = game_state->first_playing_sound;
    game_state->first_playing_sound = sound_stream;

    return sound_stream;
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
        Swap(float32, x0, y0);
        Swap(float32, x1, y1);
    }

    // Flip values around if the start is greater than the end.
    if (x0 > x1)
    {
        Swap(float32, x0, x1);
        Swap(float32, y0, y1);
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

internal void DrawPoints(GameOffscreenBuffer *buffer,
                         Vector2 *points, int32 array_length,
                         float32 r, float32 g, float32 b,
                         float32 x_offset = 0.0f, float32 y_offset = 0.0f)
{
    for (int i = 0; i < array_length; ++i)
    {
        int32 next_i = (i + 1) % array_length;
        DrawLine(buffer,
                 points[i].x      + x_offset, points[i].y      + y_offset,
                 points[next_i].x + x_offset, points[next_i].y + y_offset,
                 r, g, b);
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
                                 char letter, float32 scale,
                                 int32 x, int32 y,
                                 float32 r, float32 g, float32 b)
{
    int width, height, x_offset, y_offset;
    uchar8 *bitmap = stbtt_GetCodepointBitmap(&font->stb_font_info, 0, scale,
                                              letter, &width, &height, &x_offset, &y_offset);

    x += x_offset;
    y += y_offset;

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
                         float32 start_x, float32 start_y,
                         float32 r, float32 g, float32 b)
{
    float32 scale = stbtt_ScaleForPixelHeight(&font->stb_font_info, pixel_height);
    int32 ascent = RoundFloat32ToInt32((float32)font->ascent * scale);
    int32 descent = RoundFloat32ToInt32((float32)font->descent * scale);

    int32 x = RoundFloat32ToInt32(start_x);
    int32 y = RoundFloat32ToInt32(start_y);
    y += ascent;

    for (uint32 i = 0; i < str_length; ++i)
    {
        if (str[i] == '\0')
        {
            break;
        }
        DrawLetterFromFont(buffer, font,
                           str[i], scale,
                           x, y,
                           r, g, b);

        int32 advance_width, left_side_bearing;
        stbtt_GetCodepointHMetrics(&font->stb_font_info, str[i], &advance_width, &left_side_bearing);

        int32 kern = stbtt_GetCodepointKernAdvance(&font->stb_font_info, str[i], str[i + 1]);

        x += RoundFloat32ToInt32((float32)advance_width * scale);
        x += RoundFloat32ToInt32((float32)kern * scale);
    }
}

internal void DrawWavyString(GameOffscreenBuffer *buffer, FontData *font, GameTime *time,
                             char *str, uint32 str_length, float32 pixel_height,
                             float32 start_x, float32 start_y,
                             float32 wave_speed, float32 wave_amplitude,
                             float32 r, float32 g, float32 b)
{
    float32 scale = stbtt_ScaleForPixelHeight(&font->stb_font_info, pixel_height);
    int32 ascent = RoundFloat32ToInt32((float32)font->ascent * scale);
    int32 descent = RoundFloat32ToInt32((float32)font->descent * scale);

    int32 x = RoundFloat32ToInt32(start_x);

    for (uint32 i = 0; i < str_length; ++i)
    {
        if (str[i] == '\0')
        {
            break;
        }

        float32 sin_value = Sin((float32)time->total_time * wave_speed + (float32)x) * wave_amplitude;
        int32 y = RoundFloat32ToInt32(start_y + sin_value) + ascent;

        DrawLetterFromFont(buffer, font,
                           str[i], scale,
                           x, y,
                           r, g, b);

        int32 advance_width, left_side_bearing;
        stbtt_GetCodepointHMetrics(&font->stb_font_info, str[i], &advance_width, &left_side_bearing);

        int32 kern = stbtt_GetCodepointKernAdvance(&font->stb_font_info, str[i], str[i + 1]);

        x += RoundFloat32ToInt32((float32)advance_width * scale);
        x += RoundFloat32ToInt32((float32)kern * scale);
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
    for (int i = 0; i < ArrayCount(grid->spaces); ++i)
    {
        uint32 north_index = WrapIndex(i - NUM_GRID_SPACES_H, total_spaces);
        uint32 east_index = (i + 1) % NUM_GRID_SPACES_H;
        uint32 south_index = (i + NUM_GRID_SPACES_H) % total_spaces;
        uint32 west_index = WrapIndex(i - 1, NUM_GRID_SPACES_H);

        grid->spaces[i].north = &grid->spaces[north_index];
        grid->spaces[i].east = &grid->spaces[east_index];
        grid->spaces[i].south = &grid->spaces[south_index];
        grid->spaces[i].west = &grid->spaces[west_index];
    }
}

inline int32 GetGridPosition(GameOffscreenBuffer *buffer, Grid *grid, float32 x, float32 y)
{
    WrapFloat32PointAroundBuffer(buffer, &x, &y);

    int32 grid_x = FloorFloat32ToInt32(x / grid->space_width);
    int32 grid_y = FloorFloat32ToInt32(y / grid->space_height);
    return grid_x + (grid_y * NUM_GRID_SPACES_H);
}

internal int32 GetNearbyAsteroidCountFromGridSpace(GridSpace *space)
{
    int num_close_asteroids = space->num_asteroid_line_points;
    num_close_asteroids += space->north->num_asteroid_line_points;
    num_close_asteroids += space->north->east->num_asteroid_line_points;
    num_close_asteroids += space->east->num_asteroid_line_points;
    num_close_asteroids += space->south->east->num_asteroid_line_points;
    num_close_asteroids += space->south->num_asteroid_line_points;
    num_close_asteroids += space->south->west->num_asteroid_line_points;
    num_close_asteroids += space->west->num_asteroid_line_points;
    num_close_asteroids += space->north->west->num_asteroid_line_points;
    return num_close_asteroids;
}

internal int32 GetNearbyBulletCountFromGridSpace(GridSpace *space)
{
    int32 num_close_bullets = space->num_bullets;
    num_close_bullets += space->north->num_bullets;
    num_close_bullets += space->north->east->num_bullets;
    num_close_bullets += space->east->num_bullets;
    num_close_bullets += space->south->east->num_bullets;
    num_close_bullets += space->south->num_bullets;
    num_close_bullets += space->south->west->num_bullets;
    num_close_bullets += space->west->num_bullets;
    num_close_bullets += space->north->west->num_bullets;
    return num_close_bullets;
}

internal int32 GetNearbyUFOPointCountFromGridSpace(GridSpace *space)
{
    int32 num_close_ufo_points = space->num_ufo_points;
    num_close_ufo_points += space->north->num_ufo_points;
    num_close_ufo_points += space->north->east->num_ufo_points;
    num_close_ufo_points += space->east->num_ufo_points;
    num_close_ufo_points += space->south->east->num_ufo_points;
    num_close_ufo_points += space->south->num_ufo_points;
    num_close_ufo_points += space->south->west->num_ufo_points;
    num_close_ufo_points += space->west->num_ufo_points;
    num_close_ufo_points += space->north->west->num_ufo_points;
    return num_close_ufo_points;
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

internal void ComputePlayerPointsLocal(Player *player)
{
    player->points_local[0].x = player->forward.x * -20.0f;
    player->points_local[0].y = player->forward.y * -20.0f;

    player->points_local[1].x = player->right.x * 10.0f;
    player->points_local[1].y = player->right.y * 10.0f;
    player->points_local[1].x += player->forward.x * 10.0f;
    player->points_local[1].y += player->forward.y * 10.0f;

    player->points_local[2].x = player->points_local[1].x - player->right.x * 4.0f - player->forward.x * 5.0f;
    player->points_local[2].y = player->points_local[1].y - player->right.y * 4.0f - player->forward.y * 5.0f;

    player->points_local[4].x = player->right.x * -10.0f;
    player->points_local[4].y = player->right.y * -10.0f;
    player->points_local[4].x += player->forward.x * 10.0f;
    player->points_local[4].y += player->forward.y * 10.0f;

    player->points_local[3].x = player->points_local[4].x + player->right.x * 4.0f - player->forward.x * 5.0f;
    player->points_local[3].y = player->points_local[4].y + player->right.y * 4.0f - player->forward.y * 5.0f;
}

internal void ComputePlayerPointsGlobal(Player *player)
{
    /*

      0
      o
      /\
      /  \
      /    \
      /      \
      / 3    2 \
      / o------o \
      //         \ \
      4 o/           \o 1

    */

    player->points_global[0].x = player->position.x + player->forward.x * 20.0f;
    player->points_global[0].y = player->position.y + player->forward.y * 20.0f;

    player->points_global[1].x = player->position.x + player->right.x * 10.0f;
    player->points_global[1].y = player->position.y + player->right.y * 10.0f;
    player->points_global[1].x -= player->forward.x * 10.0f;
    player->points_global[1].y -= player->forward.y * 10.0f;

    player->points_global[2].x = player->points_global[1].x - player->right.x * 4.0f + player->forward.x * 5.0f;
    player->points_global[2].y = player->points_global[1].y - player->right.y * 4.0f + player->forward.y * 5.0f;

    player->points_global[4].x = player->position.x - player->right.x * 10.0f;
    player->points_global[4].y = player->position.y - player->right.y * 10.0f;
    player->points_global[4].x -= player->forward.x * 10.0f;
    player->points_global[4].y -= player->forward.y * 10.0f;

    player->points_global[3].x = player->points_global[4].x + player->right.x * 4.0f + player->forward.x * 5.0f;
    player->points_global[3].y = player->points_global[4].y + player->right.y * 4.0f + player->forward.y * 5.0f;
}

internal void ComputeAsteroidLines(Asteroid *asteroid)
{
    for (int point = 0; point < MAX_ASTEROID_POINTS; ++point)
    {
        asteroid->points_global[point] = asteroid->position + asteroid->points_local[point];
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
    for (int i = 0; i < ArrayCount(game_state->asteroids); ++i)
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
    // Phase size bounds.
    Assert(phase_index < 3 && phase_index >= 0); // Should never be above two for as long as we only have three phases.
    asteroid->phase_index = phase_index;
    int32 lower_size_bound = game_state->asteroid_phase_sizes[phase_index];
    int32 upper_size_bound = game_state->asteroid_phase_sizes[phase_index + 1];

    // Position.
    int32 rand_x = RandomInt32InRange(&game_state->random, 0, buffer->width);
    int32 rand_y = RandomInt32InRange(&game_state->random, 0, buffer->height);
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
    int32 rand_direction_angle = RandomInt32InRange(&game_state->random, 0, 360);
    float32 direction_radians = (float32)DEG2RAD * (float32)rand_direction_angle;
    asteroid->forward.x = Cos(direction_radians);
    asteroid->forward.y = Sin(direction_radians);

    // Points.
    for (int point = 0; point < MAX_ASTEROID_POINTS; ++point)
    {
        asteroid->points_local[point] = {};

        float32 pct = (float32)point / (float32)MAX_ASTEROID_POINTS;
        float32 point_radians_around_circle = TWO_PI_32 * pct;

        int32 rand_offset_for_point = RandomInt32InRange(&game_state->random,
                                                         lower_size_bound,
                                                         upper_size_bound);
        asteroid->points_local[point].x = Cos(point_radians_around_circle) * (float32)rand_offset_for_point;
        asteroid->points_local[point].y = Sin(point_radians_around_circle) * (float32)rand_offset_for_point;
    }

    // Speed.
    asteroid->speed = game_state->asteroid_phase_speeds[asteroid->phase_index];

    // Color.
    asteroid->color_r = 0.94f;
    asteroid->color_g = 0.94f;
    asteroid->color_b = 0.94f;

    game_state->num_active_asteroids++;
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

    game_state->num_active_asteroids--;
    asteroid->is_active = false; // Setting this to false means it won't get drawn.
}

internal void ResetAsteroidPhaseSpeeds(GameState *game_state)
{
    game_state->asteroid_phase_speeds[0] = 96.0f;
    game_state->asteroid_phase_speeds[1] = 64.0f;
    game_state->asteroid_phase_speeds[2] = 32.0f;

    for (int i = 0; i < ArrayCount(game_state->asteroids); ++i)
    {
        Asteroid *asteroid = &game_state->asteroids[i];
        asteroid->speed = game_state->asteroid_phase_speeds[asteroid->phase_index];
    }
}

// =================================================================================================
// PARTICLE SYSTEM
// =================================================================================================

internal void InitializeParticle(GameState *game_state,
                                 ParticleSystem *parent_system, Particle *particle,
                                 float32 pos_x, float32 pos_y)
{
    particle->position = { pos_x, pos_y };

    particle->forward.x = RandomFloat32InRange(&game_state->random, -1.0f, 1.0f);
    particle->forward.y = RandomFloat32InRange(&game_state->random, -1.0f, 1.0f);
    particle->forward = Normalize(particle->forward);

    particle->move_speed = RandomFloat32InRange(&game_state->random,
                                                parent_system->particle_move_speed_min,
                                                parent_system->particle_move_speed_max);

    particle->time_remaining = RandomFloat32InRange(&game_state->random,
                                                    parent_system->particle_lifetime_min,
                                                    parent_system->particle_lifetime_max);

    particle->is_active = true;
}

internal void EmitSplashParticles(GameState *game_state, float32 pos_x, float32 pos_y)
{
    // Find a particle system in the buffer we can use.
    int32 available_splash_system_index = 0;
    for (int i = 0; i < ArrayCount(game_state->particle_system_splash); ++i)
    {
        if (!game_state->particle_system_splash[i].is_emitting)
        {
            available_splash_system_index = i;
            break;
        }
    }

    ParticleSystem *splash = &game_state->particle_system_splash[available_splash_system_index];
    splash->position = { pos_x, pos_y };

    for (int i = 0; i < splash->num_particles; ++i)
    {
        InitializeParticle(game_state, splash, &splash->particles[i], pos_x, pos_y);
    }

    splash->system_timer = 0.0f;
    splash->is_emitting = true;
}

internal void EmitLineParticles(GameState *game_state, int32 num_points, Vector2 *points)
{
    ParticleSystem *lines = &game_state->particle_system_lines;
    lines->position = { points[0].x, points[0].y };
    lines->num_particles = num_points * 2;
    Assert(lines->num_particles < MAX_PARTICLES);

    for (int point_i = 0; point_i < num_points; ++point_i)
    {
        int next_point_i = (point_i + 1) % num_points;

        int part_i = point_i * 2;
        int next_part_i = (part_i + 1) % lines->num_particles;
        InitializeParticle(game_state, lines, &lines->particles[part_i], points[point_i].x, points[point_i].y);

        // Next particle in the line will share the same attributes as the first.
        Particle *first_particle = &lines->particles[part_i];
        Particle *next_particle = &lines->particles[next_part_i];
        next_particle->position = { points[next_point_i].x, points[next_point_i].y };
        next_particle->forward.x = first_particle->forward.x;
        next_particle->forward.y = first_particle->forward.y;
        next_particle->move_speed = first_particle->move_speed;
        next_particle->time_remaining = first_particle->time_remaining;
        next_particle->is_active = true;
    }

    lines->system_timer = 0.0f;
    lines->is_emitting = true;
}

// =================================================================================================
// PLAYER (compressed functions from previously-repeated code)
// =================================================================================================

internal void TeleportPlayerToSafeLocationOnGrid(GameState *game_state, Grid *grid, Player *player)
{
    // Find a place on the screen that's unoccupied.
    int32 total_spaces = NUM_GRID_SPACES_H * NUM_GRID_SPACES_V;
    int32 rand_space_index = RandomInt32InRange(&game_state->random, 0, total_spaces - 1);

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
}

internal void HandlePlayerDeath(GameState *game_state, Player *player)
{
    EmitLineParticles(game_state, ArrayCount(player->points_global), player->points_global);

    player->lives--;

    // NOTE(mara): Procedurally, the death timer takes precedence over the invuln timer, and the
    // latter won't start counting down until the death timer has reached 0.
    player->death_timer = DEATH_TIME;
    player->invuln_timer = INVULN_TIME;

    TeleportPlayerToSafeLocationOnGrid(game_state, &game_state->grid, player);
}

internal void EnterNewHighScore(GameState *game_state, char *name, int32 score)
{
    for (int i = 0; i < ArrayCount(game_state->high_scores); ++i)
    {
        if (score > game_state->high_scores[i].score)
        {
            HighScore *score_to_override = &game_state->high_scores[i];

            // Keep a temporary copy of the score we're overriding.
            HighScore oldhs = {};
            oldhs.score = score_to_override->score;
            sprintf_s(oldhs.name, "%s", score_to_override->name);

            // Override that high score in place.
            score_to_override->score = score;
            sprintf_s(score_to_override->name, "%s", name);

            // Check the score we replaced against the list of high scores recursively.
            int32 next_index = (i + 1) % MAX_HIGH_SCORES;
            if (oldhs.score > game_state->high_scores[next_index].score)
            {
                EnterNewHighScore(game_state, oldhs.name, oldhs.score);
            }

            break;
        }
    }
}

internal void ResetDynamicGameStateValues(GameState *game_state, GameOffscreenBuffer *buffer)
{
    game_state->phase = GAME_PHASE_ATTRACT_MODE;
    game_state->random = {};
    SeedRandom(&game_state->random, std::time(NULL));
    game_state->score = 0;
    game_state->level = 0;

    // Player
    Player *player = &game_state->player;
    player->position.x = (float32)buffer->width / 2.0f;
    player->position.y = (float32)buffer->height / 2.0f;
    player->forward.x = 0.0f;
    player->forward.y = 1.0f;
    player->right.x = 1.0f;
    player->right.y = 0.0f;

    player->rotation = (TWO_PI_32 / 4.0f) * 3.0f; // Face the player upwards.

    player->death_timer = 0.0f;
    player->invuln_timer = 0.0f;

    player->lives = game_state->num_lives_at_start;

    ComputePlayerPointsLocal(player);
    ComputePlayerPointsGlobal(player);

    // Asteroids
    for (int i = 0; i < ArrayCount(game_state->asteroids); ++i)
    {
        game_state->asteroids[i].is_active = false;
    }

    ResetAsteroidPhaseSpeeds(game_state);
    game_state->time_until_next_speed_increase = 8.0f;

    for (int i = 0; i < game_state->num_asteroids_at_start; ++i)
    {
        GenerateAsteroid(game_state, buffer, 2);
        ComputeAsteroidLines(&game_state->asteroids[i]);
    }

    // UFO
    game_state->ufo.time_to_next_spawn = RandomFloat32InRange(&game_state->random,
                                                               game_state->ufo_spawn_time_min,
                                                               game_state->ufo_spawn_time_max);
}

// =================================================================================================
// GAME UPDATE AND RENDER (GUAR)
// =================================================================================================

// Function Signature:
// GameUpdateAndRender(GameMemory *memory, GameTime *time, GameInput *input, GameOffscreenBuffer *buffer)
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    global_platform = memory->platform_api;

    Assert(sizeof(GameState) <= memory->permanent_storage_size);
    Assert(sizeof(TransientState) <= memory->transient_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    TransientState *transient_state = (TransientState *)memory->transient_storage;

    Player *player = &game_state->player;
    UFO *ufo = &game_state->ufo;
    Grid *grid = &game_state->grid;

    if (!memory->is_initialized)
    {
        InitializeArena(&transient_state->arena,
                        memory->permanent_storage_size - sizeof(TransientState),
                        (uint8 *)memory->transient_storage + sizeof(TransientState));

        InitializeArena(&game_state->sound_arena,
                        memory->permanent_storage_size - sizeof(GameState),
                        (uint8 *)memory->permanent_storage + sizeof(GameState));

        game_state->sounds[0] = LoadWAV(&game_state->sound_arena, "sounds/bangLarge.wav");
        game_state->sounds[1] = LoadWAV(&game_state->sound_arena, "sounds/bangMedium.wav");
        game_state->sounds[2] = LoadWAV(&game_state->sound_arena, "sounds/bangSmall.wav");
        game_state->sounds[3] = LoadWAV(&game_state->sound_arena, "sounds/beat1.wav");
        game_state->sounds[4] = LoadWAV(&game_state->sound_arena, "sounds/beat2.wav");
        game_state->sounds[5] = LoadWAV(&game_state->sound_arena, "sounds/extraShip.wav");
        game_state->sounds[6] = LoadWAV(&game_state->sound_arena, "sounds/fire.wav");
        game_state->sounds[7] = LoadWAV(&game_state->sound_arena, "sounds/saucerBig.wav");
        game_state->sounds[8] = LoadWAV(&game_state->sound_arena, "sounds/saucerSmall.wav");
        game_state->sounds[9] = LoadWAV(&game_state->sound_arena, "sounds/thrust.wav");
        game_state->sounds[10] = LoadWAV(&game_state->sound_arena, "sounds/song.wav");

        game_state->test_wav = LoadWAV(&game_state->sound_arena, "sounds/fire.wav");

        PlaySound(game_state, SOUND_SONG);

        ConstructGridPartition(grid, buffer);

        game_state->font = LoadFont();

        game_state->num_lives_at_start = 4;

        // Player configuration.
        player->rotation_speed = 6.0f;
        player->maximum_velocity = 4.5f;
        player->thrust_factor = 128.0f;
        player->acceleration = 0.14f;
        player->speed_damping_factor = 0.98f;
        player->color_r = 1.0f;
        player->color_g = 1.0f;
        player->color_b = 0.0f;

        // Bullet configuration.
        game_state->bullet_speed = 589.0f;
        game_state->bullet_size = 7.0f;
        game_state->bullet_lifespan_seconds = 2.5f;

        // Asteroid configuration.
        game_state->asteroid_player_min_spawn_distance = 148.0f;
        game_state->num_asteroids_at_start = 6;
        game_state->asteroid_phase_sizes[0] = 12;
        game_state->asteroid_phase_sizes[1] = 32;
        game_state->asteroid_phase_sizes[2] = 45;
        game_state->asteroid_phase_sizes[3] = 75;
        game_state->asteroid_phase_point_values[0] = 100;
        game_state->asteroid_phase_point_values[1] = 50;
        game_state->asteroid_phase_point_values[2] = 20;
        game_state->asteroid_speed_max = 256.0f;
        game_state->asteroid_speed_increase_scalar = 1.15f;

        // UFO configuration.
        game_state->ufo_large_point_value = 200;
        game_state->ufo_small_point_value = 1000;
        game_state->ufo_bullet_time_min = 0.75f;
        game_state->ufo_bullet_time_max = 2.25f;
        game_state->ufo_bullet_speed = 412.0f;
        game_state->ufo_bullet_size = 5.0f;
        game_state->ufo_bullet_lifespan_seconds = 1.75f;
        game_state->ufo_spawn_time_min = 5.0f;
        game_state->ufo_spawn_time_max = 15.0f;
        game_state->ufo_direction_change_time_min = 0.5f;
        game_state->ufo_direction_change_time_max = 2.0f;

        ufo->speed = 128.0f;
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

        ResetDynamicGameStateValues(game_state, buffer);

        // Particle system configuration.
        for (int i = 0; i < ArrayCount(game_state->particle_system_splash); ++i)
        {
            ParticleSystem *splash = &game_state->particle_system_splash[i];
            splash->particle_lifetime_min = 0.1f;
            splash->particle_lifetime_max = 0.3f;
            splash->particle_move_speed_min = 256.0f;
            splash->particle_move_speed_max = 412.0f;
            splash->num_particles = MAX_PARTICLES;
        }
        ParticleSystem *lines = &game_state->particle_system_lines;
        lines->particle_lifetime_min = DEATH_TIME - 1.0f;
        lines->particle_lifetime_max = DEATH_TIME;
        lines->particle_move_speed_min = 4.0f;
        lines->particle_move_speed_max = 24.0f;
        lines->num_particles = ArrayCount(player->points_local);

        // Load the high scores.
        ReadFileResult result = global_platform.ReadEntireFile("highscores.ahs");
        if (result.content_size > 0)
        {
            HighScore *scores = (HighScore *)result.content;

            int num_elements = result.content_size / sizeof(HighScore);
            for (int i = 0; i < num_elements; ++i)
            {
                HighScore *hs = &scores[i];
                game_state->high_scores[i].score = hs->score;
                sprintf_s(game_state->high_scores[i].name, NAME_ENTRY_MAX_LENGTH + 1, "%s", hs->name);
            }
        }

        char alphabet[37] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        sprintf_s(game_state->name_chars, "%s", alphabet);
        game_state->entered_name[0] = 'A';
        game_state->entered_name[1] = ' ';
        game_state->entered_name[2] = ' ';

        memory->is_initialized = true;
    }

    if (!memory->is_initialized)
    {
        return;
    }

    CheckArenaForLingeringTemporaryMemory(&transient_state->arena);

    float32 delta_time = (float32)time->delta_time;

    // =============================================================================================
    // INPUT PROCESSING
    // =============================================================================================
    float32 move_input_x = 0.0f;
    float32 move_input_y = 0.0f;
    bool32 is_bullet_desired = false;
    bool32 is_hyperspace_desired = false;
    for (int controller_index = 0; controller_index < ArrayCount(input->controllers); ++controller_index)
    {
        GameControllerInput *controller = GetController(input, controller_index);
        if (!controller->is_analog)
        {
            if (controller->move_up.ended_down)
            {
                move_input_y = -1.0f;
                if (controller->move_up.half_transition_count > 0)
                {
                    game_state->name_move_up_desired = game_state->phase == GAME_PHASE_NAME_ENTRY;
                }
            }
            if (controller->move_down.ended_down)
            {
                move_input_y = 0.0f;
                if (controller->move_down.half_transition_count > 0)
                {
                    is_hyperspace_desired = true;
                    game_state->name_move_down_desired = game_state->phase == GAME_PHASE_NAME_ENTRY;
                }
            }
            if (controller->move_left.ended_down)
            {
                move_input_x = -1.0f;
                if (controller->move_left.half_transition_count > 0)
                {
                    game_state->name_move_left_desired = game_state->phase == GAME_PHASE_NAME_ENTRY;
                }
            }
            if (controller->move_right.ended_down)
            {
                move_input_x = 1.0f;
                if (controller->move_right.half_transition_count > 0)
                {
                    game_state->name_move_right_desired = game_state->phase == GAME_PHASE_NAME_ENTRY;
                }
            }
            if (controller->action_down.ended_down &&
                controller->action_down.half_transition_count > 0)
            {
                is_bullet_desired = game_state->phase == GAME_PHASE_PLAY && player->death_timer <= 0.0f;
                game_state->name_completion_desired = game_state->phase == GAME_PHASE_NAME_ENTRY;
            }
        }
    }

    // =============================================================================================
    // HYPERSPACE
    // =============================================================================================
    if (is_hyperspace_desired)
    {
        if (game_state->phase == GAME_PHASE_PLAY)
        {
            TeleportPlayerToSafeLocationOnGrid(game_state, grid, player);
        }
        else if (game_state->phase == GAME_PHASE_ATTRACT_MODE)
        {
            game_state->phase = GAME_PHASE_PLAY;
            ResetAsteroidPhaseSpeeds(game_state);
        }

        is_hyperspace_desired = false;
    }

    // =============================================================================================
    // PLAYER UPDATE
    // =============================================================================================

    if (game_state->phase == GAME_PHASE_PLAY)
    {
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

        if (player->death_timer >= 0.0f)
        {
            player->death_timer -= delta_time;
        }
        else if (player->invuln_timer >= 0.0f)
        {
            player->invuln_timer -= delta_time;
        }

        ComputePlayerPointsGlobal(player);
    }

    // =============================================================================================
    // LEVEL & DIFFICULTY UPDATE
    // =============================================================================================

    if (game_state->num_active_asteroids <= 0)
    {
        game_state->level++;

        game_state->num_asteroids_at_start++;
        if (game_state->num_asteroids_at_start > MAX_ASTEROIDS)
        {
            game_state->num_asteroids_at_start = MAX_ASTEROIDS;
        }

        for (int i = 0; i < game_state->num_asteroids_at_start; ++i)
        {
            GenerateAsteroid(game_state, buffer, 2);
        }

        ResetAsteroidPhaseSpeeds(game_state);
    }

    if (game_state->asteroid_speed_increase_count < MAX_SPEED_INCREASES)
    {
        game_state->time_until_next_speed_increase -= delta_time;
        if (game_state->time_until_next_speed_increase <= 0.0f)
        {
            // Multiply by the scalar and clamp to the max speed.
            float32 s0 = game_state->asteroid_phase_speeds[0];
            float32 s1 = game_state->asteroid_phase_speeds[1];
            float32 s2 = game_state->asteroid_phase_speeds[2];
            s0 *= game_state->asteroid_speed_increase_scalar;
            s1 *= game_state->asteroid_speed_increase_scalar;
            s2 *= game_state->asteroid_speed_increase_scalar;
            s0 = s0 > game_state->asteroid_speed_max ? game_state->asteroid_speed_max : s0;
            s1 = s1 > game_state->asteroid_speed_max ? game_state->asteroid_speed_max : s1;
            s2 = s2 > game_state->asteroid_speed_max ? game_state->asteroid_speed_max : s2;

            game_state->asteroid_phase_speeds[0] = s0;
            game_state->asteroid_phase_speeds[1] = s1;
            game_state->asteroid_phase_speeds[2] = s2;

            for (int i = 0; i < ArrayCount(game_state->asteroids); ++i)
            {
                Asteroid *asteroid = &game_state->asteroids[i];
                asteroid->speed = game_state->asteroid_phase_speeds[asteroid->phase_index];
            }

            game_state->time_until_next_speed_increase = 8.0f;
            game_state->asteroid_speed_increase_count++;
        }
    }

    if (game_state->phase == GAME_PHASE_PLAY &&
        player->lives <= 0 &&
        player->death_timer <= 0.0f)
    {
        for (int i = 0; i < ArrayCount(game_state->asteroids); ++i)
        {
            game_state->asteroids[i].is_active = false;
        }

        ufo->is_active = false;

        // Check the score.
        bool32 should_go_to_name_entry = false;
        for (int i = 0; i < ArrayCount(game_state->high_scores); ++i)
        {
            if (game_state->score > game_state->high_scores[i].score)
            {
                should_go_to_name_entry = true;
                break;
            }
        }

        if (should_go_to_name_entry)
        {
            game_state->phase = GAME_PHASE_NAME_ENTRY;
        }
        else
        {
            ResetDynamicGameStateValues(game_state, buffer);
        }
    }

    // =============================================================================================
    // NAME ENTRY PHASE UPDATE
    // =============================================================================================

    if (game_state->phase == GAME_PHASE_NAME_ENTRY)
    {
        // Y-input travels up or down the list of allowed characters.
        if (game_state->name_move_up_desired)
        {
            game_state->name_move_up_desired = false;

            int32 char_index = game_state->char_indices[game_state->name_index];
            game_state->char_indices[game_state->name_index] = WrapIndex(char_index + 1, NAME_ENTRY_MAX_ALLOWED_CHARS);
            game_state->entered_name[game_state->name_index] = game_state->name_chars[game_state->char_indices[game_state->name_index]];
        }

        if (game_state->name_move_down_desired)
        {
            game_state->name_move_down_desired = false;

            int32 char_index = game_state->char_indices[game_state->name_index];
            game_state->char_indices[game_state->name_index] = WrapIndex(char_index - 1, NAME_ENTRY_MAX_ALLOWED_CHARS);
            game_state->entered_name[game_state->name_index] = game_state->name_chars[game_state->char_indices[game_state->name_index]];
        }

        // X-input travels the "cursor" between name entry indices.
        if (game_state->name_move_left_desired)
        {
            game_state->name_move_left_desired = false;
            game_state->name_index = WrapIndex(game_state->name_index - 1, NAME_ENTRY_MAX_LENGTH);
        }

        if (game_state->name_move_right_desired)
        {
            game_state->name_move_right_desired = false;
            game_state->name_index = WrapIndex(game_state->name_index + 1, NAME_ENTRY_MAX_LENGTH);
        }

        if (game_state->name_completion_desired)
        {
            game_state->name_completion_desired = false;

            // Verify that we've got a complete name.
            bool32 is_name_valid = true;
            int32 invalid_name_index = 0;
            for (int i = 0; i < ArrayCount(game_state->entered_name); ++i)
            {
                if (game_state->entered_name[i] == ' ')
                {
                    is_name_valid = false;
                    invalid_name_index = i;
                    break;
                }
            }

            if (is_name_valid)
            {
                EnterNewHighScore(game_state, game_state->entered_name, game_state->score);

                global_platform.WriteEntireFile("highscores.ahs", sizeof(HighScore) * MAX_HIGH_SCORES, game_state->high_scores);

                ResetDynamicGameStateValues(game_state, buffer);
            }
            else
            {
                game_state->name_index = invalid_name_index;
            }
        }
    }

    // =============================================================================================
    // COLLISION TESTING
    // =============================================================================================

    // Clear the grid.
    for (int i = 0; i < ArrayCount(game_state->grid.spaces); ++i)
    {
        game_state->grid.spaces[i].num_asteroid_line_points = 0;
        game_state->grid.spaces[i].num_bullets = 0;
        game_state->grid.spaces[i].num_ufo_points = 0;
        game_state->grid.spaces[i].has_player = false;
    }

    // Update grid spaces with player positions.
    uint32 space_index = 0;
    for (int point_index = 0; point_index < ArrayCount(player->points_global); ++point_index)
    {
        Vector2 *point = &player->points_global[point_index];
        space_index = GetGridPosition(buffer, &game_state->grid, point->x, point->y);
        game_state->grid.spaces[space_index].has_player = true;
    }

    // Update grid spaces with asteroid positions.
    for (int32 asteroid_index = 0; asteroid_index < ArrayCount(game_state->asteroids); ++asteroid_index)
    {
        Asteroid *asteroid = &game_state->asteroids[asteroid_index];

        if (asteroid->is_active)
        {
            for (int point_index = 0; point_index < MAX_ASTEROID_POINTS; ++point_index)
            {
                Vector2 *point = &asteroid->points_global[point_index];
                space_index = GetGridPosition(buffer, &game_state->grid, point->x, point->y);
                game_state->grid.spaces[space_index].num_asteroid_line_points++;
            }
        }
    }

    // Update grid spaces with bullet positions.
    for (int bullet_index = 0; bullet_index < ArrayCount(game_state->bullets); ++bullet_index)
    {
        Bullet *bullet = &game_state->bullets[bullet_index];
        if (bullet->is_active)
        {
            space_index = GetGridPosition(buffer, &game_state->grid, bullet->position.x, bullet->position.y);
            game_state->grid.spaces[space_index].num_bullets++;
        }
    }

    for (int bullet_index = 0; bullet_index < ArrayCount(game_state->ufo_bullets); ++bullet_index)
    {
        Bullet *bullet = &game_state->ufo_bullets[bullet_index];
        if (bullet->is_active)
        {
            space_index = GetGridPosition(buffer, &game_state->grid, bullet->position.x, bullet->position.y);
            game_state->grid.spaces[space_index].num_bullets++;
        }
    }

    // Update grid spaces with ufo positions.
    if (ufo->is_active)
    {
        for (int point_index = 0; point_index < ArrayCount(ufo->points); ++point_index)
        {
            space_index = GetGridPosition(buffer, &game_state->grid,
                                          ufo->points[point_index].x, ufo->points[point_index].y);
            game_state->grid.spaces[space_index].num_ufo_points++;
        }
    }

    // Iterate through the spaces and perform discrete collision testing ONLY if spaces within a
    // certain distance contain objects that are concerned with each other.
    if (game_state->phase == GAME_PHASE_PLAY)
    {
        int32 total_spaces = ArrayCount(game_state->grid.spaces);
        for (int i = 0; i < total_spaces; ++i)
        {
            GridSpace *space = &grid->spaces[i];

            if (space->has_player && player->invuln_timer <= 0.0f)
            {
                if (GetNearbyAsteroidCountFromGridSpace(space) > 0)
                {
                    // Test Collision Player - Asteroid
                    int32 total_player_points = ArrayCount(player->points_global);
                    for (int player_point_index = 0; player_point_index < total_player_points; ++player_point_index)
                    {
                        int next_player_point_index = (player_point_index + 1) % total_player_points;
                        for (int asteroid_index = 0; asteroid_index < ArrayCount(game_state->asteroids); ++asteroid_index)
                        {
                            if (game_state->asteroids[asteroid_index].is_active)
                            {
                                Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                                int32 total_asteroid_points = ArrayCount(asteroid->points_global);
                                for (int asteroid_point_index = 0; asteroid_point_index < total_asteroid_points; ++asteroid_point_index)
                                {
                                    int32 next_asteroid_point_index = (asteroid_point_index + 1) % total_asteroid_points;
                                    if (TestLineIntersection(player->points_global[player_point_index],
                                                             player->points_global[next_player_point_index],
                                                             asteroid->points_global[asteroid_point_index],
                                                             asteroid->points_global[next_asteroid_point_index]))
                                    {
                                        // Increment our score.
                                        game_state->score += game_state->asteroid_phase_point_values[asteroid->phase_index];

                                        BreakAsteroid(game_state, buffer, asteroid);

                                        EmitSplashParticles(game_state, player->position.x, player->position.y);
                                        HandlePlayerDeath(game_state, player);

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if (GetNearbyBulletCountFromGridSpace(space) > 0)
                {
                    // Test Collision Player - Bullet (from UFO).
                    int32 total_player_points = ArrayCount(player->points_global);
                    for (int player_point_index = 0; player_point_index < total_player_points; ++player_point_index)
                    {
                        int next_player_point_index = (player_point_index + 1) % total_player_points;
                        for (int bullet_index = 0; bullet_index < ArrayCount(game_state->ufo_bullets); ++bullet_index)
                        {
                            if (game_state->ufo_bullets[bullet_index].is_active && !game_state->ufo_bullets[bullet_index].is_friendly)
                            {
                                Bullet *bullet = &game_state->ufo_bullets[bullet_index];
                                if (TestLineCircleIntersection(player->points_global[player_point_index], player->points_global[next_player_point_index],
                                                               bullet->position, game_state->bullet_size))
                                {
                                    EmitSplashParticles(game_state, bullet->position.x, bullet->position.y);

                                    HandlePlayerDeath(game_state, player);

                                    bullet->is_active = false;

                                    break;
                                }
                            }
                        }
                    }
                }

                if (GetNearbyUFOPointCountFromGridSpace(space) > 0)
                {
                    int32 num_ufo_points = ArrayCount(ufo->points);
                    int32 total_player_points = ArrayCount(player->points_global);

                    // Test Collision Player - UFO.
                    for (int player_point_index = 0; player_point_index < total_player_points; ++player_point_index)
                    {
                        int next_player_point_index = (player_point_index + 1) % total_player_points;

                        for (int ufo_point_index = 0; ufo_point_index < num_ufo_points; ++ufo_point_index)
                        {
                            int32 next_ufo_point_index = (ufo_point_index + 1) % num_ufo_points;
                            if (TestLineIntersection(ufo->points[ufo_point_index], ufo->points[next_ufo_point_index],
                                                     player->points_global[player_point_index],
                                                     player->points_global[next_player_point_index]))
                            {
                                game_state->score += ufo->is_small ? game_state->ufo_small_point_value : game_state->ufo_large_point_value;

                                HandlePlayerDeath(game_state, player);

                                ufo->is_active = false;
                                break;
                            }
                        }
                    }
                }
            }

            if (space->num_bullets > 0)
            {
                if (GetNearbyAsteroidCountFromGridSpace(space) > 0)
                {
                    // Test Collision Asteroid - Bullet
                    for (int bullet_index = 0; bullet_index < ArrayCount(game_state->bullets); ++bullet_index)
                    {
                        if (game_state->bullets[bullet_index].is_active)
                        {
                            Bullet *bullet = &game_state->bullets[bullet_index];
                            for (int asteroid_index = 0; asteroid_index < ArrayCount(game_state->asteroids); ++asteroid_index)
                            {
                                if (game_state->asteroids[asteroid_index].is_active)
                                {
                                    Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                                    int32 total_asteroid_points = ArrayCount(asteroid->points_global);
                                    for (int asteroid_point_index = 0; asteroid_point_index < total_asteroid_points; ++asteroid_point_index)
                                    {
                                        int32 next_asteroid_point_index = (asteroid_point_index + 1) % total_asteroid_points;
                                        if (TestLineCircleIntersection(asteroid->points_global[asteroid_point_index],
                                                                       asteroid->points_global[next_asteroid_point_index],
                                                                       bullet->position, game_state->bullet_size))
                                        {
                                            game_state->score += game_state->asteroid_phase_point_values[asteroid->phase_index];

                                            EmitSplashParticles(game_state, bullet->position.x, bullet->position.y);

                                            BreakAsteroid(game_state, buffer, asteroid);

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
                int32 num_ufo_points = ArrayCount(ufo->points);

                if (GetNearbyAsteroidCountFromGridSpace(space) > 0)
                {
                    // Test Collision UFO - Asteroid
                    for (int point_index = 0; point_index < num_ufo_points; ++point_index)
                    {
                        int32 next_point_index = (point_index + 1) % num_ufo_points;

                        if (ufo->is_active)
                        {
                            for (int asteroid_index = 0; asteroid_index < ArrayCount(game_state->asteroids); ++asteroid_index)
                            {
                                if (game_state->asteroids[asteroid_index].is_active)
                                {
                                    Asteroid *asteroid = &game_state->asteroids[asteroid_index];
                                    int32 total_asteroid_points = ArrayCount(asteroid->points_global);
                                    for (int asteroid_point_index = 0; asteroid_point_index < total_asteroid_points; ++asteroid_point_index)
                                    {
                                        int32 next_asteroid_point_index = (asteroid_point_index + 1) % total_asteroid_points;
                                        if (TestLineIntersection(ufo->points[point_index],
                                                                 ufo->points[next_point_index],
                                                                 asteroid->points_global[asteroid_point_index],
                                                                 asteroid->points_global[next_asteroid_point_index]))
                                        {
                                            BreakAsteroid(game_state, buffer, asteroid);

                                            EmitSplashParticles(game_state, ufo->position.x, ufo->position.y);

                                            ufo->is_active = false;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (GetNearbyBulletCountFromGridSpace(space) > 0)
                {
                    // Test Collision UFO - Bullet
                    for (int point_index = 0; point_index < num_ufo_points; ++point_index)
                    {
                        int32 next_point_index = (point_index + 1) % num_ufo_points;

                        if (ufo->is_active)
                        {
                            for (int bullet_index = 0; bullet_index < ArrayCount(game_state->bullets); ++bullet_index)
                            {
                                if (game_state->bullets[bullet_index].is_active)
                                {
                                    Bullet *bullet = &game_state->bullets[bullet_index];
                                    if (TestLineCircleIntersection(ufo->points[point_index],
                                                                   ufo->points[next_point_index],
                                                                   bullet->position, game_state->bullet_size))
                                    {
                                        game_state->score += ufo->is_small ? game_state->ufo_small_point_value : game_state->ufo_large_point_value;

                                        EmitSplashParticles(game_state, bullet->position.x, bullet->position.y);

                                        bullet->is_active = false;
                                        ufo->is_active = false;

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Clear the screen.
    DrawFilledRectangle(buffer, 0.0f, 0.0f, (float32)buffer->width, (float32)buffer->height, 0.06f, 0.18f, 0.17f);

    // Just for fun ;)
    DrawWavyString(buffer, &game_state->font, time,
                   "This game is brought to you by the Lachlan Mouse Brothers.", 128, 32.0f,
                   0.0f, (float32)buffer->height - 32.0f,
                   6.0f, 2.0f,
                   0.16f, 0.28f, 0.27f);

#if 0
    // DEBUG: Draw grid spaces either filled or unfilled if objects are present.
    for (int i = 0; i < ArrayCount(grid->spaces); ++i)
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

    if (game_state->phase == GAME_PHASE_PLAY && player->death_timer <= 0)
    {
        if (player->invuln_timer <= 0.0f)
        {
            DrawPoints(buffer,
                       player->points_global, ArrayCount(player->points_global),
                       player->color_r, player->color_g, player->color_b);
        }
        else
        {
            DrawPoints(buffer,
                       player->points_global, ArrayCount(player->points_global),
                       0.5f, 0.5f, 0.3f);
        }

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
            bullet->is_friendly = true;
            bullet->is_active = true;

            PlaySound(game_state, SOUND_FIRE);

            is_bullet_desired = false;
        }
    }

    // UFO Bullet Update & Draw
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        Bullet *bullet = &game_state->ufo_bullets[i];
        if (bullet->is_active)
        {
            bullet->time_remaining -= delta_time;
            if (bullet->time_remaining <= 0)
            {
                bullet->is_active = false;
                continue;
            }

            bullet->position.x += bullet->forward.x * game_state->ufo_bullet_speed * delta_time;
            bullet->position.y += bullet->forward.y * game_state->ufo_bullet_speed * delta_time;
            WrapFloat32PointAroundBuffer(buffer, &bullet->position.x, &bullet->position.y);

            DrawCircle(buffer,
                       bullet->position.x, bullet->position.y, game_state->ufo_bullet_size,
                       0.92f, 0.2f, 0.43f);
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

            for (int point_index = 0; point_index < MAX_ASTEROID_POINTS; ++point_index)
            {
                int next_point_index = (point_index + 1) % MAX_ASTEROID_POINTS;
                DrawLine(buffer,
                         asteroid->points_global[point_index].x, asteroid->points_global[point_index].y,
                         asteroid->points_global[next_point_index].x, asteroid->points_global[next_point_index].y,
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
            ufo->forward.y = RandomFloat32InRange(&game_state->random, -1.0f, 1.0f);
            ufo->forward = Normalize(ufo->forward);
            ufo->time_to_next_direction_change = RandomFloat32InRange(&game_state->random,
                                                                      game_state->ufo_direction_change_time_min,
                                                                      game_state->ufo_direction_change_time_max);
        }

        ufo->time_to_next_bullet -= delta_time;
        if (ufo->time_to_next_bullet <= 0.0f)
        {
            for (int i = 0; i < MAX_BULLETS; ++i)
            {
                Bullet *bullet = &game_state->ufo_bullets[i];
                if (!bullet->is_active)
                {
                    // Create the bullet in this index instead if we need one.

                    float32 random_dir_radians = RandomFloat32InRange(&game_state->random,
                                                                      0.0f, TWO_PI_32);
                    float32 x = Cos(random_dir_radians);
                    float32 y = Sin(random_dir_radians);
                    bullet->forward = { x, y };

                    // Uncomment this to have the UFO shoot at the player instead.
                    //bullet->forward = player->position - ufo->position;
                    //bullet->forward = Normalize(bullet->forward);

                    bullet->position.x = ufo->position.x + bullet->forward.x * 20.0f;
                    bullet->position.y = ufo->position.y + bullet->forward.y * 20.0f;
                    bullet->time_remaining = game_state->ufo_bullet_lifespan_seconds;
                    bullet->is_friendly = false;
                    bullet->is_active = true;
                    break;
                }
            }

            ufo->time_to_next_bullet = RandomFloat32InRange(&game_state->random,
                                                            game_state->ufo_bullet_time_min,
                                                            game_state->ufo_bullet_time_max);
        }

        ufo->position.x += ufo->forward.x * ufo->speed * delta_time;
        ufo->position.y += ufo->forward.y * ufo->speed * delta_time;
        WrapFloat32PointAroundBuffer(buffer, &ufo->position.x, &ufo->position.y);

        ComputeUFOPoints(ufo);

        // Check to see if the UFO reached the other side.
        if (ufo->started_on_left_side && ufo->position.x >= buffer->width - 12.0f)
        {
            ufo->is_active = false;
        }
        else if (!ufo->started_on_left_side && ufo->position.x <= 12.0f)
        {
            ufo->is_active = false;
        }

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
        if (game_state->phase == GAME_PHASE_PLAY)
        {
            // Countdown to the next spawn!
            ufo->time_to_next_spawn -= delta_time;
            if (ufo->time_to_next_spawn <= 0.0f)
            {
                ufo->started_on_left_side = RandomInt32InRange(&game_state->random, 0, 1);
                ufo->position.x = ufo->started_on_left_side ? 0.0f : buffer->width;
                ufo->position.y = RandomFloat32InRange(&game_state->random, 10.0f, buffer->height - 10.0f);
                ufo->forward.x = ufo->started_on_left_side ? 1.0f : -1.0f;
                ufo->forward.y = 0.0f;
                ufo->time_to_next_direction_change = RandomFloat32InRange(&game_state->random,
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
                    float32 pct = RandomFloat32InRange(&game_state->random, 0.0f, 1.0f);
                    ufo->is_small = pct > 0.85f; // Smaller is rarer.
                }

                ufo->time_to_next_spawn = RandomFloat32InRange(&game_state->random,
                                                               game_state->ufo_spawn_time_min,
                                                               game_state->ufo_spawn_time_max);
                ufo->time_to_next_bullet = RandomFloat32InRange(&game_state->random,
                                                                game_state->ufo_bullet_time_min,
                                                                game_state->ufo_bullet_time_max);
                ufo->is_active = true;
            }
        }
    }

    // =============================================================================================
    // PARTICLE SYSTEM UPDATE & DRAWING
    // =============================================================================================
    if (game_state->phase == GAME_PHASE_PLAY)
    {
        for (int i = 0; i < ArrayCount(game_state->particle_system_splash); ++i)
        {
            if (game_state->particle_system_splash[i].is_emitting)
            {
                ParticleSystem *splash = &game_state->particle_system_splash[i];

                splash->system_timer += delta_time;
                if (splash->system_timer >= splash->particle_lifetime_max)
                {
                    splash->is_emitting = false;
                    continue;
                }

                for (int particle_index = 0; particle_index < splash->num_particles; ++particle_index)
                {
                    if (splash->particles[particle_index].is_active)
                    {
                        Particle *particle = &splash->particles[particle_index];

                        particle->time_remaining -= delta_time;
                        if (particle->time_remaining <= 0.0f)
                        {
                            particle->is_active = false;
                        }

                        particle->position.x += particle->forward.x * particle->move_speed * delta_time;
                        particle->position.y += particle->forward.y * particle->move_speed * delta_time;

                        int32 x = RoundFloat32ToInt32(particle->position.x);
                        int32 y = RoundFloat32ToInt32(particle->position.y);
                        WrapInt32PointAroundBuffer(buffer, &x, &y);

                        DrawPixel(buffer,
                                  x, y,
                                  MakeColor(0.94f, 0.94f, 0.94f));
                    }
                }
            }
        }

        if (game_state->particle_system_lines.is_emitting)
        {
            ParticleSystem *lines = &game_state->particle_system_lines;

            lines->system_timer += delta_time;
            if (lines->system_timer >= lines->particle_lifetime_max)
            {
                lines->is_emitting = false;
            }

            // NOTE(mara): For the line particle system, we process and draw particles in batches of
            // two, so that we can make a call to DrawLine in one go. In the emit initialization step,
            // we multiply the number of input points by two in order to ensure that we have an even
            // number of particles, and to also ensure that the individual lines will be separated from
            // one another (we're not sharing points like we usually do in the player drawing logic).
            Assert(lines->num_particles % 2 == 0);
            for (int particle_index = 0; particle_index < lines->num_particles; particle_index += 2)
            {
                if (lines->particles[particle_index].is_active)
                {
                    Particle *particle = &lines->particles[particle_index];
                    Particle *next_particle = &lines->particles[(particle_index + 1)];

                    // We only really need to decrement the first particle's timer as the time
                    // remaining will be the same between both.
                    particle->time_remaining -= delta_time;
                    if (particle->time_remaining <= 0.0f)
                    {
                        particle->is_active = false;
                    }

                    particle->position.x += particle->forward.x * particle->move_speed * delta_time;
                    particle->position.y += particle->forward.y * particle->move_speed * delta_time;

                    next_particle->position.x += next_particle->forward.x * next_particle->move_speed * delta_time;
                    next_particle->position.y += next_particle->forward.y * next_particle->move_speed * delta_time;

                    DrawLine(buffer,
                             particle->position.x, particle->position.y,
                             next_particle->position.x, next_particle->position.y,
                             player->color_r, player->color_g, player->color_b);
                }
            }
        }
    }

    // =============================================================================================
    // UI DRAWING
    // =============================================================================================

    // Score display.
    char score_string[24];
    if (game_state->score < 10)
    {
        sprintf_s(score_string, "0%ld", game_state->score);
    }
    else
    {
        sprintf_s(score_string, "%ld", game_state->score);
    }
    DrawString(buffer, &game_state->font,
               score_string, 24, 48.0f,
               290.0f, 25.0f,
               0.3f, 0.5f, 1.0f);

    // Level display.
    char level_string[24];
    if (game_state->level < 10)
    {
        sprintf_s(level_string, "0%ld", game_state->level);
    }
    else
    {
        sprintf_s(level_string, "%ld", game_state->level);
    }
    DrawString(buffer, &game_state->font,
               level_string, 24, 28.0f,
               buffer->width / 2.0f, 25.0f,
               0.75f, 0.75f, 0.75f);

    if (game_state->phase == GAME_PHASE_ATTRACT_MODE)
    {
        DrawString(buffer, &game_state->font,
                   "PRESS HYPERSPACE (DOWN / S) TO PLAY", 128, 32.0f,
                   ((float32)buffer->width / 2.0f) - 256.0f, (float32)buffer->height - 128.0f,
                   0.2f, 0.3f, 0.75f);
    }
    else if (game_state->phase == GAME_PHASE_PLAY)
    {
        // Lives display.
        float32 x_offset = 300.0f;
        for (int i = 0; i < player->lives; ++i)
        {
            DrawPoints(buffer,
                       player->points_local, ArrayCount(player->points_local),
                       player->color_r, player->color_g, player->color_b,
                       x_offset, 85.0f);
            x_offset += 30.0f;
        }
    }

    if (player->death_timer >= 0.0f && player->lives <= 0)
    {
        DrawString(buffer, &game_state->font,
                   "Game...over...", 128, 48.0f,
                   ((float32)buffer->width / 2.0f) - 128.0f, (float32)buffer->height / 2.0f,
                   0.95f, 0.95f, 0.95f);
    }

    if (game_state->phase == GAME_PHASE_ATTRACT_MODE)
    {
        float32 x = ((float32)buffer->width / 2.0f) - 128.0f;
        float32 y = ((float32)buffer->height / 2.0f) - 200.0f;

        // Display the list of current high scores.
        for (int i = 0; i < MAX_HIGH_SCORES; ++i)
        {
            HighScore *hs = &game_state->high_scores[i];
            if (hs->score == 0)
            {
                DrawString(buffer, &game_state->font,
                           "AST | 00", 128, 48.0f,
                           x, y,
                           0.95f, 0.95f, 0.95f);
            }
            else
            {
                char hs_buffer[32];
                sprintf_s(hs_buffer, "%s | %i", hs->name, hs->score);

                DrawString(buffer, &game_state->font,
                           hs_buffer, 128, 48.0f,
                           x, y,
                           0.95f, 0.95f, 0.95f);
            }

            y += 40.0f;
        }
    }

    if (game_state->phase == GAME_PHASE_NAME_ENTRY)
    {
        float32 x = ((float32)buffer->width / 2.0f) - 128.0f;
        float32 y = ((float32)buffer->height / 2.0f) - 200.0f;

        // Display the list of current high scores.
        for (int i = 0; i < MAX_HIGH_SCORES; ++i)
        {
            HighScore *hs = &game_state->high_scores[i];
            if (hs->score == 0)
            {
                DrawString(buffer, &game_state->font,
                           "AST | 00", 128, 48.0f,
                           x, y,
                           0.95f, 0.95f, 0.95f);
            }
            else
            {
                char hs_buffer[32];
                sprintf_s(hs_buffer, "%s | %i", hs->name, hs->score);

                DrawString(buffer, &game_state->font,
                           hs_buffer, 128, 48.0f,
                           x, y,
                           0.95f, 0.95f, 0.95f);
            }

            y += 40.0f;
        }

        y += 32.0f;

        DrawString(buffer, &game_state->font,
                   "New high score! Please enter your name and press space.", 128, 36.0f,
                   x - 256.0f, y,
                   0.33f, 0.86f, 0.51f);

        y += 32.0f;

        char name_entry_buffer[6];
        for (int i = 0; i < ArrayCount(game_state->entered_name); ++i)
        {
            if (game_state->entered_name[i] == '\0')
            {
                break;
            }

            char entered_char = game_state->entered_name[i];
            if (game_state->entered_name[i] == 0)
            {
                entered_char = ' ';
            }

            name_entry_buffer[i * 2] = entered_char;
            name_entry_buffer[i * 2 + 1] = ' ';

            // Draw a little underline beneath the letter.
            float32 col_r = i == game_state->name_index ? 1.0f : 0.9f;
            float32 col_g = i == game_state->name_index ? 0.0f : 0.89f;
            float32 col_b = i == game_state->name_index ? 0.0f : 0.76f;
            DrawString(buffer, &game_state->font,
                       "_ ", 16, 48.0f,
                       x + (i * 44.0f), y + 16.0f,
                       col_r, col_g, col_b);
        }
        DrawString(buffer, &game_state->font,
                   name_entry_buffer, 16, 48.0f,
                   x, y,
                   0.9f, 0.89f, 0.76f);
    }

#if 0
    char time_string[128];
    sprintf_s(time_string, "%.2f seconds elapsed.", time->total_time);
    DrawString(buffer, &game_state->font,
               time_string, 128, 20.0f,
               4.0f, 4.0f,
               0.75f, 0.75f, 0.75f);
#endif
}

// void GameGetSoundSamples(GameMemory *memory, GameSoundOutputBuffer *buffer)
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    GameState *game_state = (GameState *)memory->permanent_storage;
    TransientState *transient_state = (TransientState *)memory->transient_storage;

    TemporaryMemory mixer_memory = BeginTemporaryMemory(&transient_state->arena);

    float32 *mix_buffer_ch_0 = PushArray(&transient_state->arena, buffer->sample_count, float32);
    float32 *mix_buffer_ch_1 = PushArray(&transient_state->arena, buffer->sample_count, float32);

    // Clear the mix buffer.
    {
        float32 *dest_0 = mix_buffer_ch_0;
        float32 *dest_1 = mix_buffer_ch_1;
        for (int sample_index = 0; sample_index < buffer->sample_count; ++sample_index)
        {
            *dest_0++ = 0.0f;
            *dest_1++ = 0.0f;
        }
    }

    // Sine wave (for testing).
    /*
    {
        float32 *dest_0 = mix_buffer_ch_0;
        float32 *dest_1 = mix_buffer_ch_1;
        float32 phase = 0.0;
        for (int sample_index = 0; sample_index < buffer->sample_count; ++sample_index)
        {
            phase += TWO_PI_32 / ((float32)buffer->samples_per_second / 400.0f);
            float32 sample = Sin(phase) * (float32)INT16_MAX * 0.1f;
            *dest_0++ += sample;
            *dest_1++ += sample;
        }
    }
    */

    // .wav file testing.
    /*
    {
        float32 *dest_0 = mix_buffer_ch_0;
        float32 *dest_1 = mix_buffer_ch_1;
        for (int32 sample_index = 0; sample_index < buffer->sample_count; ++sample_index)
        {
            uint32 wrapped_index = (game_state->test_wav_sample_index + sample_index) % game_state->test_wav.sample_count;
            int16 sample = game_state->test_wav.samples[0][wrapped_index];

            *dest_0++ += sample;
            *dest_1++ += sample;
        }

        game_state->test_wav_sample_index += buffer->sample_count;
    }
    */

    // Fill the mix buffer with this frame's sound sample values (additive).
    {
        for (SoundStream **playing_sound_ptr = &game_state->first_playing_sound; *playing_sound_ptr;)
        {
            SoundStream *playing_sound = *playing_sound_ptr;
            bool32 is_sound_finished = false;
            uint32 total_samples_to_mix = buffer->sample_count;

            float32 *dest_0 = mix_buffer_ch_0;
            float32 *dest_1 = mix_buffer_ch_1;
            while (total_samples_to_mix && !is_sound_finished)
            {
                WAVESoundData *loaded_sound = &game_state->sounds[playing_sound->loaded_sound_id];
                if (loaded_sound)
                {
                    float32 volume_0 = playing_sound->volume[0];
                    float32 volume_1 = playing_sound->volume[1];

                    Assert(playing_sound->samples_played >= 0);

                    uint32 samples_to_mix = total_samples_to_mix;
                    uint32 samples_remaining_in_sound = loaded_sound->sample_count - playing_sound->samples_played;
                    if (samples_to_mix > samples_remaining_in_sound)
                    {
                        samples_to_mix = samples_remaining_in_sound;
                    }

                    for (uint32 sample_index = playing_sound->samples_played;
                         sample_index < (playing_sound->samples_played + samples_to_mix);
                         ++sample_index)
                    {
                        float32 sample_value = loaded_sound->samples[0][sample_index];
                        *dest_0++ += volume_0 * sample_value;
                        *dest_1++ += volume_1 * sample_value;
                    }

                    playing_sound->samples_played += samples_to_mix;
                    total_samples_to_mix -= samples_to_mix;

                    if ((uint32)playing_sound->samples_played == loaded_sound->sample_count)
                    {
                        if (playing_sound->is_loop)
                        {
                            playing_sound->samples_played = 0;
                            playing_sound_ptr = &playing_sound->next;
                        }
                        else
                        {
                            is_sound_finished = true;
                        }
                    }
                    else
                    {
                        Assert(total_samples_to_mix == 0);
                    }
                }
                else
                {
                    break;
                }
            }

            if (is_sound_finished)
            {
                *playing_sound_ptr = playing_sound->next;
                playing_sound->next = game_state->first_free_playing_sound;
                game_state->first_free_playing_sound = playing_sound;
            }
            else
            {
                playing_sound_ptr = &playing_sound->next;
            }
        }
    }

    // Copy the mix buffer (src, float32) to the output buffer (dest, int16).
    {
        float32 *src_0 = mix_buffer_ch_0;
        float32 *src_1 = mix_buffer_ch_1;

        int16 *dest = buffer->samples;
        for (int sample_index = 0; sample_index < buffer->sample_count; ++sample_index)
        {
            // Convert float sample values to int16 by rounding.
            *dest++ = (int16)(*src_0++ + 0.5f);
            *dest++ = (int16)(*src_1++ + 0.5f);
        }
    }

    EndTemporaryMemory(mixer_memory);
}
