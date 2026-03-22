#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct
{
    float window_scale;
    float image_scale;
    float opacity;
    char filename[256];
} static args = {
    .window_scale = 1.0f,
    .image_scale = 1.0f,
    .opacity = .5f,
};

struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_FRect texture_info;

    bool is_dragging;
    float window_scale;
    float image_scale;
    float opacity;

    // keycode "I" for image
    // keycode "W" for window
    // keycode "A" for all
    uint8_t scale_mode;
    uint16_t tmp_scale;
} static state = {
    .window_scale = 1.0f,
    .image_scale = 1.0f,
    .opacity = .5f,
    .texture_info = {
        .x = 0,
        .y = 0,
        .w = 0,
        .h = 0,
    },
    .scale_mode = SDL_SCANCODE_UNKNOWN,
    .tmp_scale = 0,
};

static void parse_args(int argc, char *argv[])
{
    int opt, index;

    struct option long_options[] = {
        {"window_scale", required_argument, 0, 'w'},
        {"image_scale", required_argument, 0, 'i'},
        {"filename", required_argument, 0, 'f'},
        {0, 0, 0, 0},
    };

    while ((opt = getopt_long(argc, argv, "w:i:f:", long_options, &index)) != -1)
    {
        switch (opt)
        {
        case 'w':
        {
            args.window_scale = atof(optarg);
            break;
        }

        case 'i':
        {
            args.image_scale = atof(optarg);
            break;
        }

        case 'f':
        {
            strncpy(args.filename, optarg, sizeof(args.filename));
            break;
        }

        default:
        {
            fprintf(stderr, "Unknown argument: %d", optopt);
            exit(EXIT_FAILURE);
            break;
        }
        }
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    parse_args(argc, argv);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "60"))
    {
        SDL_Log("Couldn't set framerate: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(
            /*title*/           "A fucking moron",
            /*width*/           320,
            /*height*/          240,
            /*window flags*/    SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE,
            &state.window,
            &state.renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state.texture = IMG_LoadTexture(state.renderer, args.filename);
    if (state.texture == NULL)
    {
        SDL_Log("Couldn't load texture SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state.window_scale = args.window_scale;
    state.image_scale = args.image_scale;
    state.texture_info = (SDL_FRect){
        .x = 0,
        .y = 0,
        .w = state.texture->w,
        .h = state.texture->h,
    };

    SDL_SetWindowSize(state.window, state.texture->w * state.window_scale, state.texture->h * state.window_scale);
    SDL_SetWindowOpacity(state.window, state.opacity);
    SDL_SetRenderScale(state.renderer, state.image_scale, state.image_scale);
    SDL_SetTextureScaleMode(state.texture, SDL_SCALEMODE_NEAREST);

    return SDL_APP_CONTINUE;
}

static void restore_default_args_into_state()
{
    state.opacity = args.opacity;
    SDL_SetWindowOpacity(state.window, state.opacity);

    state.window_scale = args.window_scale;
    SDL_SetWindowSize(state.window, state.texture->w * args.window_scale, state.texture->h * args.window_scale);

    state.image_scale = args.image_scale;
    SDL_SetRenderScale(state.renderer, state.image_scale, state.image_scale);

    state.texture_info.x = 0;
    state.texture_info.y = 0;
}

static SDL_AppResult handle_key_down_event(SDL_Scancode scancode)
{
    SDL_Window *window = state.window;
    SDL_Texture *texture = state.texture;
    SDL_FRect *texture_info = &state.texture_info;

    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_0)
    {
        uint8_t number = ((scancode + 1) - SDL_SCANCODE_1) % 10;
        state.tmp_scale *= 10;
        state.tmp_scale += number;
    }

    switch (scancode)
    {
    case SDL_SCANCODE_ESCAPE:
        return SDL_APP_SUCCESS;
    case SDL_SCANCODE_R:
        restore_default_args_into_state();
        return SDL_APP_CONTINUE;
    case SDL_SCANCODE_RIGHT:
        texture_info->x += 1;
        break;
    case SDL_SCANCODE_LEFT:
        texture_info->x += -1;
        break;
    case SDL_SCANCODE_DOWN:
        texture_info->y += 1;
        break;
    case SDL_SCANCODE_UP:
        texture_info->y -= 1;
        break;
    }

    if (!state.scale_mode)
    {
        if (scancode == SDL_SCANCODE_W)
        {
            state.scale_mode = SDL_SCANCODE_W;
        }
        else if (scancode == SDL_SCANCODE_I)
        {
            state.scale_mode = SDL_SCANCODE_I;
        }
        state.tmp_scale = 0;
    }
    else
    {
        if (scancode == SDL_SCANCODE_RETURN)
        {
            if (state.scale_mode == SDL_SCANCODE_W)
            {
                state.window_scale = state.tmp_scale / 100.0f;
            }
            if (state.scale_mode == SDL_SCANCODE_I)
            {
                state.image_scale = state.tmp_scale / 100.0f;
            }

            SDL_SetWindowSize(
                window,
                texture->w * state.window_scale,
                texture->h * state.window_scale);
            SDL_SetRenderScale(
                state.renderer,
                state.image_scale,
                state.image_scale);

            state.tmp_scale = 0;
            state.scale_mode = SDL_SCANCODE_UNKNOWN;
        }
        if (scancode == SDL_SCANCODE_ESCAPE)
        {
            state.tmp_scale = 0;
            state.scale_mode = SDL_SCANCODE_UNKNOWN;
            return SDL_APP_CONTINUE;
        }
    }

    return SDL_APP_CONTINUE;
}

static SDL_AppResult handle_mouse_wheel_event(float y)
{
    state.opacity += y > .0f ? +.1f : -.1f;
    state.opacity = SDL_clamp(state.opacity, 0.0f, 1.0f);

    SDL_SetWindowOpacity(state.window, state.opacity);

    return SDL_APP_CONTINUE;
}

static SDL_AppResult handle_mouse_button_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        state.is_dragging = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        state.is_dragging = false;
        // round our drag and drop to be pixel perfect
        state.texture_info.x = round(state.texture_info.x);
        state.texture_info.y = round(state.texture_info.y);
        break;
    }

    case SDL_EVENT_MOUSE_MOTION:
    {
        if (!state.is_dragging)
        {
            return SDL_APP_CONTINUE;
        }
        // since our renderer is scaled we need to fix the event before usage
        SDL_ConvertEventToRenderCoordinates(state.renderer, event);
        state.texture_info.x += event->motion.xrel;
        state.texture_info.y += event->motion.yrel;
    }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        return handle_key_down_event(event->key.scancode);
    case SDL_EVENT_MOUSE_WHEEL:
        return handle_mouse_wheel_event(event->wheel.y);
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        return handle_mouse_button_event(event);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *)
{
    SDL_RenderClear(state.renderer);
    SDL_RenderTexture(state.renderer, state.texture, NULL, &state.texture_info);
    SDL_RenderPresent(state.renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}