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
    const char *path;
    char filename[256];
    float window_scale;
    float image_scale;
    float opacity;
} static args = {
    .filename = {0},
    .path = NULL,
    .window_scale = 1.0f,
    .image_scale = 1.0f,
    .opacity = .5f,
};

static void parse_args(int argc, char *argv[])
{
    args.path = argv[0];

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

struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_FRect texture_info;

    float window_scale;
    float image_scale;
    float opacity;
} static state = {
    .window = NULL,
    .renderer = NULL,
    .texture = NULL,
};

static void initialize_texture_from_args()
{
    if (state.texture)
    {
        SDL_DestroyTexture(state.texture);
    }

    state.texture = IMG_LoadTexture(state.renderer, args.filename);
    if (state.texture == NULL)
    {
        SDL_Log("Couldn't load texture SDL: %s", SDL_GetError());
        return;
    }

    state.window_scale = args.window_scale;
    state.image_scale = args.image_scale;
    state.opacity = args.opacity;
    state.texture_info = (SDL_FRect){
        .x = 0,
        .y = 0,
        .w = state.texture->w,
        .h = state.texture->h,
    };

    SDL_SetWindowSize(state.window, state.texture->w * state.window_scale, state.texture->h * state.window_scale);
    SDL_SetRenderScale(state.renderer, state.image_scale, state.image_scale);
    SDL_SetWindowOpacity(state.window, state.opacity);
    SDL_SetTextureScaleMode(state.texture, SDL_SCALEMODE_NEAREST);
}

static void show_open_file_dialog_callback(void *userdata, const char *const *filelist, int filter)
{
    if (filelist == NULL)
    {
        SDL_Log("Filelist is null: %s", SDL_GetError());
        return;
    }
    
    const char *const filename = filelist[0];
    printf("Selected file: %s", filename);

    strncpy(args.filename, filename, sizeof(args.filename));

    initialize_texture_from_args();
}

static void show_open_file_dialog()
{
    const SDL_DialogFileFilter filters[] = {
        {"All images", "png;jpg;jpeg"},
    };

    SDL_ShowOpenFileDialog(
        show_open_file_dialog_callback,
        /*userData*/ NULL,
        state.window,
        filters,
        /*nfilters*/ sizeof(filters) / sizeof(*filters),
        args.path,
        /*allowMany*/ false);
}

struct
{
    uint16_t scale;
    bool is_scaling;
} static state_scale = {
    .scale = 0,
    .is_scaling = false,
};

static void scale_by_keyboard(SDL_Event *event)
{
    if (event->type != SDL_EVENT_KEY_DOWN)
    {
        return;
    }

    SDL_Scancode scancode = event->key.scancode;

    switch (scancode)
    {
    case SDL_SCANCODE_S:
    {
        state_scale.scale = 0;
        state_scale.is_scaling = true;
        break;
    }
    case SDL_SCANCODE_0:
    case SDL_SCANCODE_1:
    case SDL_SCANCODE_2:
    case SDL_SCANCODE_3:
    case SDL_SCANCODE_4:
    case SDL_SCANCODE_5:
    case SDL_SCANCODE_6:
    case SDL_SCANCODE_7:
    case SDL_SCANCODE_8:
    case SDL_SCANCODE_9:
    {
        uint8_t number = ((scancode + 1) - SDL_SCANCODE_1) % 10;
        state_scale.scale *= 10;
        state_scale.scale += number;
        break;
    }
    case SDL_SCANCODE_RETURN:
    {
        if (!state_scale.is_scaling)
        {
            return;
        }

        if (state_scale.scale == 0)
        {
            state_scale.scale = 100;
        }

        state.window_scale = state_scale.scale / 100.0f;
        SDL_SetWindowSize(state.window, state.texture->w * state.window_scale, state.texture->h * state.window_scale);

        state.image_scale = state_scale.scale / 100.0f;
        SDL_SetRenderScale(state.renderer, state.image_scale, state.image_scale);

        state_scale.is_scaling = false;
        break;
    }
    }
}

static void move_image_by_arrows(SDL_Event *event)
{
    SDL_FRect *rect = &state.texture_info;

    switch (event->key.scancode)
    {
    case SDL_SCANCODE_RIGHT:
        rect->x += 1;
        break;
    case SDL_SCANCODE_LEFT:
        rect->x -= 1;
        break;
    case SDL_SCANCODE_DOWN:
        rect->y += 1;
        break;
    case SDL_SCANCODE_UP:
        rect->y -= 1;
        break;
    }
}

static void restore_default_state_by_pressing_r(SDL_Event *event)
{
    if (event->key.scancode != SDL_SCANCODE_R)
    {
        return;
    }

    state.opacity = args.opacity;
    SDL_SetWindowOpacity(state.window, state.opacity);

    state.window_scale = args.window_scale;
    SDL_SetWindowSize(state.window, state.texture->w * args.window_scale, state.texture->h * args.window_scale);

    state.image_scale = args.image_scale;
    SDL_SetRenderScale(state.renderer, state.image_scale, state.image_scale);

    state.texture_info.x = 0;
    state.texture_info.y = 0;
}

static void change_opacity_by_wheel(SDL_Event *event)
{
    if (event->type != SDL_EVENT_MOUSE_WHEEL)
    {
        return;
    }

    state.opacity += event->wheel.y > .0f ? +.1f : -.1f;
    state.opacity = SDL_clamp(state.opacity, 0.0f, 1.0f);

    SDL_SetWindowOpacity(state.window, state.opacity);
}

static void move_image_by_mouse_cursor(SDL_Event *event)
{
    if (event->button.button != SDL_BUTTON_LEFT)
    {
        return;
    }

    switch (event->type)
    {
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        // round our drag and drop to be pixel perfect
        state.texture_info.x = round(state.texture_info.x);
        state.texture_info.y = round(state.texture_info.y);
        break;
    }

    case SDL_EVENT_MOUSE_MOTION:
    {
        // Needed after you call SDL_SetRenderScale on that scale
        SDL_ConvertEventToRenderCoordinates(state.renderer, event);
        state.texture_info.x += event->motion.xrel;
        state.texture_info.y += event->motion.yrel;
    }
    }
}

static void trigger_file_picker_by_double_click(SDL_Event *event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.clicks == 2)
    {
        show_open_file_dialog();
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
            /*title*/ "A fucking moron",
            /*width*/ 320,
            /*height*/ 240,
            /*window flags*/ SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE,
            &state.window,
            &state.renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    initialize_texture_from_args();

    if (state.texture == NULL)
    {
        show_open_file_dialog();
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    move_image_by_arrows(event);
    move_image_by_mouse_cursor(event);
    scale_by_keyboard(event);
    restore_default_state_by_pressing_r(event);
    change_opacity_by_wheel(event);
    trigger_file_picker_by_double_click(event);

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