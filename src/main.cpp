/*
 * This example code creates an SDL window and renderer, and then clears the
 * window to a different color every frame, so you'll effectively get a window
 * that's smoothly fading between colors.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


using Pixel = uint32_t;
#define WRITE_PIXEL(r, g, b, a)                                                                                        \
    Pixel(((static_cast<uint32_t>(r) & 0xFFU) << 24) | ((static_cast<uint32_t>(g) & 0xFFU) << 16) |                    \
          ((static_cast<uint32_t>(b) & 0xFFU) << 8) | (static_cast<uint32_t>(a)) & 0xFFU)

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static SDL_Texture* g_screen_texture = nullptr;
static SDL_FRect g_dest_rect = {};
static Pixel* g_framebuffer = nullptr;


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.g_renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    static int initial_w = 640;
    static int initial_h = 480;

    if (!SDL_CreateWindowAndRenderer("X-Wave", initial_w, initial_h, SDL_WINDOW_RESIZABLE, &g_window, &g_renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(g_renderer, initial_w, initial_h, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    g_screen_texture =
        SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, initial_w, initial_h);
    g_dest_rect = { 0, 0, static_cast<float>(initial_w), static_cast<float>(initial_h) };
    g_framebuffer = new Pixel[initial_w * initial_h];
    SDL_Log("Succesfully initialised SDL");
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    case SDL_EVENT_WINDOW_RESIZED: {
        int w, h;
        if (!SDL_GetWindowSize(g_window, &w, &h)) {
            SDL_Log("Failed to retrieve window size: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
        if (g_screen_texture) {
            SDL_DestroyTexture(g_screen_texture);
        }
        g_screen_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        g_dest_rect = { 0, 0, static_cast<float>(w), static_cast<float>(h) };
        if (g_framebuffer) {
            delete[] g_framebuffer;
        }
        g_framebuffer = new Pixel[w * h];
        SDL_SetRenderLogicalPresentation(g_renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        return SDL_APP_CONTINUE;
    }
    default:
        return SDL_APP_CONTINUE; /* carry on with the program! */
    }
}

static void DrawFramebuffer(int width, int height);

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate) {
    SDL_RenderClear(g_renderer);

    DrawFramebuffer(g_dest_rect.w, g_dest_rect.h);

    SDL_UpdateTexture(g_screen_texture, NULL, g_framebuffer, g_dest_rect.w * sizeof(Pixel));
    SDL_RenderTexture(g_renderer, g_screen_texture, nullptr, &g_dest_rect);
    SDL_RenderPresent(g_renderer);

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    if (g_framebuffer) {
        delete[] g_framebuffer;
    }
}

void DrawFramebuffer(int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Pixel* pixel = &g_framebuffer[y * width + x];

            *pixel = WRITE_PIXEL(255, 128, 128, 255);
        }
    }
}
