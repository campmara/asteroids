#include <SDL3/SDL.h>

#include "asteroids.h"

#include "sdl3_asteroids.h"

global bool32 global_is_running;

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        // TODO(mara): setup performance counters.

        SDL_Window *window;
        SDL_Renderer *renderer;

        if (SDL_CreateWindowAndRenderer("Mara's Asteroids",
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_RESIZABLE,
                                        &window, &renderer))
        {
            // TODO(mara): Construct the exe filenames for the asteroids game dll here?

            // TODO(mara): Figure out how to calculate delta_time.

            // TODO(mara): Sound initialization.

            // TODO(mara): Memory initialization.

            // TODO(mara): Input initialization.

            // TODO(mara): Game code dll code loading.

            SDL_Texture *texture = SDL_CreateTexture(renderer,
                                                     SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                                     WINDOW_WIDTH, WINDOW_HEIGHT);

            global_is_running = true;

            while (global_is_running)
            {
                SDL_Event event;

                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_QUIT)
                    {
                        global_is_running = false;
                    }

                    // TODO(mara): Input processing.

                    // TODO(mara): Rendering.
                    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
                    SDL_RenderClear(renderer);

                    // SDL_RenderTexture(renderer, texture, NULL, NULL);
                    // SDL_RenderPresent(renderer);\

                    // TODO(mara): Sound Processing.

                    // TODO(mara): Figure out FPS and output that.
                }
            }
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create window and renderer: %s\n", SDL_GetError());
            return 3;
        }
    }
    else
    {
        return 3;
    }

    return 0;
}
