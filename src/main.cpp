#include <thread>
#include <vector>

#include <src/Graphics/Random.hpp>
#include <src/Renderer/PathTracer.hpp>

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <format>
#include <functional>
#include <mutex>


/* We will use this renderer to draw into this window every frame. */
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static SDL_Texture* g_screen_texture = nullptr;
static SDL_FRect g_dest_rect = {};
static Pixel* g_framebuffer = nullptr;
static float* g_time_buffer = nullptr;
static bool g_show_timing = false;
static devs_out_of_bounds::PathTracer* g_path_tracer = nullptr;

static std::vector<std::thread> g_worker_threads;
static std::atomic_int g_threads_active_count = { 0 };

// Add explicit padding to prevent False Sharing between these two hot variables
// (Aligns them to different 64-byte cache lines)
struct alignas(64) AlignedAtomic {
    std::atomic_int val;
};
static AlignedAtomic g_hot_index; // Replaces g_next_tile_index

static int g_frame_generation = 0;
static std::condition_variable g_cv_start_work;
static std::condition_variable g_cv_all_work_finished;
static std::mutex g_work_mutex;
static std::mutex g_completion_mutex;
static int g_total_tiles = 0;
static int g_curr_width = 1;
static int g_curr_height = 1;
static int g_num_tiles_x = 1;
static int g_num_tiles_y = 1;
static bool g_should_exit = false;

static constexpr int THREAD_DISPATCH_X = 64;
static constexpr int THREAD_DISPATCH_Y = 64;

static void RenderRegion(
    int dispatch_id, uint32_t& seed, int x_start, int y_start, int width, int height, int fb_width);

glm::vec3 GetHeatmapColor(float t) {
    // Ensure t is clamped for the gradient lookup
    t = std::clamp(t, 0.0f, 1.0f);

    // Define color stops: (Stop Position, Color)
    // 0.0: Black/Deep Blue (Cheapest)
    // 0.2: Blue
    // 0.4: Cyan
    // 0.6: Green (Average)
    // 0.8: Yellow
    // 1.0: Red/White (Most Expensive)

    // Simple 5-segment gradient
    const glm::vec3 c0(0.0f, 0.0f, 0.0f); // Black
    const glm::vec3 c1(0.0f, 0.0f, 1.0f); // Blue
    const glm::vec3 c2(0.0f, 1.0f, 1.0f); // Cyan
    const glm::vec3 c3(0.0f, 1.0f, 0.0f); // Green
    const glm::vec3 c4(1.0f, 1.0f, 0.0f); // Yellow
    const glm::vec3 c5(1.0f, 0.0f, 0.0f); // Red
    const glm::vec3 c6(1.0f, 1.0f, 1.0f); // White

    if (t < 0.16f)
        return glm::mix(c0, c1, t / 0.16f);
    if (t < 0.33f)
        return glm::mix(c1, c2, (t - 0.16f) / 0.17f);
    if (t < 0.50f)
        return glm::mix(c2, c3, (t - 0.33f) / 0.17f);
    if (t < 0.66f)
        return glm::mix(c3, c4, (t - 0.50f) / 0.16f);
    if (t < 0.83f)
        return glm::mix(c4, c5, (t - 0.66f) / 0.17f);
    return glm::mix(c5, c6, (t - 0.83f) / 0.17f);
}

static void InitThreads() {

// 4. Launch Threads
// Only launch as many threads as you have cores.
#ifdef NDEBUG
    unsigned int core_count = std::thread::hardware_concurrency();
#else
    // make it easier to debug!
    unsigned int core_count = 1;
#endif
    for (unsigned int i = 0; i < core_count; ++i) {
        g_worker_threads.emplace_back([dispatch_id = i]() {
            int my_local_gen = 0;
            uint32_t seed = dispatch_id;
            UniformDistribution::RandomStateAdvance(seed);
            while (true) {
                // --- Wait Phase ---
                {
                    std::unique_lock<std::mutex> lock(g_work_mutex);
                    g_cv_start_work.wait(lock, [&] {
                        // Wake up ONLY if the global generation is newer than mine
                        return (g_frame_generation > my_local_gen) || g_should_exit;
                    });
                }
                if (g_should_exit)
                    return;
                // Update my generation so I don't run this frame twice
                my_local_gen = g_frame_generation;

                // --- Work Phase ---
                while (true) {
                    int my_job_index = g_hot_index.val.fetch_add(1);

                    if (my_job_index >= g_total_tiles) {
                        break; // No more tiles
                    }

                    // Render Logic
                    int tile_x_index = my_job_index % g_num_tiles_x;
                    int tile_y_index = my_job_index / g_num_tiles_x;
                    int pixel_x = tile_x_index * THREAD_DISPATCH_X;
                    int pixel_y = tile_y_index * THREAD_DISPATCH_Y;
                    int current_draw_w = std::min(THREAD_DISPATCH_X, g_curr_width - pixel_x);
                    int current_draw_h = std::min(THREAD_DISPATCH_Y, g_curr_height - pixel_y);

                    RenderRegion(dispatch_id, seed, pixel_x, pixel_y, current_draw_w, current_draw_h, g_curr_width);
                }

                // --- Completion Phase ---
                // Decrement active count. fetch_sub returns the value BEFORE decrementing.
                int remaining_threads = g_threads_active_count.fetch_sub(1) - 1;

                if (remaining_threads == 0) {
                    // Last thread notifies Main
                    std::lock_guard<std::mutex> lock(g_completion_mutex);
                    g_cv_all_work_finished.notify_one();
                }
            }
        });
    }
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.g_renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    static int initial_w = 640;
    static int initial_h = 480;

    if (g_window = SDL_CreateWindow("jetwave", initial_w, initial_h, SDL_WINDOW_RESIZABLE); !g_window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (g_renderer = SDL_CreateRenderer(g_window, nullptr); !g_renderer) {
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(g_renderer, SDL_RENDERER_VSYNC_ADAPTIVE);
    SDL_SetRenderLogicalPresentation(g_renderer, initial_w, initial_h, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    g_screen_texture =
        SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, initial_w, initial_h);
    g_dest_rect = { 0, 0, static_cast<float>(initial_w), static_cast<float>(initial_h) };
    g_framebuffer = new Pixel[initial_w * initial_h];
    g_time_buffer = new float[initial_w * initial_h];
    SDL_Log("Succesfully initialised SDL");

    g_path_tracer = new devs_out_of_bounds::PathTracer();
    g_path_tracer->OnResize(initial_w, initial_h);
    InitThreads();
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
        if (g_time_buffer) {
            delete[] g_time_buffer;
        }
        g_time_buffer = new float[w * h];
        SDL_SetRenderLogicalPresentation(g_renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        g_path_tracer->OnResize(w, h);

        return SDL_APP_CONTINUE;
    }
    case SDL_EVENT_KEY_DOWN: {
        if (event->key.key == SDLK_1 && !event->key.repeat) {
            g_path_tracer->m_parameters.max_light_bounces = (g_path_tracer->m_parameters.max_light_bounces + 1) % 17;
            g_path_tracer->ResetAccumulator();
        }
        if (event->key.key == SDLK_2 && !event->key.repeat) {
            g_path_tracer->m_parameters.b_gt7_tonemapper = !g_path_tracer->m_parameters.b_gt7_tonemapper;
        }
        if (event->key.key == SDLK_3 && !event->key.repeat) {
            g_path_tracer->m_parameters.b_radiance_clamping = !g_path_tracer->m_parameters.b_radiance_clamping;
            g_path_tracer->ResetAccumulator();
        }
        if (event->key.key == SDLK_4 && !event->key.repeat) {
            g_show_timing = !g_show_timing;
        }
        if (event->key.key == SDLK_0 && !event->key.repeat) {
            g_path_tracer->m_parameters.b_accumulate = !g_path_tracer->m_parameters.b_accumulate;
        }
        return SDL_APP_CONTINUE;
    }
    default:
        return SDL_APP_CONTINUE; /* carry on with the program! */
    }
}

static void DrawFramebuffer(int width, int height);

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate) {
    static auto last_frame = std::chrono::high_resolution_clock::now();
    auto current_frame = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> duration = current_frame - last_frame;
    float frame_time = duration.count();
    last_frame = current_frame;

    g_path_tracer->OnUpdate(frame_time);

    SDL_RenderClear(g_renderer);

    DrawFramebuffer(g_dest_rect.w, g_dest_rect.h);

    if (g_show_timing) {
        for (int y = 0; y < g_curr_height; ++y) {
            for (int x = 0; x < g_curr_width; ++x) {
                float avgTimePerPixel = frame_time / static_cast<float>(g_curr_height * g_curr_width);
                float ratio = g_time_buffer[x + y * g_curr_width] / avgTimePerPixel;

                // Adjust 'MAX_RATIO' to define what is "too expensive".
                // e.g., 4.0 means "Red/White" happens at 4x the average cost.
                static constexpr float MAX_RATIO = 100.0f;
                float t_normalized = ratio / MAX_RATIO;

                float t_visual = glm::pow(t_normalized, 1.0f / 2.2f);

                glm::vec3 col = GetHeatmapColor(t_visual);
                g_framebuffer[x + y * g_curr_width] = DOOB_WRITE_PIXEL_F32(col.r, col.g, col.b, 1.0f);
            }
        }
    }
    SDL_UpdateTexture(g_screen_texture, NULL, g_framebuffer, g_dest_rect.w * sizeof(Pixel));
    SDL_RenderTexture(g_renderer, g_screen_texture, nullptr, &g_dest_rect);

    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderDebugTextFormat(g_renderer, 16.f, 16.f, "jetwave | FPS: %i, Frame Time: %.2f ms",
        static_cast<int>(1.0 / frame_time), 1000.0f * frame_time);
    SDL_RenderDebugTextFormat(g_renderer, 16.f, 26.f,
        "Max Light Bounces: %i | Tone Mapper: %s | Samples: %u | Clamping: %s",
        g_path_tracer->m_parameters.max_light_bounces, g_path_tracer->m_parameters.b_gt7_tonemapper ? "GT7" : "Exp",
        g_path_tracer->GetSamplesAccumulated(), g_path_tracer->m_parameters.b_radiance_clamping ? "Yes" : "No");
    float inv_shutter_speed, aperture, iso;
    g_path_tracer->m_parameters.assets.camera.GetSensor(aperture, inv_shutter_speed, iso);
    SDL_RenderDebugTextFormat(g_renderer, 16.f, 36.f,
        "Shutter Speed: 1 / %i | Aperture: %.1ff | ISO: %i | Exposure: %.3f",
        static_cast<int>(std::round(inv_shutter_speed)), aperture, static_cast<int>(std::round(iso)),
        expf(g_path_tracer->m_parameters.assets.camera.GetLogExposure()));

    SDL_RenderPresent(g_renderer);
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    {
        std::lock_guard<std::mutex> lock(g_work_mutex);
        g_should_exit = true;
    }
    g_cv_start_work.notify_all();

    for (auto& t : g_worker_threads) {
        if (t.joinable())
            t.join();
    }
    g_worker_threads.clear();

    delete g_path_tracer;
    g_path_tracer = nullptr;
    if (g_framebuffer) {
        delete[] g_framebuffer;
        g_framebuffer = nullptr;
    }
    if (g_time_buffer) {
        delete[] g_time_buffer;
        g_time_buffer = nullptr;
    }
}

void RenderRegion(int dispatch_id, uint32_t& seed, int x_start, int y_start, int width, int height, int fb_width) {
    int x_end = x_start + width;
    int y_end = y_start + height;

    for (int y = y_start; y < y_end; ++y) {
        Pixel* row_ptr = &g_framebuffer[y * fb_width];
        float* time_row_ptr = &g_time_buffer[y * fb_width];
        for (int x = x_start; x < x_end; ++x) {
            auto then = std::chrono::high_resolution_clock::now();
            row_ptr[x] = g_path_tracer->Evaluate(x, y, seed);
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = now - then;
            time_row_ptr[x] = duration.count();
        }
    }
}
void DrawFramebuffer(int width, int height) {
    g_curr_width = width;
    g_curr_height = height;

    int num_tiles_x = (width + THREAD_DISPATCH_X - 1) / THREAD_DISPATCH_X;
    int num_tiles_y = (height + THREAD_DISPATCH_Y - 1) / THREAD_DISPATCH_Y;
    g_num_tiles_x = num_tiles_x;
    g_total_tiles = num_tiles_x * num_tiles_y;

    // Reset Job Counter
    g_hot_index.val = 0;

    // Set active threads to the total count
    g_threads_active_count = g_worker_threads.size();

    // --- WAKE WORKERS ---
    {
        std::lock_guard<std::mutex> lock(g_work_mutex);
        g_frame_generation++; // Increment Ticket Number
    }
    g_cv_start_work.notify_all();

    // --- WAIT FOR COMPLETION ---
    {
        std::unique_lock<std::mutex> lock(g_completion_mutex);
        // Wait until all threads have checked out
        g_cv_all_work_finished.wait(lock, [] { return g_threads_active_count == 0; });
    }

    // Note: We no longer need to "Reset" a boolean flag here.
    // The generation counter naturally handles the state.
}