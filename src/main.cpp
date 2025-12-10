/*
 * This example code creates an SDL window and renderer, and then clears the
 * window to a different color every frame, so you'll effectively get a window
 * that's smoothly fading between colors.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */

#include <thread>
#include <vector>

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <functional>


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

static constexpr int THREAD_DISPATCH_X = 8;
static constexpr int THREAD_DISPATCH_Y = 8;

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
        g_framebuffer = nullptr;
    }
}

void RenderRegion(int dispatch_id, int x_start, int y_start, int width, int height, int fb_width) {
    int x_end = x_start + width;
    int y_end = y_start + height;

    for (int y = y_start; y < y_end; ++y) {
        Pixel* row_ptr = &g_framebuffer[y * fb_width];
        for (int x = x_start; x < x_end; ++x) {
            row_ptr[x] = WRITE_PIXEL(dispatch_id * 16, 0, 0, 255);
        }
    }
}
void DrawFramebuffer(int width, int height) {
    // 1. Calculate grid dimensions
    const int tile_w = THREAD_DISPATCH_X;
    const int tile_h = THREAD_DISPATCH_Y;

    // Integer division that rounds up to ensure we cover edges
    const int num_tiles_x = (width + tile_w - 1) / tile_w;
    const int num_tiles_y = (height + tile_h - 1) / tile_h;
    const int total_tiles = num_tiles_x * num_tiles_y;

    // 2. The Shared Counter
    // This atomic integer represents the next tile "job" waiting to be done.
    std::atomic_int next_tile_index{ 0 };

    // 3. The Worker Function
    // This lambda will run on every thread. It keeps grabbing work until none is left.
    auto worker = [&](int dispatch_id) {
        while (true) {
            // "fetch_add" atomically grabs the current value and increments it.
            // This is thread-safe and lock-free.
            int my_job_index = next_tile_index.fetch_add(1);

            // If the index is out of bounds, we are done.
            if (my_job_index >= total_tiles) {
                return;
            }

            // Convert linear index (0, 1, 2...) back to 2D Grid Coordinates
            int tile_x_index = my_job_index % num_tiles_x;
            int tile_y_index = my_job_index / num_tiles_x;

            // Convert Grid Coordinates to Pixel Coordinates
            int pixel_x = tile_x_index * tile_w;
            int pixel_y = tile_y_index * tile_h;

            // Handle edge cases (don't draw off the screen)
            int current_draw_w = std::min(tile_w, width - pixel_x);
            int current_draw_h = std::min(tile_h, height - pixel_y);

            // Draw
            RenderRegion(dispatch_id, pixel_x, pixel_y, current_draw_w, current_draw_h, width);
        }
    };

    // 4. Launch Threads
    // Only launch as many threads as you have cores.
    unsigned int core_count = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    // We run (core_count - 1) threads, and let the main thread help too
    // to avoid context switching overhead.
    for (unsigned int i = 1; i < core_count; ++i) {
        threads.emplace_back(std::bind(worker, i));
    }

    // Main thread also does work!
    worker(0);

    // 5. Cleanup
    // Wait for all helpers to finish.
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
}