#define _POSIX_C_SOURCE 200809L
#include <SDL.h>
#include <SDL_ttf.h>
#include <X11/Xlib.h>
#include <ctype.h>
#include "media.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W 800
#define H 600
#define LINES 32
#define LINE_LEN 240

/* Named story states keep the game flow readable and independent of the old executable's layout. */
typedef enum {
    INTRO, PHONE, SCANNING, VIDEO_PLAYBACK, STORY_CAPTION, DIAL_NUMBERS, BANK_PROMPT,
    DIAL_MENU, DIAL_DESTINATION, DIAL_DEPARTURE, DIAL_PASSENGER, DIAL_TICKET,
    LOGON, LOGON_REJECTED, GREETING, HOW_ARE_YOU, MISTAKES, PLAY, CHESS_REFUSAL, LATER,
    SIMULATION_ACCEPTED, SIMULATION_OVERVIEW, SIMULATION_WARMUP, SIMULATION_BATTLE, SIMULATION_TACTICAL,
    SIMULATION_MONTAGE, MODEM_CONNECT,
    SIDE, CITIES, GREETING3, IDENTIFICATION, PRIMARY_GOAL, WIN_STATEMENT,
    LOGON_AGAIN, STILL_PLAYING, REAL_OR_GAME, LOGON_FINAL, LIST_GAMES, GAME_SHELL,
    CHESS, TIC_TAC_TOE, TIC_MARK, TIC_MOVE, TIC_ANIMATION, TIC_AGAIN, ENDING_PRELUDE,
    FINAL_GREETING, ENDING,
    NORAD_PRELUDE, NORAD_CLIP_2, NORAD_CLIP_3, NORAD_CLIP_4,
    ENDING_CHESS, ENDING_MUSIC
} Stage;

typedef struct {
    const char *caption_asset;
    const char *background_asset;
} StoryCaption;

typedef enum {
    CAPTIONS_NONE,
    CAPTIONS_AFTER_SIMULATION,
    CAPTIONS_POST_HACK,
    CAPTIONS_FBI,
    CAPTIONS_FBI_TAIL,
    CAPTIONS_NORAD_12_23,
    CAPTIONS_NORAD_24_25,
    CAPTIONS_NORAD_26_30,
    CAPTIONS_NORAD_31_32
} CaptionProgram;

/* Everything owned by the interactive game loop lives in this structure. */
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    SDL_Texture *background;
    SDL_Texture *intro_banner;
    SDL_Texture *caption_texture;
    int caption_width;
    int caption_height;
    char lines[LINES][LINE_LEN];
    int line_count;
    char input[LINE_LEN];
    int ignore_next_menu_text;
    char panam_destination[LINE_LEN];
    char panam_departure[LINE_LEN];
    char panam_passenger[LINE_LEN];
    char terminal_last_input[LINE_LEN];
    char logon_info[16][LINE_LEN];
    int logon_info_count;
    Stage stage;
    int game_list_shown;
    int game_list_waiting;
    uint32_t game_list_deadline;
    int played_chess;
    int played_poker;
    int played_global_war;
    unsigned target_mask;
    int tic_index;
    char tic_mark;
    int tic_player_count;
    char tic_board[9];
    int tic_phase;
    uint32_t tic_deadline;
    int overview_line;
    int overview_visible[14];
    int overview_ready;
    uint32_t overview_deadline;
    int overview_prompt;
    int overview_prompt_visible[9];
    int overview_prompt_started;
    char side_response[LINE_LEN];
    char first_target[LINE_LEN];
    char second_target[LINE_LEN];
    int carrier_selection;
    int carrier_navigated;
    int bank_connecting;
    int bank_sound_mask;
    int bank_key_pending;
    uint32_t bank_connect_started;
    uint32_t stage_started;
    unsigned scan_digit_index;
    char scan_digits[25];
    uint32_t scan_started;
    int scan_sfm_played;
    int scan_caption_started;
    int scan_dynamic_started;
    int scan_dynamic_count;
    uint32_t scan_dynamic_deadline;
    int running;
    GameAudio audio;
    MediaPlayer video;
    char video_queue[4][8];
    int video_count;
    int video_index;
    Stage after_video_stage;
    char after_video_background[32];
    CaptionProgram after_video_captions;
    CaptionProgram caption_program;
    int caption_index;
    Stage after_caption_stage;
    char after_caption_background[32];
    uint32_t caption_deadline;
    int montage_index;
    uint32_t montage_deadline;
    int battle_index;
    int battle_warning_active;
    int battle_sound_channel;
    SDL_Texture *battle_warning_texture;
    uint32_t battle_warning_started;
    uint32_t battle_deadline;
    int tactical_frame;
    int tactical_exit_requested;
    int tactical_ready;
    int tactical_phase;
    char tactical_cells[37][101];
    int tactical_cursor_row;
    int tactical_cursor_column;
    int tactical_caption_visible;
    int tactical_compare_frame;
    int tactical_compare_phase;
    int tactical_rows[2];
    int tactical_columns[2];
    unsigned tactical_coordinates[4];
    unsigned tactical_pause_ms;
    int tactical_audio_channel;
    uint32_t tactical_deadline;
    char deferred_voice[32];
    uint32_t deferred_voice_deadline;
    int play_prompt_pending;
    uint32_t play_prompt_deadline;
    int modem_ring_started;
    uint32_t modem_ring_deadline;
    uint32_t ending_deadline;
    uint32_t intro_started;
} App;

/* Story helpers transition between named states and narrative cards. */
static void start_stage(App *a, Stage stage, const char *background);
static void start_video_sequence(App *a, const char *const *names, int count,
                                 Stage next_stage, const char *next_background,
                                 CaptionProgram after_captions);
static void start_simulation_montage(App *a);
static void advance_simulation_montage(App *a);
static void start_simulation_overview(App *a);
static void update_simulation_overview(App *a);
static void start_simulation_tactical(App *a);
static void draw_text(App *a, const char *text, int x, int y, SDL_Color color);
static void draw_block_cursor(App *a, int x, int y, SDL_Color color);
static void draw_tic_mark(App *a, int cell, char mark, SDL_Color color);
static void advance_tic_animation(App *a);

/* The original bitmap captions, in story order, with the matching supplied artwork. */
/* The original closes the simulation with seven full-screen cards. */
static const char *const simulation_montage[] = {
    "Warswoff", "Warswof2", "Warswof3", "Warswof4",
    "Warswof5", "Warswof6", "Warswof7"
};
static const char *const simulation_battle[] = {
    "Warmiwn1", "Warmiwn2", "Wararro1", "Wararro2", "Warusam1",
    "Warusam2", "Warselvg"
};
/* Both maps have fourteen rows; only the opening map types them character by character. */
static const char *const tactical_map_lines[] = {
    "                                                                  _    ",
    "  __________________                                           __/ \\",
    " /                  \\___        _                           __/     /  /\\   _",
    "{                       \\_   __/ |                       __/        `-'  \\_/ \\",
    "/                         \\_/   /                _    __/                      `~-~~._",
    "\\                              /                | \\__/                                \\",
    "|                             /                /                                    /\\/",
    "(                           _/                {                              _   __/  ",
    " |                      ___/                  |                           __/ \\ /   ",
    "  `~_                  /                      |                         _/    ) }   ",
    "     \\__     __  __   /                       (             ___   ,-._  \\    / /      ",
    "        \\   /  \\/  \\  \\                        \\_        _ /   \\_/    / /    \\/",
    "         \\_/        \\_/                           \\_   _/            (_/",
    "                                                    \\_/"
};
/* Text cells from the initial strategy map at the original 100-column size. */
static const struct {
    const char *text;
    int column, row;
    uint32_t interval;
    const char *voice;
} overview_prompts[] = {
    {"UNITED STATES", 10, 17, 30, NULL},
    {"SOVIET UNION", 59, 17, 30, NULL},
    {"WHICH SIDE DO YOU WANT?", 6, 20, 40, "Warwsdyw"},
    {"1.    UNITED STATES", 8, 22, 30, NULL},
    {"2.    SOVIET UNION", 8, 23, 30, NULL},
    {"PLEASE CHOOSE ONE:  ", 6, 25, 30, NULL},
    {"AWAITING FIRST STRIKE COMMAND", 6, 28, 30, NULL},
    {"PLEASE LIST PRIMARY TARGETS", 6, 32, 40, "Warplpt"},
    {"BY CITY AND/OR COUNTY NAME:", 6, 33, 30, NULL}
};
static const char *const tactical_trajectories[] = {
    "3-3478-A 234 765        4-2864-A 336 498        6-4524-A 984 846        6-5528 A 454 318",
    "       B 954 390               B 475 390               B 193 322               B 398 423",
    "       C 423 482               C 629 746               C 439 577               C 098 774",
    "       D 732 866               D 522 684               D 732 463               D 134 552",
    "       E 534 624               E 238 144               E 694 733               E 744 896",
    "9-6684-A 489 319        3-8524-A 673 542        3-3889-A 235 472        8-6422 A 475 572",
    "       B 245 107               B 341 221               B 469 854               B 213 633",
    "       C 657 584               C 328 854               C 238 783               C 636 234",
    "       D 576 324               D 892 218               D 373 437               D 389 238",
    "       E 438 587               E 574 758               E 485 288               E 567 742"
};
/* FUN_00405190 types all fourteen opening rows before asking for a side. */
static const StoryCaption post_simulation_captions[] = {
    {"Wartxt01", "Warpswde"}, {"Wartxt02", "Warpswde"}
};
static const StoryCaption post_hack_captions[] = {
    {"Wartxt03", NULL}, {"Wartxt4", NULL},
    {"Wartxt04", NULL}, {"Wartxt5", NULL}
};
static const StoryCaption fbi_captions[] = {
    {"Wartxt7", "Wargisld"}, {"Wartxt8", "Wargisld"}
};
static const StoryCaption fbi_tail_captions[] = {
    {"Wartxt9", "Wargisld"}, {"Wartxt10", "Wargisld"}, {"Wartxt11", "Wargisld"}
};
static const StoryCaption norad_12_23_captions[] = {
    {"Wartxt12", "Warpswde"}, {"Wartxt13", "Warpswde"}, {"Wartxt14", "Warpswde"},
    {"Wartxt15", "Warpswde"}, {"Wartxt16", "Warpswde"}, {"Wartxt17", "Warhelic"},
    {"Wartxt18", "Wardefc1"}, {"Wartxt19", "Wardefc1"}, {"Wartxt20", "War2400"},
    {"Wartxt21", "Wardefc1"}, {"Wartxt22", "Waronews"}, {"Wartxt23", "War2400"}
};
static const StoryCaption norad_24_25_captions[] = {
    {"Wartxt24", "Wargate"}, {"Wartxt25", "Wargate"}
};
static const StoryCaption norad_26_30_captions[] = {
    {"Wartxt26", "Wargate"}, {"Wartxt27", "Wargate"}, {"Wartxt28", "Wargate"},
    {"Wartxt29", "Wargate"}, {"Wartxt30", "Warselvg"}
};
static const StoryCaption norad_31_32_captions[] = {
    {"Wartxt31", "Warexpl"}, {"Wartxt32", "War2400"}
};
static const char *const carrier_numbers[] = {
    "(311) 399 2364", "(311) 399 3582", "(311) 437 8739",
    "(311) 437 1883", "(311) 437 2977", "(311) 767 7285",
    "(311) 767 3795", "(311) 936 3923", "(311) 936 1493"
};

/* The runtime's first RND call initializes Mersenne Twister with seed zero.
   The fallback seed inside the generator itself is bypassed by that initializer. */
static uint32_t game_rng_state[624];
static unsigned game_rng_index = 624;
static int game_rng_initialized;

static void game_rng_seed(void) {
    game_rng_state[0] = 0;
    for (unsigned i = 1; i < 624; ++i)
        game_rng_state[i] = game_rng_state[i - 1] * 0x19660du + 0x3c6ef35fu;
    game_rng_index = 624;
    game_rng_initialized = 1;
}

static uint32_t game_rng_u32(void) {
    if (!game_rng_initialized) game_rng_seed();
    if (game_rng_index >= 624) {
        unsigned i = 0;
        for (; i < 227; ++i) {
            uint32_t y = (game_rng_state[i] & 0x80000000u) |
                         (game_rng_state[i + 1] & 0x7fffffffu);
            game_rng_state[i] = game_rng_state[i + 397] ^ (y >> 1) ^
                                ((y & 1u) ? 0x9908b0dfu : 0u);
        }
        for (; i < 623; ++i) {
            uint32_t y = (game_rng_state[i] & 0x80000000u) |
                         (game_rng_state[i + 1] & 0x7fffffffu);
            game_rng_state[i] = game_rng_state[i - 227] ^ (y >> 1) ^
                                ((y & 1u) ? 0x9908b0dfu : 0u);
        }
        uint32_t y = (game_rng_state[623] & 0x80000000u) |
                     (game_rng_state[0] & 0x7fffffffu);
        game_rng_state[623] = game_rng_state[396] ^ (y >> 1) ^
                              ((y & 1u) ? 0x9908b0dfu : 0u);
        game_rng_index = 0;
    }
    uint32_t y = game_rng_state[game_rng_index++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680u;
    y ^= (y << 15) & 0xefc60000u;
    y ^= y >> 18;
    return y;
}

static double game_rng_unit(void) {
    return (double)game_rng_u32() / 4294967296.0;
}

/* FBI aftermath cards are advanced explicitly by the player in the original. */
static int caption_waits_for_input(CaptionProgram program) {
    return program == CAPTIONS_FBI_TAIL;
}

/* Match the original's caption-animation hold times for its NORAD montage.
   Ghidra shows mode 5 waiting four seconds, while modes 3 and 4 each animate
   for 1.2 seconds, pause four seconds, then animate for another 1.2 seconds.
   The last card also remains visible for the original two-second pause before
   the next video is opened. */
static uint32_t caption_hold_duration_ms(CaptionProgram program, int index) {
    if (program == CAPTIONS_AFTER_SIMULATION && (index == 0 || index == 1))
        return 1250u; /* FUN_0040e470 mode 0 waits 0x4e2 ms after Wartxt01/02. */
    if (program == CAPTIONS_NORAD_12_23) {
        if (index == 0) return 4000u;
        return index == 11 ? 8400u : 6400u;
    }
    return 4500u;
}

/* FUN_00412220 waits until ROUND(TIMER) plus the requested whole seconds. */
static uint32_t game_timer_deadline(uint32_t start_ticks, unsigned seconds) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return start_ticks + seconds * 1000u;
    uint32_t ticks = SDL_GetTicks();
    int32_t offset = (int32_t)(start_ticks - ticks);
    int64_t now_ms = (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000L;
    int64_t start_ms = now_ms + offset;
    int64_t rounded_start = ((start_ms + 500) / 1000) * 1000;
    int64_t target_ms = rounded_start + (int64_t)seconds * 1000;
    int64_t delay = target_ms - now_ms;
    return ticks + (uint32_t)(delay > 0 ? delay : 0);
}

static uint32_t caption_deadline(CaptionProgram program, int index) {
    uint32_t now = SDL_GetTicks();
    if (program == CAPTIONS_POST_HACK) {
        if (index == 0) {
            uint32_t second_pass = game_timer_deadline(now + 1200u, 4);
            return second_pass + 1200u;
        }
        if (index == 1 || index == 2) return game_timer_deadline(now, 4);
        return now + 2500u; /* Final card, one-second pause, then two ring-loop phases. */
    }
    return now + caption_hold_duration_ms(program, index);
}

/* Add a message to the terminal history, dropping the oldest line when full. */
static void push_line(App *a, const char *s) {
    if (a->line_count == LINES) {
        memmove(a->lines, a->lines + 1, sizeof(a->lines[0]) * (LINES - 1));
        a->line_count--;
    }
    snprintf(a->lines[a->line_count++], LINE_LEN, "%s", s);
}

/* The original Wargames.cfg records completion of its optional carrier scan.
   That persisted flag determines whether the next launch replays the intro. */
static int has_completed_carrier_scan(void) {
    FILE *file = fopen("../Wargames.cfg", "r");
    if (!file) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "\"nfsctwd\"")) {
            char *value = strchr(line, ',');
            char *start = value ? strchr(value + 1, '"') : NULL;
            char *end = start ? strchr(start + 1, '"') : NULL;
            found = start && end && end > start + 1;
            break;
        }
    }
    fclose(file);
    return found;
}

/* FUN_004012d0 records the scan date so later launches can skip the intro. */
static void record_carrier_scan(void) {
    char date[32];
    time_t now = time(NULL);
    struct tm local;
    if (!localtime_r(&now, &local) || !strftime(date, sizeof(date), "%m-%d-%Y", &local))
        return;
    const char *path = "../Wargames.cfg";
    const char *temporary_path = "../Wargames.cfg.tmp";
    FILE *input = fopen(path, "r");
    FILE *output = fopen(temporary_path, "w");
    if (!output) {
        if (input) fclose(input);
        return;
    }
    char line[512];
    int replaced = 0;
    while (input && fgets(line, sizeof(line), input)) {
        if (strstr(line, "\"nfsctwd\"")) {
            fprintf(output, "\"nfsctwd\",\"%s\"\n", date);
            replaced = 1;
        } else fputs(line, output);
    }
    if (input) fclose(input);
    if (!replaced) fprintf(output, "\"nfsctwd\",\"%s\"\n", date);
    if (fclose(output) == 0) (void)rename(temporary_path, path);
}

/* Replace the temporary help/profile output shown at the Joshua login prompt. */
static void set_logon_info(App *a, const char *const *lines, int count) {
    a->logon_info_count = count < 16 ? count : 16;
    for (int i = 0; i < a->logon_info_count; ++i)
        snprintf(a->logon_info[i], sizeof(a->logon_info[i]), "%s", lines[i]);
}

/* Keep the selected detected number visible while the player moves through the list. */
static void refresh_carrier_selection(App *a) {
    if (a->stage != DIAL_NUMBERS || a->line_count == 0) return;
    snprintf(a->lines[a->line_count - 1], LINE_LEN, "Selected: %s",
             carrier_numbers[a->carrier_selection - 7]);
}

/* Load one of the original 8-bit BMP backgrounds from the local asset folder. */
static void set_background(App *a, const char *name) {
    if (!name || !*name) {
        if (a->background) SDL_DestroyTexture(a->background);
        a->background = NULL;
        return;
    }
    char path[512];
    snprintf(path, sizeof(path), "../Gfx/%s.dat", name);
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface) return;
    SDL_Texture *next = SDL_CreateTextureFromSurface(a->renderer, surface);
    SDL_FreeSurface(surface);
    if (!next) return;
    if (a->background) SDL_DestroyTexture(a->background);
    a->background = next;
}

/* Some caption modes mask black; the console's native blits preserve the palette. */
static void load_story_caption(App *a, const char *name, int transparent) {
    char path[512];
    snprintf(path, sizeof(path), "../Gfx/%s.dat", name);
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface) return;
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return;
    /* The original 8-bit caption masks near-black palette shades as transparent. */
    if (transparent && SDL_LockSurface(rgba) == 0) {
        for (int y = 0; y < rgba->h; ++y) {
            uint32_t *pixels = (uint32_t *)((uint8_t *)rgba->pixels + y * rgba->pitch);
            for (int x = 0; x < rgba->w; ++x) {
                Uint8 red, green, blue, alpha;
                SDL_GetRGBA(pixels[x], rgba->format, &red, &green, &blue, &alpha);
                if (red <= 16 && green <= 16 && blue <= 16) alpha = 0;
                pixels[x] = SDL_MapRGBA(rgba->format, red, green, blue, alpha);
            }
        }
        SDL_UnlockSurface(rgba);
    }
    SDL_Texture *next = SDL_CreateTextureFromSurface(a->renderer, rgba);
    a->caption_width = rgba->w;
    a->caption_height = rgba->h;
    SDL_FreeSurface(rgba);
    if (!next) return;
    SDL_SetTextureBlendMode(next, transparent ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    if (a->caption_texture) SDL_DestroyTexture(a->caption_texture);
    a->caption_texture = next;
}

static void set_story_caption(App *a, const char *name) {
    load_story_caption(a, name, 1);
}

static void set_native_caption(App *a, const char *name) {
    load_story_caption(a, name, 0);
}

/* Reveal one bank-terminal string in the same character batches as the original. */
static size_t bank_reveal_count(size_t length, uint32_t elapsed, uint32_t offset, size_t step) {
    if (elapsed < offset) return 0;
    size_t count = (size_t)((elapsed - offset) / 30u + 1u) * step;
    return count < length ? count : length;
}

/* Draw a visible prefix and place the terminal block cursor after that prefix. */
static void draw_typed_text(App *a, const char *text, size_t count, int x, int y,
                            SDL_Color color, int cursor) {
    if (count > strlen(text)) count = strlen(text);
    char prefix[LINE_LEN];
    memcpy(prefix, text, count);
    prefix[count] = 0;
    if (count) draw_text(a, prefix, x, y, color);
    if (cursor) {
        int width = 0;
        if (count) TTF_SizeUTF8(a->font, prefix, &width, NULL);
        SDL_Rect block = {x + width, y + 2, 8, 16};
        SDL_SetRenderDrawColor(a->renderer, color.r, color.g, color.b, 255);
        SDL_RenderFillRect(a->renderer, &block);
    }
}

/* Start the original typewriter sample once for each bank-terminal line. */
static void update_bank_typewriter_audio(App *a) {
    static const uint32_t line_offsets[] = {0, 120, 180, 360, 420};
    uint32_t elapsed = SDL_GetTicks() - a->stage_started;
    for (size_t i = 0; i < sizeof(line_offsets) / sizeof(line_offsets[0]); ++i) {
        unsigned bit = 1u << i;
        if (!(a->bank_sound_mask & (int)bit) && elapsed >= line_offsets[i]) {
            media_sound(&a->audio, "Wartxtsd");
            a->bank_sound_mask |= (int)bit;
        }
    }
}

/* Return the ordered caption records for one chapter of the story. */
static const StoryCaption *caption_program_data(CaptionProgram program, int *count) {
    switch (program) {
    case CAPTIONS_AFTER_SIMULATION:
        *count = (int)(sizeof(post_simulation_captions) / sizeof(post_simulation_captions[0])); return post_simulation_captions;
    case CAPTIONS_POST_HACK:
        *count = (int)(sizeof(post_hack_captions) / sizeof(post_hack_captions[0])); return post_hack_captions;
    case CAPTIONS_FBI:
        *count = (int)(sizeof(fbi_captions) / sizeof(fbi_captions[0])); return fbi_captions;
    case CAPTIONS_FBI_TAIL:
        *count = (int)(sizeof(fbi_tail_captions) / sizeof(fbi_tail_captions[0])); return fbi_tail_captions;
    case CAPTIONS_NORAD_12_23:
        *count = (int)(sizeof(norad_12_23_captions) / sizeof(norad_12_23_captions[0])); return norad_12_23_captions;
    case CAPTIONS_NORAD_24_25:
        *count = (int)(sizeof(norad_24_25_captions) / sizeof(norad_24_25_captions[0])); return norad_24_25_captions;
    case CAPTIONS_NORAD_26_30:
        *count = (int)(sizeof(norad_26_30_captions) / sizeof(norad_26_30_captions[0])); return norad_26_30_captions;
    case CAPTIONS_NORAD_31_32:
        *count = (int)(sizeof(norad_31_32_captions) / sizeof(norad_31_32_captions[0])); return norad_31_32_captions;
    case CAPTIONS_NONE:
        *count = 0; return NULL;
    }
    *count = 0;
    return NULL;
}

/* Show the first caption in a chapter and remember the next story state. */
static void start_caption_sequence(App *a, CaptionProgram program, Stage next_stage, const char *background) {
    if (program == CAPTIONS_AFTER_SIMULATION && a->stage == CITIES) {
        a->caption_program = program;
        a->after_caption_stage = next_stage;
        snprintf(a->after_caption_background, sizeof(a->after_caption_background), "%s",
                 background ? background : "");
        a->stage = SIMULATION_WARMUP;
        a->caption_deadline = SDL_GetTicks() + 500u; /* Original pauses 500 ms before Wartxt01. */
        return;
    }
    int count = 0;
    const StoryCaption *items = caption_program_data(program, &count);
    if (!items || count == 0) { start_stage(a, next_stage, background); return; }
    media_stop_voice(&a->audio);
    if (a->caption_texture) SDL_DestroyTexture(a->caption_texture);
    a->caption_texture = NULL;
    a->caption_program = program;
    a->caption_index = 0;
    a->after_caption_stage = next_stage;
    snprintf(a->after_caption_background, sizeof(a->after_caption_background), "%s", background ? background : "");
    a->stage = STORY_CAPTION;
    if (program == CAPTIONS_POST_HACK) {
        a->modem_ring_started = 0;
        a->modem_ring_deadline = 0;
    }
    a->line_count = 0;
    a->input[0] = 0;
    if (program == CAPTIONS_AFTER_SIMULATION) set_background(a, NULL);
    else set_background(a, items[0].background_asset);
    if (program == CAPTIONS_AFTER_SIMULATION) set_native_caption(a, items[0].caption_asset);
    else set_story_caption(a, items[0].caption_asset);
    if (program == CAPTIONS_POST_HACK && a->caption_texture)
        SDL_SetTextureColorMod(a->caption_texture, 238, 238, 238);
    a->caption_deadline = caption_waits_for_input(program)
        ? 0 : caption_deadline(program, a->caption_index);
}

static void start_simulation_montage(App *a) {
    a->stage = SIMULATION_MONTAGE;
    a->montage_index = 0;
    a->line_count = 0;
    a->input[0] = 0;
    media_stop_voice(&a->audio);
    set_background(a, simulation_montage[0]);
    a->montage_deadline = SDL_GetTicks() + 500u;
}

static void advance_simulation_montage(App *a) {
    a->montage_index++;
    if (a->montage_index >= (int)(sizeof(simulation_montage) / sizeof(simulation_montage[0]))) {
        start_caption_sequence(a, CAPTIONS_POST_HACK, MODEM_CONNECT, NULL);
        return;
    }
    /* The original starts each sample after the preceding bitmap hold. */
    if (a->montage_index == 1) media_sound(&a->audio, "Warswoff");
    if (a->montage_index == 3) media_sound(&a->audio, "Warswof2");
    set_background(a, simulation_montage[a->montage_index]);
    uint32_t hold = (a->montage_index == 1 || a->montage_index == 6) ? 500u : 100u;
    a->montage_deadline = SDL_GetTicks() + hold;
}

/* The first map is fourteen typed rows, unlike the later thirteen-row map. */
static void start_simulation_overview(App *a) {
    a->stage = SIMULATION_OVERVIEW;
    a->overview_line = 0;
    memset(a->overview_visible, 0, sizeof(a->overview_visible));
    memset(a->overview_prompt_visible, 0, sizeof(a->overview_prompt_visible));
    a->overview_prompt = 0;
    a->overview_prompt_started = 0;
    a->overview_ready = 0;
    a->overview_deadline = SDL_GetTicks();
    a->target_mask = 0;
    a->side_response[0] = a->first_target[0] = a->second_target[0] = a->input[0] = 0;
    a->line_count = 0;
    set_background(a, NULL);
}

/* Non-space map glyphs wait 20 ms. Labels use 30 ms and spoken prompts
   use 40 ms, waiting for their stream before continuing the text sequence. */
static void update_simulation_overview(App *a) {
    uint32_t now = SDL_GetTicks();
    if (a->overview_ready || (int32_t)(now - a->overview_deadline) < 0) return;
    while (a->overview_line < 14) {
        const char *line = tactical_map_lines[a->overview_line];
        int *visible = &a->overview_visible[a->overview_line];
        while (line[*visible] == ' ') ++*visible;
        if (line[*visible]) {
            ++*visible;
            a->overview_deadline = now + 20u;
            return;
        }
        a->overview_line++;
    }
    if (a->stage == SIMULATION_OVERVIEW) a->stage = SIDE;
    int limit = a->stage == CITIES ? 9 : 6;
    if (a->overview_prompt >= limit) {
        a->overview_ready = 1;
        return;
    }
    int index = a->overview_prompt;
    if (!a->overview_prompt_started) {
        if (overview_prompts[index].voice)
            media_voice(&a->audio, overview_prompts[index].voice);
        else media_typewriter_start(&a->audio);
        a->overview_prompt_started = 1;
    }
    int *visible = &a->overview_prompt_visible[index];
    if (*visible < (int)strlen(overview_prompts[index].text)) {
        ++*visible;
        a->overview_deadline = now + overview_prompts[index].interval;
        return;
    }
    if (overview_prompts[index].voice && media_voice_active(&a->audio)) return;
    if (!overview_prompts[index].voice) media_sound_stop(&a->audio, 0);
    a->overview_prompt++;
    a->overview_prompt_started = 0;
}

/* The source's one-second helper waits for the next integer TIMER boundary,
   polling every 20 ms; it is not a fixed 1000-ms sleep. */
static uint32_t next_timer_second_deadline(void) {
    struct timespec time;
    uint32_t remaining = 1000u;
    if (clock_gettime(CLOCK_MONOTONIC, &time) == 0)
        remaining -= (uint32_t)(time.tv_nsec / 1000000L);
    return SDL_GetTicks() + ((remaining + 19u) / 20u) * 20u;
}

/* Bitmap pages alternate precise millisecond holds and TIMER-boundary waits. */
static void start_simulation_battle(App *a) {
    a->stage = SIMULATION_BATTLE;
    a->battle_index = 0;
    a->battle_warning_active = 0;
    a->battle_sound_channel = -1;
    a->line_count = 0;
    a->input[0] = 0;
    media_stop_voice(&a->audio);
    set_background(a, simulation_battle[0]);
    a->battle_deadline = next_timer_second_deadline();
}

static void advance_simulation_battle(App *a) {
    static const uint32_t holds_after_frame[] = {
        0u, 1800u, 500u, 0u, 500u, 0u, 0u
    };
    if (a->battle_index == 1) {
        media_sound_stop(&a->audio, a->battle_sound_channel);
        a->battle_sound_channel = -1;
        a->battle_warning_active = 0;
    }
    a->battle_index++;
    if (a->battle_index >= (int)(sizeof(simulation_battle) / sizeof(simulation_battle[0]))) {
        const StoryCaption *items = post_simulation_captions;
        a->caption_program = CAPTIONS_AFTER_SIMULATION;
        a->caption_index = 1;
        a->stage = STORY_CAPTION;
        set_background(a, NULL);
        set_native_caption(a, items[1].caption_asset);
        a->caption_deadline = caption_deadline(a->caption_program, 1);
        return;
    }
    set_background(a, simulation_battle[a->battle_index]);
    if (a->battle_index == 1) {
        a->battle_sound_channel = media_sound_loop_start(&a->audio, "Warmiswn");
        a->battle_warning_active = 1;
        a->battle_warning_started = SDL_GetTicks();
    }
    uint32_t hold = holds_after_frame[a->battle_index];
    a->battle_deadline = hold ? SDL_GetTicks() + hold : next_timer_second_deadline();
}

/* Text output overwrites console cells, including spaces. A zero coordinate
   preserves the current position, as in the original runtime's LOCATE call. */
static void tactical_write(App *a, int row, int column, const char *text, int newline) {
    if (row > 0) a->tactical_cursor_row = row;
    if (column > 0) a->tactical_cursor_column = column;
    for (size_t i = 0; text[i]; ++i) {
        int r = a->tactical_cursor_row - 1;
        int c = a->tactical_cursor_column - 1;
        if (r >= 0 && r < 37 && c >= 0 && c < 100)
            a->tactical_cells[r][c] = text[i];
        if (++a->tactical_cursor_column > 100) {
            a->tactical_cursor_column = 1;
            a->tactical_cursor_row++;
        }
    }
    if (newline) {
        a->tactical_cursor_column = 1;
        a->tactical_cursor_row++;
    }
}

static void tactical_dots(App *a, int visible) {
    static const int positions[][3] = {
        {2, 39, 1}, {3, 40, 3}, {4, 41, 1}, {11, 38, 1}, {11, 6, 1},
        {12, 37, 1}, {13, 36, 3}, {13, 3, 3}, {14, 5, 1}, {14, 35, 3}
    };
    for (size_t i = 0; i < sizeof(positions) / sizeof(positions[0]); ++i)
        tactical_write(a, positions[i][0], positions[i][1],
                       visible ? (positions[i][2] == 1 ? "." : ". .") :
                                 (positions[i][2] == 1 ? " " : "   "), 1);
}

/* The tactical map and tables are printed at once after Wartxt02. Only the
   earlier map uses the slow typewriter helper. */
static void start_simulation_tactical(App *a) {
    a->stage = SIMULATION_TACTICAL;
    a->tactical_frame = 0x10;
    a->tactical_exit_requested = 0;
    for (int row = 0; row < 37; ++row) {
        memset(a->tactical_cells[row], ' ', 100);
        a->tactical_cells[row][100] = 0;
    }
    a->tactical_cursor_row = a->tactical_cursor_column = 1;
    a->tactical_ready = 1;
    a->tactical_phase = 0;
    a->tactical_caption_visible = 0;
    a->tactical_compare_frame = a->tactical_compare_phase = -1;
    const char *hold = getenv("WARGAME_COMPARE_TACTICAL_HOLD");
    if (hold) sscanf(hold, "%d:%d", &a->tactical_compare_frame, &a->tactical_compare_phase);
    const char *skip = getenv("WARGAME_COMPARE_RNG_SKIP");
    if (skip) {
        unsigned long count = strtoul(skip, NULL, 10);
        if (count <= 100000ul) {
            game_rng_seed();
            while (count--) (void)game_rng_u32();
        }
    }
    a->input[0] = 0;
    a->line_count = 0;
    set_background(a, NULL);
    for (int i = 0; i < 14; ++i)
        tactical_write(a, i + 2, 6, tactical_map_lines[i], 1);
    tactical_write(a, 20, 5,
                   "TRAJECTORY HEADING      TRAJECTORY HEADING      TRAJECTORY HEADING      TRAJECTORY HEADING", 1);
    for (int i = 0; i < 4; ++i)
        tactical_write(a, 21, 5 + i * 24, "\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf", 1);
    for (int i = 0; i < 10; ++i)
        tactical_write(a, i < 5 ? 22 + i : 23 + i, 5, tactical_trajectories[i], i != 9);
    tactical_dots(a, 1);
    a->tactical_audio_channel = media_sound_start(&a->audio, "Wargtr");
    a->tactical_deadline = SDL_GetTicks() + 70u;
}

static void tactical_arcs(App *a) {
    if (a->tactical_frame == 16) {
        for (int row = 2; row <= 4; ++row) {
            tactical_write(a, row, row + 3, "\\\\\\\\", 1);
            tactical_write(a, row, 21 - row, "////", 1);
        }
        tactical_write(a, 5, 8, "^^^^", 1);
        tactical_write(a, 5, 16, "^^^^", 1);
    } else if (a->tactical_frame == 20) {
        tactical_write(a, 6, 8, "^^^^^", 1);
        tactical_write(a, 5, 16, "////", 1);
        tactical_write(a, 6, 15, "^^^^", 1);
    } else if (a->tactical_frame == 25) {
        tactical_write(a, 6, 15, "////", 1);
        tactical_write(a, 7, 14, "^^^^", 1);
    } else if (a->tactical_frame == 30) {
        tactical_write(a, 7, 14, "////", 1);
        tactical_write(a, 8, 13, "^^^^", 1);
    } else if (a->tactical_frame == 35) {
        tactical_write(a, 9, 12, "^^^^^", 1);
    }
}

static void advance_simulation_tactical(App *a) {
    uint32_t now = SDL_GetTicks();
    switch (a->tactical_phase) {
    case 0: /* Sound is cut after the first 70 ms; the source then waits 180 ms. */
        media_sound_stop(&a->audio, a->tactical_audio_channel);
        a->tactical_audio_channel = -1;
        a->tactical_phase = 1;
        a->tactical_deadline = now + 180u;
        return;
    case 1: { /* Source plots the frame, draws any caption and prepares random targets. */
        tactical_dots(a, 0);
        tactical_arcs(a);
        if (a->tactical_frame == 0x24 || a->tactical_frame == 0x25)
            set_native_caption(a, "Wartxt2");
        else if (a->tactical_frame == 0x26)
            set_native_caption(a, "Wartxt05");
        else if (a->tactical_frame == 0x28)
            set_native_caption(a, "Wartxt3");
        if (a->tactical_frame == 0x24 || a->tactical_frame == 0x25 ||
            a->tactical_frame == 0x26 || a->tactical_frame == 0x28)
            a->tactical_caption_visible = 1;
        if (a->tactical_exit_requested) {
            start_simulation_montage(a);
            return;
        }
        do {
            a->tactical_rows[0] = (int)floor(game_rng_unit() * 9.0) + 23;
            a->tactical_rows[1] = (int)floor(game_rng_unit() * 9.0) + 23;
        } while ((a->tactical_rows[0] >= 27 && a->tactical_rows[0] <= 28) ||
                 (a->tactical_rows[1] >= 27 && a->tactical_rows[1] <= 28));
        static const int columns[] = {14, 38, 62, 86};
        for (int i = 0; i < 2; ++i) {
            int index = (int)floor(game_rng_unit() * 4.0);
            a->tactical_columns[i] = columns[index];
        }
        for (int i = 0; i < 4; ++i)
            a->tactical_coordinates[i] = (unsigned)floor(game_rng_unit() * 899.0) + 100u;
        a->tactical_pause_ms = 250u + (unsigned)floor(game_rng_unit() * 600.0);
        tactical_write(a, a->tactical_rows[0], a->tactical_columns[0], "       ", 1);
        tactical_write(a, a->tactical_rows[1], a->tactical_columns[1], "       ", 1);
        a->tactical_audio_channel = media_sound_start(&a->audio, "Wargtr");
        a->tactical_phase = 2;
        a->tactical_deadline = now + 70u;
        return;
    }
    case 2:
        media_sound_stop(&a->audio, a->tactical_audio_channel);
        a->tactical_audio_channel = -1;
        a->tactical_phase = 3;
        a->tactical_deadline = now + 180u;
        return;
    case 3:
        a->tactical_phase = 4;
        a->tactical_deadline = now + a->tactical_pause_ms;
        return;
    default:
        for (int i = 0; i < 2; ++i) {
            char values[32];
            snprintf(values, sizeof(values), "%u %u", a->tactical_coordinates[i * 2],
                     a->tactical_coordinates[i * 2 + 1]);
            tactical_write(a, a->tactical_rows[i], a->tactical_columns[i], values, 1);
        }
        if (a->tactical_frame == 0x24 || a->tactical_frame == 0x25)
            a->tactical_caption_visible = 0;
        if (a->tactical_exit_requested || a->tactical_frame >= 0xfa) {
            start_simulation_montage(a);
            return;
        }
        a->tactical_frame++;
        a->tactical_phase = 0;
        tactical_dots(a, 1);
        a->tactical_audio_channel = media_sound_start(&a->audio, "Wargtr");
        a->tactical_deadline = now + 70u;
        return;
    }
}

/* Advance the current story card, then enter its next chapter or interaction. */
static void advance_story_caption(App *a) {
    int count = 0;
    const StoryCaption *items = caption_program_data(a->caption_program, &count);
    if (a->caption_program == CAPTIONS_AFTER_SIMULATION && a->caption_index == 0) {
        start_simulation_battle(a);
        return;
    }
    if (a->caption_program == CAPTIONS_AFTER_SIMULATION && a->caption_index == 1) {
        start_simulation_tactical(a);
        return;
    }
    if (a->caption_program == CAPTIONS_NORAD_12_23 && a->caption_index == 7)
        media_sound(&a->audio, "Warmiswn");
    a->caption_index++;
    if (items && a->caption_index < count) {
        if (a->caption_program == CAPTIONS_AFTER_SIMULATION)
            set_background(a, NULL);
        else
            set_background(a, items[a->caption_index].background_asset);
        if (a->caption_program == CAPTIONS_AFTER_SIMULATION)
            set_native_caption(a, items[a->caption_index].caption_asset);
        else set_story_caption(a, items[a->caption_index].caption_asset);
        if (a->caption_program == CAPTIONS_POST_HACK && a->caption_index == 0 &&
            a->caption_texture)
            SDL_SetTextureColorMod(a->caption_texture, 239, 239, 239);
        a->caption_deadline = caption_waits_for_input(a->caption_program)
            ? 0 : caption_deadline(a->caption_program, a->caption_index);
        return;
    }
    if (a->caption_texture) SDL_DestroyTexture(a->caption_texture);
    a->caption_texture = NULL;
    Stage next = a->after_caption_stage;
    const char *background = a->after_caption_background[0] ? a->after_caption_background : NULL;
    if (next == SIMULATION_MONTAGE) {
        start_simulation_montage(a);
    } else if (next == NORAD_PRELUDE) {
        start_caption_sequence(a, CAPTIONS_NORAD_12_23, NORAD_CLIP_2, "Wargisld");
    } else if (next == NORAD_CLIP_2) {
        static const char *const clip[] = {"War2"};
        start_video_sequence(a, clip, 1, NORAD_CLIP_3, "Wardefc1", CAPTIONS_NORAD_24_25);
    } else if (next == NORAD_CLIP_3) {
        static const char *const clip[] = {"War3"};
        start_video_sequence(a, clip, 1, NORAD_CLIP_4, "Wardefc1", CAPTIONS_NORAD_26_30);
    } else if (next == NORAD_CLIP_4) {
        static const char *const clip[] = {"War4"};
        start_video_sequence(a, clip, 1, LOGON_FINAL, NULL, CAPTIONS_NORAD_31_32);
    } else {
        start_stage(a, next, background);
    }
}

/* Add the computer's current message and the input hint for the active story state. */
static void stage_prompt(App *a) {
    switch (a->stage) {
    case INTRO: break;
    case PHONE: case SCANNING:
        break;
    case VIDEO_PLAYBACK: case SIMULATION_OVERVIEW: case SIMULATION_WARMUP: case SIMULATION_BATTLE: case SIMULATION_TACTICAL: case SIMULATION_MONTAGE: case NORAD_PRELUDE: case NORAD_CLIP_2: case NORAD_CLIP_3: case NORAD_CLIP_4: break;
    case STORY_CAPTION: break;
    case DIAL_NUMBERS:
        push_line(a, "NUMBERS FOR SPECIFIC CARRIER TONES WERE DETECTED:");
        push_line(a, "(311) 399 2364   (311) 399 3582   (311) 437 8739");
        push_line(a, "(311) 437 1883   (311) 437 2977   (311) 767 7285");
        push_line(a, "(311) 767 3795   (311) 936 3923   (311) 936 1493");
        push_line(a, "");
        refresh_carrier_selection(a);
        break;
    case BANK_PROMPT:
        push_line(a, "TEG SYSTEM A-15 24:34:33                    Y- 1293.323");
        push_line(a, "Union Pacific Bank - SouthWest Regional");
        push_line(a, "Data Control");
        push_line(a, "Logon:");
        break;
    case DIAL_MENU:
        push_line(a, "Pan Am Reservation System     DTRA:2342:435:936");
        push_line(a, "Technical Status: Active");
        push_line(a, "1. Flight Schedule    2. Credit Check");
        push_line(a, "3. Passenger Information");
        push_line(a, "4. System Status Function    6. Alternate Routine");
        push_line(a, "Menu Choice:");
        break;
    case DIAL_DESTINATION: push_line(a, "Destination:"); break;
    case DIAL_DEPARTURE: push_line(a, "Place of departure:"); break;
    case DIAL_PASSENGER: push_line(a, "Name of Passenger:"); break;
    case DIAL_TICKET: break;
    case LOGON: case LOGON_AGAIN: push_line(a, "LOGON:"); break;
    case LOGON_FINAL:
        push_line(a, "Attention To Consoles");
        push_line(a, "Inrt  06/592  STS Chk4");
        push_line(a, "PBCD 92  MANUAL LONG, LOOP RESET");
        push_line(a, "To Consoles  14-17");
        push_line(a, "Resp: TFT 06/592  575 4");
        push_line(a, "Timeref:  INT 05 # 321 615 454");
        push_line(a, ".013810373477       .327846317871       .232434563279       .782131782461");
        push_line(a, ".394872661983       .843721394893       .214286785472       .823975485775");
        push_line(a, ".120348525025       .410340148434       .124004741213       .631138127487");
        push_line(a, ".476138914618       .831368599119       .234459642113       .917491015343");
        break;
    case LOGON_REJECTED: break;
    case GREETING: push_line(a, "GREETINGS PROFESSOR FALKEN."); break;
    case HOW_ARE_YOU: push_line(a, "HOW ARE YOU TODAY?"); break;
    case MISTAKES:
        push_line(a, "EXCELLENT,  IT'S BEEN A LONG TIME.");
        push_line(a, "CAN YOU EXPLAIN THE REMOVAL OF YOUR USER ACCOUNT ON 6/23/73?");
        break;
    case PLAY:
        if (!a->play_prompt_pending) push_line(a, "SHALL WE PLAY A GAME?");
        break;
    case CHESS_REFUSAL: push_line(a, "SORRY, I DO NOT REALLY PLAY CHESS IN THIS PROGRAM"); break;
    case LATER: push_line(a, "WOULDN'T YOU PREFER A GOOD GAME OF CHESS?"); break;
    case SIMULATION_ACCEPTED: push_line(a, "FINE."); break;
    case MODEM_CONNECT: break;
    case SIDE:
        push_line(a, "WHICH SIDE DO YOU WANT?");
        push_line(a, "1.    UNITED STATES");
        push_line(a, "2.    SOVIET UNION");
        push_line(a, "PLEASE CHOOSE ONE:");
        break;
    case CITIES:
        push_line(a, "AWAITING FIRST STRIKE COMMAND");
        push_line(a, "PLEASE LIST PRIMARY TARGETS");
        push_line(a, "BY CITY AND/OR COUNTY NAME:");
        break;
    case GREETING3: push_line(a, "GREETINGS PROFESSOR FALKEN."); break;
    case IDENTIFICATION:
        push_line(a, "SORRY TO HEAR THAT, PROFESSOR.");
        push_line(a, "YESTERDAYS GAME WAS INTERRUPTED.");
        push_line(a, "ALTHOUGH PRIMARY GOAL HAS NOT YET BEEN ACHIEVED.");
        push_line(a, "SOLUTION IS NEAR.");
        break;
    case PRIMARY_GOAL:
        push_line(a, "YOU SHOULD KNOW PROFESSOR.");
        push_line(a, "YOU PROGRAMMED ME.");
        break;
    case WIN_STATEMENT: push_line(a, "TO WIN THE GAME."); break;
    case STILL_PLAYING: push_line(a, "GREETINGS PROFESSOR FALKEN."); break;
    case REAL_OR_GAME:
        push_line(a, "OF COURSE, I SHOULD REACH DEFCON 1");
        push_line(a, "AND LAUNCH MY MISSILES IN 28 HOURS.");
        push_line(a, "WOULD YOU lIKE TO SEE SOME PROJECTED KILL RATIOS?");
        break;
    case LIST_GAMES:
        push_line(a, "FALKEN'S MAZE");
        push_line(a, "BLACKJACK");
        push_line(a, "GIN RUMMY");
        push_line(a, "HEARTS");
        push_line(a, "BRIDGE");
        push_line(a, "CHECKERS");
        push_line(a, "CHESS");
        push_line(a, "POKER");
        push_line(a, "FIGHTER COMBAT");
        push_line(a, "GUERILLA ENGAGEMENT");
        push_line(a, "DESERT WARFARE");
        push_line(a, "AIR-TO-GROUND ACTIONS");
        push_line(a, "THEATERWIDE TACTICAL WARFARE");
        push_line(a, "THEATERWIDE BIOTOXIC AND CHEMICAL WARFARE");
        push_line(a, "GLOBAL THERMONUCLEAR WAR");
        break;
    case GAME_SHELL: push_line(a, "LOGON:"); break;
    case CHESS: push_line(a, "HOW ABOUT A NICE GAME OF CHESS?"); break;
    case TIC_TAC_TOE:
        push_line(a, "ONE OR TWO PLAYERS?");
        push_line(a, "PLEASE ENTER NUMBER OF PLAYERS.");
        break;
    case TIC_MARK: push_line(a, "X or O?"); break;
    case TIC_MOVE: push_line(a, "PLEASE SELECT A NUMBER:"); break;
    case TIC_ANIMATION: break;
    case TIC_AGAIN:
        push_line(a, "STALEMATE.");
        push_line(a, "WANT TO PLAY AGAIN?");
        break;
    case ENDING_PRELUDE: case ENDING_MUSIC: break;
    case FINAL_GREETING: push_line(a, "GREETINGS PROFESSOR FALKEN"); break;
    case ENDING:
        push_line(a, "A STRANGE GAME.");
        push_line(a, "THE ONLY WINNING MOVE IS");
        push_line(a, "NOT TO PLAY.");
        break;
    case ENDING_CHESS: push_line(a, "HOW ABOUT A NICE GAME OF CHESS?"); break;
    }
}

/* Change story state, refresh its backdrop, and start the matching voice line if known. */
static void start_stage(App *a, Stage stage, const char *background) {
    if (getenv("WARGAME_TRACE_INPUT"))
        fprintf(stderr, "stage=%d background=%s\n", (int)stage, background ? background : "(keep)");
    a->stage = stage;
    a->stage_started = SDL_GetTicks();
    if (stage == PLAY && a->deferred_voice[0]) {
        a->play_prompt_pending = 1;
        a->play_prompt_deadline = a->deferred_voice_deadline;
    } else if (stage != PLAY) a->play_prompt_pending = 0;
    if (stage == TIC_TAC_TOE) {
        a->tic_index = 0;
        a->tic_phase = 0;
        a->tic_deadline = 0;
        memset(a->tic_board, 0, sizeof(a->tic_board));
    }
    if (stage == PHONE || stage == SCANNING || stage == DIAL_NUMBERS ||
        stage == BANK_PROMPT || stage == DIAL_MENU || stage == DIAL_DESTINATION ||
        stage == DIAL_DEPARTURE || stage == DIAL_PASSENGER || stage == DIAL_TICKET ||
        stage == MODEM_CONNECT || stage == SIDE || stage == LOGON || stage == GREETING ||
        stage == LOGON_AGAIN || stage == LOGON_FINAL || stage == LOGON_REJECTED ||
        stage == GAME_SHELL)
        a->line_count = 0;
    if (stage != LIST_GAMES) a->game_list_waiting = 0;
    if (stage == DIAL_NUMBERS) {
        a->carrier_selection = 7;
        a->carrier_navigated = 0;
        a->bank_connecting = 0;
        a->bank_key_pending = 0;
    }
    if (stage == PHONE) {
        a->scan_digit_index = 0;
        a->scan_digits[0] = 0;
    }
    if (stage == BANK_PROMPT) a->bank_sound_mask = 0;
    if (a->caption_texture) SDL_DestroyTexture(a->caption_texture);
    a->caption_texture = NULL;
    if (background) set_background(a, background);
    const char *voice = NULL;
    switch (stage) {
    case FINAL_GREETING: voice = "Wargpf3"; break;
    case GREETING3: voice = "Wargpf1"; break;
    case MISTAKES: voice = "Waribalt"; break;
    case WIN_STATEMENT: voice = "Wartwtg"; break;
    case PLAY: voice = "Warswpag"; break;
    case LATER: voice = "Warggoc"; break;
    case SIMULATION_ACCEPTED: voice = "Warfine"; break;
    case PRIMARY_GOAL: voice = "Warypgme"; break;
    case CHESS: case CHESS_REFUSAL: voice = "Wardnpc"; break;
    case STILL_PLAYING: voice = "Wargpf2"; break;
    case REAL_OR_GAME: voice = "Warpkr"; break;
    case ENDING: voice = "Warasgam"; break;
    case ENDING_CHESS: voice = "Warngoc"; break;
    case ENDING_MUSIC: voice = "Warend"; break;
    default: break;
    }
    if (voice && !a->deferred_voice[0]) media_voice(&a->audio, voice);
    else if (stage == PHONE || stage == SCANNING || stage == DIAL_NUMBERS || stage == BANK_PROMPT || stage == DIAL_MENU || stage == LOGON || stage == LOGON_AGAIN || stage == LOGON_FINAL || stage == GAME_SHELL)
        media_stop_voice(&a->audio);
    if (stage == SIMULATION_ACCEPTED)
        a->overview_deadline = SDL_GetTicks() + 1500u;
    if (stage == MODEM_CONNECT) {
        if (!a->modem_ring_started) media_phone_ring(&a->audio);
        a->modem_ring_started = 1;
        set_story_caption(a, "Wartxt6");
    }
    if (stage == LIST_GAMES && !a->game_list_shown) {
        a->game_list_shown = 1;
        a->game_list_waiting = 1;
        a->game_list_deadline = SDL_GetTicks() + 500u;
        media_sound(&a->audio, "Warlstgm");
    }
    /* FUN_0040b590 starts this cue once on entry. Later input prompts reuse
       the same game loop and do not restart it for every selected square. */
    if (stage == TIC_TAC_TOE) media_sound(&a->audio, "Wartttsc");
    if (stage == TIC_MARK) media_sound(&a->audio, "Warxor0");
    if (stage == ENDING_PRELUDE) a->ending_deadline = SDL_GetTicks() + 2000;
    if (stage == FINAL_GREETING) a->ending_deadline = SDL_GetTicks() + 1000;
    if (stage == ENDING) a->ending_deadline = SDL_GetTicks() + 5000;
    if (stage == ENDING_CHESS) a->ending_deadline = SDL_GetTicks() + 3000;
    if (stage != LIST_GAMES || !a->game_list_waiting) stage_prompt(a);
}

static void start_intro(App *a) {
    a->stage = INTRO;
    a->intro_started = SDL_GetTicks() + 2600u;
    snprintf(a->deferred_voice, sizeof(a->deferred_voice), "%s", "Warintro");
    a->deferred_voice_deadline = a->intro_started;
    stage_prompt(a);
}

/* Queue one or more original MPEG clips and record what state should follow them. */
static void start_video_sequence(App *a, const char *const *names, int count,
                                 Stage next_stage, const char *next_background,
                                 CaptionProgram after_captions) {
    media_stop_voice(&a->audio);
    if (count > 0 && strcmp(names[0], "War2") == 0) {
        media_sound(&a->audio, "Warswoff");
        media_sound(&a->audio, "Warswof2");
        media_sound(&a->audio, "Waracdnd");
    } else if (count > 0 && strcmp(names[0], "War5") == 0) {
        media_sound(&a->audio, "Wargrr");
    }
    a->video_count = count;
    a->video_index = 0;
    a->after_video_stage = next_stage;
    a->after_video_captions = after_captions;
    snprintf(a->after_video_background, sizeof(a->after_video_background), "%s", next_background ? next_background : "");
    for (int i = 0; i < count; i++) snprintf(a->video_queue[i], sizeof(a->video_queue[i]), "%s", names[i]);
    a->line_count = 0;
    a->input[0] = 0;
    a->stage = VIDEO_PLAYBACK;
    if (count == 0 || !media_video_start(&a->video, a->renderer, a->video_queue[0])) {
        a->video_index = a->video_count;
        if (a->after_video_captions != CAPTIONS_NONE)
            start_caption_sequence(a, a->after_video_captions, a->after_video_stage,
                                  a->after_video_background[0] ? a->after_video_background : NULL);
        else start_stage(a, a->after_video_stage, a->after_video_background[0] ? a->after_video_background : NULL);
    }
}

/* Advance to the next queued film, or return control to the story. */
static void finish_current_video(App *a) {
    media_video_stop(&a->video);
    a->video_index++;
    if (a->video_index < a->video_count &&
        media_video_start(&a->video, a->renderer, a->video_queue[a->video_index])) return;
    a->video_index = a->video_count;
    if (a->after_video_captions != CAPTIONS_NONE)
        start_caption_sequence(a, a->after_video_captions, a->after_video_stage,
                              a->after_video_background[0] ? a->after_video_background : NULL);
    else start_stage(a, a->after_video_stage, a->after_video_background[0] ? a->after_video_background : NULL);
}

/* Normalize typed text for case-insensitive command matching. */
static void lower_copy(char *out, size_t cap, const char *in) {
    size_t i = 0;
    for (; in[i] && i + 1 < cap; i++) out[i] = (char)tolower((unsigned char)in[i]);
    out[i] = 0;
}

/* Reproduce the original terminal's title-casing of echoed words. */
static void terminal_case_copy(char *out, size_t cap, const char *in) {
    int capitalize = 1;
    size_t i = 0;
    for (; in[i] && i + 1 < cap; ++i) {
        unsigned char ch = (unsigned char)in[i];
        if (isalpha(ch)) {
            out[i] = (char)(capitalize ? toupper(ch) : tolower(ch));
            capitalize = 0;
        } else {
            out[i] = (char)ch;
            if (isspace(ch)) capitalize = 1;
        }
    }
    out[i] = 0;
}

/* Find a lower-case phrase in a normalized player response. */
static int has(const char *s, const char *part) { return strstr(s, part) != NULL; }

/* Match the short leading and trailing response tokens used by the original dialogues. */
static int starts(const char *s, const char *prefix) { return strncmp(s, prefix, strlen(prefix)) == 0; }
static int ends(const char *s, const char *suffix) {
    size_t n = strlen(s), m = strlen(suffix);
    return n >= m && memcmp(s + n - m, suffix, m) == 0;
}

/* Apply one player response to the current narrative state. */
static void submit(App *a) {
    char s[LINE_LEN];
    char response[LINE_LEN];
    snprintf(response, sizeof(response), "%s", a->input);
    lower_copy(s, sizeof(s), a->input);
    if (getenv("WARGAME_TRACE_INPUT"))
        fprintf(stderr, "submit stage=%d input=[%s]\n", (int)a->stage, a->input);
    if (!*s && a->stage != PHONE && a->stage != SCANNING &&
        a->stage != DIAL_NUMBERS && a->stage != DIAL_TICKET && a->stage != PLAY &&
        a->stage != CHESS_REFUSAL && a->stage != LATER) return;
    if (*s) {
        char echo[LINE_LEN];
        if (a->stage == LOGON_FINAL || a->stage == LIST_GAMES)
            snprintf(echo, sizeof(echo), "LOGON: %.232s", a->input);
        else if (a->stage == LOGON)
            snprintf(echo, sizeof(echo), "LOGON: %.232s", a->input);
        else terminal_case_copy(echo, sizeof(echo), a->input);
        push_line(a, echo);
    }
    a->input[0] = 0;

    switch (a->stage) {
    case INTRO:
        break;
    case PHONE:
        /* The original accepts the scan digits one key at a time, without Return. */
        break;
    case SCANNING:
        start_stage(a, DIAL_NUMBERS, "");
        break;
    case VIDEO_PLAYBACK: case SIMULATION_WARMUP:
        break;
    case SIMULATION_BATTLE: case SIMULATION_TACTICAL: case SIMULATION_MONTAGE:
    case SIMULATION_ACCEPTED: case SIMULATION_OVERVIEW:
        break;
    case STORY_CAPTION:
        break;
    case DIAL_NUMBERS:
        if (a->carrier_selection == 7) {
            if (a->bank_connecting) {
                a->bank_key_pending = 1;
            } else {
                a->bank_connecting = 1;
                a->bank_connect_started = SDL_GetTicks();
            }
        }
        else if (a->carrier_selection == 8) {
            media_dial(&a->audio, 0);
            start_stage(a, DIAL_MENU, "");
        } else if (a->carrier_selection == 9) {
            media_dial(&a->audio, 1);
            start_stage(a, LOGON, "");
        }
        break;
    case BANK_PROMPT:
        start_stage(a, DIAL_NUMBERS, "");
        break;
    case DIAL_MENU:
        /* The original reads a menu response, then always continues to Destination. */
        start_stage(a, DIAL_DESTINATION, NULL);
        break;
    case DIAL_DESTINATION:
        if (has(s, "paris")) {
            snprintf(a->panam_destination, sizeof(a->panam_destination), "%s", response);
            media_dial(&a->audio, 0); start_stage(a, DIAL_DEPARTURE, NULL);
        }
        else start_stage(a, DIAL_NUMBERS, "");
        break;
    case DIAL_DEPARTURE:
        if (has(s, "chicago")) {
            snprintf(a->panam_departure, sizeof(a->panam_departure), "%s", response);
            media_dial(&a->audio, 1); start_stage(a, DIAL_PASSENGER, NULL);
        }
        else start_stage(a, DIAL_NUMBERS, "");
        break;
    case DIAL_PASSENGER:
        if (*s) {
            snprintf(a->panam_passenger, sizeof(a->panam_passenger), "%s", response);
            start_stage(a, DIAL_TICKET, NULL);
            push_line(a, "Date of Issue   08/08/84");
            push_line(a, "Departure Date  08/18/84  Flight: 114");
            push_line(a, "From: Chicago/Ohara");
            push_line(a, "Carrier:    Flight:    Class:    Date:    Time:     Status:");
            push_line(a, "To: Begaul/Paris");
            push_line(a, "Panam        114        Q       18Aug.   08:15A      ok");
            push_line(a, "Fare   1164.10");
            push_line(a, "6Jagkar    SUM  172.00  TWA   6JA");
            push_line(a, "Tax      70.90");
            push_line(a, "172.00       170.00");
            push_line(a, "Total $1235.00");
            push_line(a, "Request: Spec. Data:");
        }
        break;
    case DIAL_TICKET:
        start_stage(a, DIAL_NUMBERS, "");
        break;
    case LOGON:
        snprintf(a->terminal_last_input, sizeof(a->terminal_last_input), "%s", response);
        if (strcmp(s, "joshua") == 0) {
            a->logon_info_count = 0;
            start_stage(a, GREETING, "");
        } else if (strcmp(s, "help") == 0) {
            static const char *const help[] = {"HELP NOT AVAILABLE."};
            set_logon_info(a, help, 1);
        } else if (strcmp(s, "games") == 0 || strcmp(s, "help games") == 0) {
            static const char *const games_info[] = {
                "'GAMES' REFERS TO MODELS, SIMULATIONS AND GAMES",
                "WHICH HAVE TACTICAL AND STRATEGIC APPLICATIONS."
            };
            set_logon_info(a, games_info, 2);
        } else if (strcmp(s, "list games") == 0) {
            static const char *const games[] = {
                "FALKEN'S MAZE", "BLACKJACK", "GIN RUMMY", "HEARTS", "BRIDGE",
                "CHECKERS", "CHESS", "POKER", "FIGHTER COMBAT",
                "GUERILLA ENGAGEMENT", "DESERT WARFARE", "AIR-TO-GROUND ACTIONS",
                "THEATERWIDE TACTICAL WARFARE",
                "THEATERWIDE BIOTOXIC AND CHEMICAL WARFARE",
                "GLOBAL THERMONUCLEAR WAR"
            };
            set_logon_info(a, games, (int)(sizeof(games) / sizeof(games[0])));
            media_sound(&a->audio, "Warlstgm");
        } else if (strcmp(s, "franco") == 0) {
            static const char *const profile[] = {
                "**FRANCO TOGLIATTO**",
                "SYSTEM INDICATES USER (AKA FALCON) ORIGINATOR OF THE PROGRAM",
                "CURENTLY LIVING IN AUSTRALIA INVOLVED IN STOCKMARKET TRADING"
            };
            set_logon_info(a, profile, 3);
        } else {
            a->logon_info_count = 0;
            start_stage(a, LOGON_REJECTED, "");
        }
        break;
    case LOGON_REJECTED:
        break;
    case GREETING:
        if (starts(s, "hello") || starts(s, "how") || ends(s, "?"))
            start_stage(a, HOW_ARE_YOU, NULL);
        break;
    case HOW_ARE_YOU:
        if (starts(s, "i") || starts(s, "ho") || starts(s, "g") || ends(s, "?"))
            start_stage(a, MISTAKES, NULL);
        break;
    case MISTAKES:
        if (starts(s, "peo")) {
            push_line(a, "YES THEY DO.");
            media_voice(&a->audio, "Warytd");
            snprintf(a->deferred_voice, sizeof(a->deferred_voice), "%s", "Warswpag");
            a->deferred_voice_deadline = SDL_GetTicks() + 1000u;
            start_stage(a, PLAY, NULL);
        }
        break;
    case PLAY:
        if (has(s, "chess")) start_stage(a, CHESS_REFUSAL, NULL);
        else if (starts(s, "lov") || starts(s, "let") || starts(s, "ye"))
            start_stage(a, LATER, NULL);
        else if (*s) start_stage(a, SIMULATION_ACCEPTED, NULL);
        break;
    case CHESS_REFUSAL:
        if (*s) start_stage(a, SIMULATION_ACCEPTED, NULL);
        break;
    case LATER:
        if (*s) start_stage(a, SIMULATION_ACCEPTED, NULL);
        break;
    case SIDE:
        /* FUN_0040fcb0(2) keeps waiting until the numeric input converts to 2. */
        if (strtod(s, NULL) == 2.0) {
            snprintf(a->side_response, sizeof(a->side_response), "%s", response);
            start_stage(a, CITIES, "");
            a->overview_ready = 0;
            a->overview_prompt = 6;
            a->overview_prompt_started = 0;
            a->overview_deadline = SDL_GetTicks() + 900u;
        }
        break;
    case CITIES:
        if (strcmp(s, "las vegas") == 0 || strcmp(s, "seattle") == 0) {
            if (!a->target_mask)
                terminal_case_copy(a->first_target, sizeof(a->first_target), response);
            a->target_mask |= strcmp(s, "las vegas") == 0 ? 1u : 2u;
        }
        if (a->target_mask == 3u) {
            terminal_case_copy(a->second_target, sizeof(a->second_target), response);
            start_caption_sequence(a, CAPTIONS_AFTER_SIMULATION, SIMULATION_MONTAGE, "War2400");
        }
        break;
    case GREETING3:
        if (starts(s, "i") || ends(s, "ad")) start_stage(a, IDENTIFICATION, NULL);
        break;
    case MODEM_CONNECT:
        break;
    case IDENTIFICATION:
        if (starts(s, "what") || ends(s, "?")) start_stage(a, PRIMARY_GOAL, NULL);
        break;
    case PRIMARY_GOAL:
        if (starts(s, "what")) start_stage(a, WIN_STATEMENT, NULL);
        break;
    case WIN_STATEMENT:
        if (*s) start_caption_sequence(a, CAPTIONS_FBI, LOGON_AGAIN, "Wargisld");
        break;
    case LOGON_AGAIN:
        if (has(s, "joshua")) start_stage(a, STILL_PLAYING, "");
        else push_line(a, "IDENTIFICATION NOT RECOGNIZED.");
        break;
    case STILL_PLAYING:
        start_stage(a, REAL_OR_GAME, NULL);
        break;
    case REAL_OR_GAME:
        media_sound(&a->audio, "Warstwt");
        start_caption_sequence(a, CAPTIONS_FBI_TAIL, NORAD_PRELUDE, "Wargisld");
        break;
    case LOGON_FINAL:
        if (has(s, "tic tac toe") || has(s, "tic-tac-toe")) {
            start_stage(a, TIC_TAC_TOE, "");
        } else if (has(s, "list games")) {
            start_stage(a, LIST_GAMES, NULL);
        } else if (has(s, "chess") && !a->played_chess) {
            static const char *const clip[] = {"War5"};
            a->played_chess = 1;
            start_video_sequence(a, clip, 1, LOGON_FINAL, "Wardefc1", CAPTIONS_NONE);
        } else if (has(s, "poker") && !a->played_poker) {
            static const char *const clip[] = {"War7"};
            a->played_poker = 1;
            start_video_sequence(a, clip, 1, LOGON_FINAL, "Wardefc1", CAPTIONS_NONE);
        } else if ((has(s, "global") || has(s, "theaterwide")) && !a->played_global_war) {
            static const char *const clip[] = {"War6"};
            a->played_global_war = 1;
            start_video_sequence(a, clip, 1, LOGON_FINAL, "Wardefc1", CAPTIONS_NONE);
        }
        break;
    case LIST_GAMES:
        if (has(s, "tic tac toe") || has(s, "tic-tac-toe")) {
            a->tic_index = 0;
            a->tic_mark = 0;
            a->tic_player_count = 0;
            start_stage(a, TIC_TAC_TOE, "");
        } else if (has(s, "list games")) {
            a->line_count = 0;
            a->game_list_waiting = 1;
            a->game_list_deadline = SDL_GetTicks() + 500u;
            media_sound(&a->audio, "Warlstgm");
        } else if (has(s, "chess") && !a->played_chess) {
            static const char *const clip[] = {"War5"};
            a->played_chess = 1;
            start_video_sequence(a, clip, 1, LIST_GAMES, "Wargenbg", CAPTIONS_NONE);
        } else if (has(s, "poker") && !a->played_poker) {
            static const char *const clip[] = {"War7"};
            a->played_poker = 1;
            start_video_sequence(a, clip, 1, LIST_GAMES, "Wargenbg", CAPTIONS_NONE);
        } else if ((has(s, "global") || has(s, "theaterwide")) && !a->played_global_war) {
            static const char *const clip[] = {"War6"};
            a->played_global_war = 1;
            start_video_sequence(a, clip, 1, LIST_GAMES, "Wargenbg", CAPTIONS_NONE);
        }
        break;
    case CHESS:
        if (has(s, "tic tac toe") || has(s, "tic-tac-toe")) start_stage(a, TIC_TAC_TOE, "Wartxt02");
        else push_line(a, "WOPR: Shall we play a game?");
        break;
    case TIC_TAC_TOE:
        if (strcmp(s, "0") == 0 || strcmp(s, "zero") == 0) {
            start_stage(a, ENDING_PRELUDE, NULL);
        } else if (strcmp(s, "1") == 0 || strcmp(s, "one") == 0) {
            a->tic_player_count = 1;
            start_stage(a, TIC_MARK, NULL);
        } else if (strcmp(s, "2") == 0 || strcmp(s, "two") == 0) {
            a->tic_player_count = 2;
            /* FUN_0040b590 recognizes 2/Two but repeats the player prompt;
               only 1/One exits this prompt loop in the original binary. */
        } else {
            start_stage(a, GAME_SHELL, NULL);
        }
        break;
    case TIC_MARK:
        if (strcmp(s, "x") == 0 || strcmp(s, "o") == 0) {
            a->tic_mark = s[0];
            a->tic_index = 0;
            start_stage(a, TIC_MOVE, NULL);
        } else start_stage(a, GAME_SHELL, NULL);
        break;
    case TIC_MOVE: {
        static const char *moves[] = {"5", "3", "8", "6", "1"};
        if (s[0] < '1' || s[0] > '9') {
            start_stage(a, GAME_SHELL, NULL);
        } else if (strcmp(s, moves[a->tic_index]) == 0) {
            static const int x_cells[] = {4, 2, 7, 5, 0};
            a->tic_board[x_cells[a->tic_index]] = 'X';
            a->tic_phase = 0;
            a->tic_deadline = SDL_GetTicks() + 750u;
            media_sound(&a->audio, "Warxttt");
            start_stage(a, TIC_ANIMATION, NULL);
        }
        break;
    }
    case TIC_ANIMATION:
        break;
    case TIC_AGAIN:
        if (strcmp(s, "y") == 0) {
            a->tic_index = 0;
            a->tic_mark = 0;
            a->tic_player_count = 0;
            memset(a->tic_board, 0, sizeof(a->tic_board));
            start_stage(a, TIC_TAC_TOE, "");
        } else {
            start_stage(a, GAME_SHELL, NULL);
        }
        break;
    case GAME_SHELL:
        if (strcmp(s, "0") == 0 || has(s, "zero")) {
            start_stage(a, ENDING_PRELUDE, NULL);
        } else if (has(s, "tic tac toe") || has(s, "tic-tac-toe")) {
            a->tic_index = 0;
            a->tic_mark = 0;
            a->tic_player_count = 0;
            memset(a->tic_board, 0, sizeof(a->tic_board));
            start_stage(a, TIC_TAC_TOE, "");
        }
        break;
    case ENDING_PRELUDE: break;
    case FINAL_GREETING:
        if (has(s, "hello") || has(s, "hi")) start_stage(a, ENDING, "");
        else push_line(a, "WOPR: GREETINGS PROFESSOR FALKEN.");
        break;
    case ENDING: case ENDING_CHESS: case ENDING_MUSIC: break;
    case NORAD_PRELUDE: case NORAD_CLIP_2: case NORAD_CLIP_3: case NORAD_CLIP_4: break;
    }
}

/* Draw one UTF-8 line using SDL_ttf and release the temporary render objects. */
static void draw_text(App *a, const char *text, int x, int y, SDL_Color c) {
    if (!a->font || !*text) return;
    SDL_Surface *surface = TTF_RenderUTF8_Solid_Wrapped(a->font, text, c, 740);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(a->renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (texture) { SDL_RenderCopy(a->renderer, texture, NULL, &dst); SDL_DestroyTexture(texture); }
}

/* FUN_004047f0 keeps the opening dialogue on one DOS text screen. Its
   responses are not a scrolling chat log: prompts occupy the original rows. */
static int opening_dialogue_line_y(const App *a, int index) {
    if (index == 0) return 64;
    const char *line = a->lines[index];
    if (strcmp(line, "HOW ARE YOU TODAY?") == 0) return 144;
    if (strncmp(line, "EXCELLENT,", 10) == 0) return 224;
    if (strncmp(line, "CAN YOU EXPLAIN", 15) == 0) return 240;
    if (strcmp(line, "YES THEY DO.") == 0) return 320;
    if (strcmp(line, "SHALL WE PLAY A GAME?") == 0) return 352;
    if (strncmp(line, "WOULDN'T YOU PREFER", 19) == 0 ||
        strncmp(line, "SORRY, I DO NOT", 15) == 0)
        return opening_dialogue_line_y(a, index - 1) + 48;
    if (strcmp(line, "FINE.") == 0)
        return opening_dialogue_line_y(a, index - 1) + 48;
    return opening_dialogue_line_y(a, index - 1) + 32;
}

static int is_opening_dialogue_stage(Stage stage) {
    return stage == HOW_ARE_YOU || stage == MISTAKES || stage == PLAY ||
           stage == CHESS_REFUSAL || stage == LATER || stage == SIMULATION_ACCEPTED;
}

/* Draw the solid 8-by-16 cursor used by the original text-mode interface. */
static void draw_block_cursor(App *a, int x, int y, SDL_Color color) {
    SDL_Rect cursor = {x, y, 8, 16};
    SDL_SetRenderDrawColor(a->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(a->renderer, &cursor);
}

/* FUN_00413750 draws each of the six X strokes independently. The constants
   below are the original 800x600 fractions from FUN_0040b590, kept as floats
   so the SDL rasterizer receives the same rounded endpoints. */
static void draw_tic_mark(App *a, int cell, char mark, SDL_Color color) {
    static const float x_strokes[9][4] = {
        {800.0f / 3.75f, 15.0f, 800.0f / 2.75f, 600.0f / 6.75f},
        {0},
        {800.0f / 1.575f, 15.0f, 800.0f / 1.365f, 600.0f / 6.75f},
        {0},
        {800.0f / 2.25f, 600.0f / 5.26f, 800.0f / 1.85f, 600.0f / 3.15f},
        {800.0f / 1.575f, 600.0f / 5.28f, 800.0f / 1.365f, 600.0f / 3.15f},
        {0},
        {800.0f / 2.25f, 600.0f / 2.8f, 800.0f / 1.85f, 600.0f / 2.06f},
        {0}
    };
    /* Four FUN_00414010 ellipses use these centers; each is drawn three
       times at radii 800/20.5, +1 and +2, as in the original. */
    static const float o_centers[9][2] = {
        {0}, {400.0f, 600.0f / 13.0f}, {0},
        {800.0f / 3.15f, 600.0f / 4.0f}, {0},
        {0}, {800.0f / 3.15f, 600.0f / 2.35f}, {0},
        {800.0f / 1.46f, 600.0f / 2.35f}
    };
    if (cell < 0 || cell > 8) return;
    SDL_SetRenderDrawColor(a->renderer, color.r, color.g, color.b, color.a);
    if (mark == 'X') {
        float x1 = x_strokes[cell][0], y1 = x_strokes[cell][1];
        float x2 = x_strokes[cell][2], y2 = x_strokes[cell][3];
        for (int offset = 0; offset < 3; ++offset) {
            SDL_RenderDrawLine(a->renderer, (int)lroundf(x1 + offset), (int)lroundf(y1),
                               (int)lroundf(x2 + offset), (int)lroundf(y2));
            SDL_RenderDrawLine(a->renderer, (int)lroundf(x2 + offset), (int)lroundf(y1),
                               (int)lroundf(x1 + offset), (int)lroundf(y2));
        }
    } else if (mark == 'O') {
        int cx = (int)lroundf(o_centers[cell][0]);
        int cy = (int)lroundf(o_centers[cell][1]);
        int radius = (int)lroundf(800.0f / 20.5f);
        for (int ring = 0; ring < 3; ++ring) {
            int x = radius + ring, y = 0, error = 1 - x;
            while (x >= y) {
                SDL_RenderDrawPoint(a->renderer, cx + x, cy + y);
                SDL_RenderDrawPoint(a->renderer, cx + y, cy + x);
                SDL_RenderDrawPoint(a->renderer, cx - y, cy + x);
                SDL_RenderDrawPoint(a->renderer, cx - x, cy + y);
                SDL_RenderDrawPoint(a->renderer, cx - x, cy - y);
                SDL_RenderDrawPoint(a->renderer, cx - y, cy - x);
                SDL_RenderDrawPoint(a->renderer, cx + y, cy - x);
                SDL_RenderDrawPoint(a->renderer, cx + x, cy - y);
                ++y;
                if (error < 0) error += 2 * y + 1;
                else { --x; error += 2 * (y - x) + 1; }
            }
        }
    }
}

/* Each accepted digit draws an X, waits 750 ms, then draws an O and waits
   another 750 ms. The fifth digit draws only the final X before STALEMATE. */
static void advance_tic_animation(App *a) {
    uint32_t now = SDL_GetTicks();
    if ((int32_t)(now - a->tic_deadline) < 0) return;
    if (a->tic_phase == 0 && a->tic_index < 4) {
        static const int o_cells[] = {8, 6, 1, 3};
        int cell = o_cells[a->tic_index];
        a->tic_board[cell] = 'O';
        media_sound(&a->audio, "Warottt");
        a->tic_phase = 1;
        a->tic_deadline = now + 750u;
        return;
    }
    if (a->tic_index == 4) {
        media_sound(&a->audio, "Warottt");
        start_stage(a, TIC_AGAIN, NULL);
        return;
    }
    a->tic_index++;
    a->tic_phase = 0;
    start_stage(a, TIC_MOVE, NULL);
}

/* Pick an installed monospace font, including the common Windows console font. */
static TTF_Font *open_font(void) {
    const char *paths[] = {
        "/usr/share/fonts/pc/Px_IBM_VGA8.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "C:\\Windows\\Fonts\\consola.ttf",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        TTF_Font *font = TTF_OpenFont(paths[i], i == 0 ? 16 : 13);
        if (font) return font;
    }
    return NULL;
}

/* Ask SDL's X11 backend to use the server's default visual so RGB colors stay exact. */
static void select_default_x11_visual(void) {
    Display *display = XOpenDisplay(NULL);
    if (!display) return;
    Visual *visual = DefaultVisual(display, DefaultScreen(display));
    char visual_id[32];
    snprintf(visual_id, sizeof(visual_id), "0x%lx",
             (unsigned long)XVisualIDFromVisual(visual));
    SDL_SetHint(SDL_HINT_VIDEO_X11_WINDOW_VISUALID, visual_id);
    XCloseDisplay(display);
}

/* Draw either the title/scene art and WOPR terminal, or the active video frame. */
static void render(App *a) {
    SDL_SetRenderDrawColor(a->renderer, 0, 0, 0, 255);
    SDL_RenderClear(a->renderer);
    if (a->stage == INTRO) {
        int32_t elapsed = (int32_t)(SDL_GetTicks() - a->intro_started);
        if (elapsed < 0) {
            SDL_RenderPresent(a->renderer);
            return;
        }
        if (a->background) {
            int width = 0, height = 0;
            SDL_QueryTexture(a->background, NULL, NULL, &width, &height);
            SDL_Rect title = width == 768 && height == 576
                ? (SDL_Rect){16, 0, 768, 576} : (SDL_Rect){0, 0, W, H};
            SDL_RenderCopy(a->renderer, a->background, NULL, &title);
        }
        SDL_SetRenderDrawColor(a->renderer, 0, 0, 0, 255);
        SDL_Rect banner_strip = {0, 568, W, 32};
        SDL_RenderFillRect(a->renderer, &banner_strip);
        if (a->intro_banner) {
            int scroll = (int)(elapsed / 5u);
            SDL_Rect banner = {W - scroll, 568, 1720, 32};
            SDL_RenderCopy(a->renderer, a->intro_banner, NULL, &banner);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == PHONE || a->stage == SCANNING) {
        SDL_Color cyan = {131, 237, 240, 255};
        const int columns[] = {15, 231, 448, 664};
        draw_text(a, "TO SCAN FOR CARRIER TONES, PLEASE LIST", 240, 48, cyan);
        draw_text(a, "DESIRED AREA CODES AND PREFIXES", 270, 68, cyan);
        for (int i = 0; i < 4; ++i) {
            draw_text(a, "AREA", columns[i], 98, cyan);
            draw_text(a, "CODE PRFX NUMBER", columns[i], 116, cyan);
        }
        SDL_SetRenderDrawColor(a->renderer, cyan.r, cyan.g, cyan.b, 255);
        SDL_RenderDrawLine(a->renderer, 14, 135, 790, 135);
        if (a->stage == PHONE) {
            for (unsigned i = 0; i < a->scan_digit_index; ++i) {
                unsigned group = i / 6;
                unsigned field_digit = i % 6;
                int x = columns[group] + (field_digit < 3 ? (int)field_digit * 8
                                                           : 48 + ((int)field_digit - 3) * 8);
                char digit[2] = {a->scan_digits[i], 0};
                draw_text(a, digit, x, 144, cyan);
            }
            unsigned group = a->scan_digit_index / 6;
            unsigned field_digit = a->scan_digit_index % 6;
            if (group > 3) group = 3;
            int x = columns[group] + (field_digit < 3 ? (int)field_digit * 8
                                                       : 48 + ((int)field_digit - 3) * 8);
            SDL_Rect cursor = {x, 144, 8, 16};
            SDL_SetRenderDrawColor(a->renderer, cyan.r, cyan.g, cyan.b, 255);
            SDL_RenderFillRect(a->renderer, &cursor);
        } else {
            uint32_t elapsed = SDL_GetTicks() - a->scan_started;
            static const char *const prefixes[] = {
                "(311) 399-", "(311) 437-", "(311) 767-", "(311) 936-"
            };
            if (elapsed < 16000u) {
                for (int i = 0; i < 4; ++i)
                    draw_text(a, prefixes[i], columns[i] - 7, 144, cyan);
                static const uint32_t times[] = {0u, 5700u, 12960u};
                for (int i = 0; i < 3; ++i) {
                    if (elapsed >= times[i]) {
                        char result[32];
                        snprintf(result, sizeof(result), "%s%04d", prefixes[0], i + 1);
                        draw_text(a, result, columns[0] - 7, 144 + i * 16, cyan);
                    }
                }
                SDL_Rect cursor = {8, 192, 8, 16};
                SDL_SetRenderDrawColor(a->renderer, cyan.r, cyan.g, cyan.b, 255);
                SDL_RenderFillRect(a->renderer, &cursor);
            } else {
                for (int column = 0; column < 3; ++column) {
                    int count = column < 2 ? 24 : 18 + (a->scan_dynamic_count < 6
                                                           ? a->scan_dynamic_count : 6);
                    if (count > 24) count = 24;
                    for (int row = 0; row < count; ++row) {
                        int number = 9977 + row;
                        char result[32];
                        snprintf(result, sizeof(result), "%s%04d", prefixes[column], number % 10000);
                        int y = 144 + row * 16;
                        if (y >= 144 && y < H - 16)
                            draw_text(a, result, columns[column] - 7, y, cyan);
                    }
                }
                int fourth_count = a->scan_dynamic_count > 6
                                       ? a->scan_dynamic_count - 6 : 0;
                for (int row = 0; row < fourth_count && row < 25; ++row) {
                    int y = 144 + (row - 1) * 16;
                    int number = (9976 + row) % 10000;
                    char result[32];
                    snprintf(result, sizeof(result), "%s%04d", prefixes[3], number);
                    if (y >= 144 && y < H - 16)
                        draw_text(a, result, columns[3] - 7, y, cyan);
                }
            }
            if (elapsed >= 13585u && a->caption_texture) {
                SDL_Rect story = {(W - 505) / 2, 280, 505, 40};
                SDL_RenderCopy(a->renderer, a->caption_texture, NULL, &story);
            }
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == DIAL_NUMBERS) {
        SDL_Color cyan = {131, 237, 240, 255};
        draw_text(a, "NUMBERS FOR SPECIFIC CARRIER TONES WERE DETECTED:", 207, 46, cyan);
        draw_text(a, "Please select a number and then press (RETURN) to begin dialing", 151, 272, cyan);
        for (int i = 0; i < 9; ++i) {
            draw_text(a, carrier_numbers[i], 335, 96 + i * 16, cyan);
            if (a->carrier_selection == i + 7) {
                if ((SDL_GetTicks() - a->stage_started) % 1000u < 500u)
                    draw_text(a, "===>", 296, 96 + i * 16, cyan);
                SDL_Rect cursor = {456, 96 + i * 16, 24, 16};
                SDL_SetRenderDrawColor(a->renderer, cyan.r, cyan.g, cyan.b, 255);
                SDL_RenderFillRect(a->renderer, &cursor);
            }
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == DIAL_MENU || a->stage == DIAL_DESTINATION ||
        a->stage == DIAL_DEPARTURE || a->stage == DIAL_PASSENGER ||
        a->stage == DIAL_TICKET) {
        SDL_Color cyan = {131, 237, 240, 255};
        if (a->stage == DIAL_MENU) {
            draw_text(a, "Pan Am Reservation System", 600, 16, cyan);
            draw_text(a, "DTRA:2342:435:936", 8, 48, cyan);
            draw_text(a, "Technical Status: Active", 8, 64, cyan);
            draw_text(a, "1. Flight Schedule", 8, 96, cyan);
            draw_text(a, "2. Credit Check", 8, 112, cyan);
            draw_text(a, "3. Passenger Information", 8, 128, cyan);
            draw_text(a, "4. System Status Function", 8, 144, cyan);
            draw_text(a, "6. Alternate Routine", 8, 160, cyan);
            draw_text(a, "Menu Choice", 8, 192, cyan);
            draw_block_cursor(a, 8, 224, cyan);
        } else if (a->stage == DIAL_DESTINATION) {
            draw_text(a, "Destination:", 8, 16, cyan);
            draw_text(a, a->input, 120, 16, cyan);
            draw_block_cursor(a, 120 + (int)strlen(a->input) * 8, 16, cyan);
        } else if (a->stage == DIAL_DEPARTURE) {
            draw_text(a, "Destination:", 8, 16, cyan);
            draw_text(a, a->panam_destination, 120, 16, cyan);
            draw_text(a, "Place of Departure:", 8, 48, cyan);
            draw_text(a, a->input, 176, 48, cyan);
            draw_block_cursor(a, 176 + (int)strlen(a->input) * 8, 48, cyan);
        } else if (a->stage == DIAL_PASSENGER) {
            draw_text(a, "Destination:", 8, 16, cyan);
            draw_text(a, a->panam_destination, 120, 16, cyan);
            draw_text(a, "Place of Departure:", 8, 48, cyan);
            draw_text(a, a->panam_departure, 176, 48, cyan);
            draw_text(a, "Name of Passenger:", 8, 80, cyan);
            draw_text(a, a->input, 168, 80, cyan);
            draw_block_cursor(a, 168 + (int)strlen(a->input) * 8, 80, cyan);
        } else if (a->stage == DIAL_TICKET) {
            draw_text(a, "Destination:", 8, 16, cyan);
            draw_text(a, a->panam_destination, 120, 16, cyan);
            draw_text(a, "Place of Departure:", 8, 48, cyan);
            draw_text(a, a->panam_departure, 176, 48, cyan);
            draw_text(a, "Name of Passenger:", 8, 80, cyan);
            draw_text(a, a->panam_passenger, 168, 80, cyan);
            draw_text(a, "Date of Issue", 584, 112, cyan);
            draw_text(a, "08/08/84", 712, 112, cyan);
            draw_text(a, "Departure Date", 488, 160, cyan);
            draw_text(a, "08/18/84", 616, 160, cyan);
            draw_text(a, "Flight: 114", 696, 160, cyan);
            draw_text(a, "From: Chicago/Ohara", 8, 192, cyan);
            draw_text(a, "Carrier:", 312, 192, cyan);
            draw_text(a, "Flight:", 408, 192, cyan);
            draw_text(a, "Class:", 496, 192, cyan);
            draw_text(a, "Date:", 576, 192, cyan);
            draw_text(a, "Time:", 648, 192, cyan);
            draw_text(a, "Status:", 728, 192, cyan);
            draw_text(a, "To: Begaul/Paris", 8, 208, cyan);
            draw_text(a, "Panam", 320, 208, cyan);
            draw_text(a, "114", 424, 208, cyan);
            draw_text(a, "Q", 512, 208, cyan);
            draw_text(a, "18Aug.", 576, 208, cyan);
            draw_text(a, "08:15A", 648, 208, cyan);
            draw_text(a, "ok", 744, 208, cyan);
            draw_text(a, "Fare   1164.10", 8, 240, cyan);
            draw_text(a, "6Jagkar", 312, 240, cyan);
            draw_text(a, "SUM", 400, 240, cyan);
            draw_text(a, "172.00", 440, 240, cyan);
            draw_text(a, "TWA", 504, 240, cyan);
            draw_text(a, "6JA", 552, 240, cyan);
            draw_text(a, "Tax", 8, 256, cyan);
            draw_text(a, "70.90", 80, 256, cyan);
            draw_text(a, "172.00", 320, 256, cyan);
            draw_text(a, "170.00", 424, 256, cyan);
            draw_text(a, "Total $1235.00", 8, 304, cyan);
            draw_text(a, "Request: Spec. Data:", 8, 352, cyan);
            draw_block_cursor(a, 8, 384, cyan);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == LOGON || a->stage == LOGON_REJECTED) {
        SDL_Color cyan = {131, 237, 240, 255};
        if (a->stage == LOGON) {
            char echoed[LINE_LEN];
            terminal_case_copy(echoed, sizeof(echoed), a->input);
            draw_text(a, "LOGON: ", 8, 64, cyan);
            if (echoed[0]) {
                draw_text(a, echoed, 64, 64, cyan);
                draw_block_cursor(a, 64 + (int)strlen(echoed) * 8, 64, cyan);
            } else draw_block_cursor(a, 64, 64, cyan);
            for (int i = 0; i < a->logon_info_count; ++i)
                draw_text(a, a->logon_info[i], 8, 96 + i * 22, cyan);
        } else if (a->stage == LOGON_REJECTED) {
            static const char error[] = "CONNECTION NOT RECOGNIZED BY SYSTEM .....";
            static const char terminated[] = "-- CONNECTION TERMINATED -- ";
            uint32_t elapsed = SDL_GetTicks() - a->stage_started;
            uint32_t error_duration = (uint32_t)strlen(error) * 30u;
            size_t visible = elapsed / 30u + 1u;
            if (visible > strlen(error)) visible = strlen(error);
            draw_typed_text(a, error, visible, 8, 96, cyan, elapsed < error_duration);
            if (elapsed >= error_duration + 1000u) {
                int error_width = 0;
                TTF_SizeUTF8(a->font, error, &error_width, NULL);
                uint32_t term_elapsed = elapsed - error_duration - 1000u;
                size_t term_visible = term_elapsed / 30u + 1u;
                if (term_visible > strlen(terminated)) term_visible = strlen(terminated);
                draw_typed_text(a, terminated, term_visible, 8 + error_width, 96, cyan,
                                term_elapsed < strlen(terminated) * 30u);
            }
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == GREETING) {
        SDL_Color cyan = {131, 237, 240, 255};
        char echoed[LINE_LEN];
        terminal_case_copy(echoed, sizeof(echoed), a->input);
        draw_text(a, "GREETINGS PROFESSOR FALKEN.", 8, 64, cyan);
        if (echoed[0]) draw_text(a, echoed, 8, 96, cyan);
        draw_block_cursor(a, 8 + (int)strlen(echoed) * 8, 96, cyan);
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == LOGON_FINAL) {
        SDL_Color cyan = {131, 237, 240, 255};
        if (a->background) {
            int width = 0, height = 0;
            SDL_QueryTexture(a->background, NULL, NULL, &width, &height);
            SDL_Rect dst = width == 768 && height == 576
                ? (SDL_Rect){16, 0, 768, 576} : (SDL_Rect){0, 0, W, H};
            SDL_RenderCopy(a->renderer, a->background, NULL, &dst);
        }
        int y = 16;
        for (int i = 0; i < a->line_count; ++i) {
            draw_text(a, a->lines[i], 8, y, cyan);
            y += 16;
        }
        char echoed[LINE_LEN];
        terminal_case_copy(echoed, sizeof(echoed), a->input);
        draw_text(a, "LOGON: ", 8, y + 6, cyan);
        draw_text(a, echoed, 64, y + 6, cyan);
        draw_block_cursor(a, 64 + (int)strlen(echoed) * 8, y + 6, cyan);
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == LIST_GAMES) {
        SDL_Color cyan = {131, 237, 240, 255};
        if (a->background) {
            int width = 0, height = 0;
            SDL_QueryTexture(a->background, NULL, NULL, &width, &height);
            SDL_Rect dst = width == 768 && height == 576
                ? (SDL_Rect){16, 0, 768, 576} : (SDL_Rect){0, 0, W, H};
            SDL_RenderCopy(a->renderer, a->background, NULL, &dst);
        }
        int y = 16;
        for (int i = 0; i < a->line_count; ++i) {
            draw_text(a, a->lines[i], 8, y, cyan);
            y += 16;
        }
        char echoed[LINE_LEN];
        terminal_case_copy(echoed, sizeof(echoed), a->input);
        draw_text(a, "LOGON: ", 8, y, cyan);
        draw_text(a, echoed, 64, y, cyan);
        draw_block_cursor(a, 64 + (int)strlen(echoed) * 8, y, cyan);
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (is_opening_dialogue_stage(a->stage)) {
        SDL_Color cyan = {131, 237, 240, 255};
        int last_y = 64;
        for (int i = 0; i < a->line_count; ++i) {
            last_y = opening_dialogue_line_y(a, i);
            draw_text(a, a->lines[i], 8, last_y, cyan);
        }
        int input_y = last_y + 32;
        if (a->stage != SIMULATION_ACCEPTED) {
            char echoed[LINE_LEN];
            terminal_case_copy(echoed, sizeof(echoed), a->input);
            if (echoed[0]) draw_text(a, echoed, 8, input_y, cyan);
            draw_block_cursor(a, 8 + (int)strlen(echoed) * 8, input_y, cyan);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == BANK_PROMPT) {
        SDL_Color cyan = {131, 237, 240, 255};
        static const char *const lines[] = {
            "TEG SYSTEM A-15 24:34:33", "Y- 1293.323",
            "Union Pacific Bank - SouthWest Regional", "Data Control", "Logon:"
        };
        static const int x[] = {8, 665, 8, 608, 8};
        static const int y[] = {16, 16, 32, 32, 46};
        static const uint32_t offsets[] = {0, 120, 180, 360, 420};
        static const size_t steps[] = {7, 7, 7, 7, 3};
        uint32_t elapsed = SDL_GetTicks() - a->stage_started;
        int active_line = 0;
        for (int i = 1; i < 5; ++i)
            if (elapsed >= offsets[i]) active_line = i;
        for (int i = 0; i < 5; ++i) {
            size_t length = strlen(lines[i]);
            size_t visible = bank_reveal_count(length, elapsed, offsets[i], steps[i]);
            const char *text = lines[i];
            char logon[LINE_LEN];
            if (i == 4 && a->input[0] && visible == length) {
                size_t label_length = strlen(lines[i]);
                size_t input_length = strlen(a->input);
                if (input_length > sizeof(logon) - label_length - 1)
                    input_length = sizeof(logon) - label_length - 1;
                memcpy(logon, lines[i], label_length);
                memcpy(logon + label_length, a->input, input_length);
                logon[label_length + input_length] = 0;
                text = logon;
                visible = strlen(text);
            }
            draw_typed_text(a, text, visible, x[i], y[i], cyan, i == active_line);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == SIMULATION_OVERVIEW || a->stage == SIDE || a->stage == CITIES ||
        a->stage == SIMULATION_WARMUP) {
        SDL_Color cyan = {131, 237, 240, 255};
        int cities = a->stage == CITIES || a->stage == SIMULATION_WARMUP;
        for (int i = 0; i < 14; ++i)
            draw_typed_text(a, tactical_map_lines[i], (size_t)a->overview_visible[i],
                            40, 16 + i * 16, cyan, 0);
        int limit = cities ? 9 : 6;
        for (int i = 0; i < limit; ++i) {
            int visible = a->overview_prompt_visible[i];
            /* The chosen side flashes off/on three times before the city prompt. */
            if (a->stage == CITIES && i == 4 &&
                SDL_GetTicks() - a->stage_started < 900u &&
                ((SDL_GetTicks() - a->stage_started) / 150u) % 2u == 0) {
                draw_text(a, "2.", 56, 352, cyan);
                continue;
            }
            draw_typed_text(a, overview_prompts[i].text, (size_t)visible,
                            (overview_prompts[i].column - 1) * 8,
                            (overview_prompts[i].row - 1) * 16,
                            cyan, 0);
        }
        if (cities) draw_text(a, a->side_response, 200, 384, cyan);
        if (cities && a->overview_prompt > 6) {
            /* Twenty-nine CP437 0xc4 cells form a continuous horizontal rule. */
            SDL_SetRenderDrawColor(a->renderer, cyan.r, cyan.g, cyan.b, 255);
            SDL_RenderDrawLine(a->renderer, 40, 455, 271, 455);
        }
        if (!a->overview_ready && a->overview_line >= 14 &&
            a->overview_prompt < limit && a->overview_prompt_started) {
            int i = a->overview_prompt;
            draw_block_cursor(a, (overview_prompts[i].column - 1 +
                                  a->overview_prompt_visible[i]) * 8,
                              (overview_prompts[i].row - 1) * 16, cyan);
        }
        if (cities && a->target_mask)
            draw_text(a, a->first_target, 40, 544, cyan);
        if (a->overview_ready) {
            char echoed[LINE_LEN];
            if (a->stage == SIDE) snprintf(echoed, sizeof(echoed), "%s", a->input);
            else terminal_case_copy(echoed, sizeof(echoed),
                                    a->stage == SIMULATION_WARMUP ? a->second_target : a->input);
            int x = a->stage == SIDE ? 200 : 40;
            int y = a->stage == SIDE ? 384 : a->target_mask ? 560 : 544;
            draw_text(a, echoed, x, y, cyan);
            if (a->stage != SIMULATION_WARMUP)
                draw_block_cursor(a, x + (int)strlen(echoed) * 8, y, cyan);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->background) {
        int width = 0, height = 0;
        SDL_QueryTexture(a->background, NULL, NULL, &width, &height);
        /* FUN_0040e470 mode 2 places 768x576 artwork at x=16, y=0
           (800/2 - 384), leaving the original 24-pixel bottom margin. */
        SDL_Rect dst = width == 768 && height == 576
            ? (SDL_Rect){16, 0, 768, 576} : (SDL_Rect){0, 0, W, H};
        SDL_RenderCopy(a->renderer, a->background, NULL, &dst);
    }
    if (a->stage == LIST_GAMES && a->game_list_waiting) {
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == MODEM_CONNECT) {
        if (a->caption_texture) {
            SDL_Rect card = {(W - 505) / 2, 280, 505, 40};
            SDL_RenderCopy(a->renderer, a->caption_texture, NULL, &card);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == SIMULATION_BATTLE && a->battle_warning_active &&
        a->battle_warning_texture) {
        uint32_t elapsed = SDL_GetTicks() - a->battle_warning_started;
        if (a->battle_warning_active == 2 ||
            (elapsed < 1800u && ((elapsed / 150u) & 1u) == 0)) {
            SDL_Rect warning = {65, 500, 670, 58};
            SDL_RenderCopy(a->renderer, a->battle_warning_texture, NULL, &warning);
        } else {
            /* Mode 1 blits a black 670x58 page during the off phase. */
            SDL_SetRenderDrawColor(a->renderer, 0, 0, 0, 255);
            SDL_Rect blank = {65, 500, 670, 58};
            SDL_RenderFillRect(a->renderer, &blank);
        }
    }
    if (a->stage == SIMULATION_BATTLE || a->stage == SIMULATION_MONTAGE) {
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == SIMULATION_TACTICAL) {
        SDL_Color cyan = {131, 237, 240, 255};
        for (int row = 0; row < 37; ++row) {
            char text[301];
            int length = 100;
            while (length > 0 && a->tactical_cells[row][length - 1] == ' ') --length;
            if (!length) continue;
            size_t n = 0;
            for (int column = 0; column < length; ++column) {
                if ((unsigned char)a->tactical_cells[row][column] == 0xdf) {
                    memcpy(text + n, "\xe2\x96\x80", 3); /* CP437 upper half block. */
                    n += 3;
                } else text[n++] = a->tactical_cells[row][column];
            }
            text[n] = 0;
            /* Console rows never word-wrap at the dialogue renderer's 740 px limit. */
            SDL_Surface *surface = TTF_RenderUTF8_Solid(a->font, text, cyan);
            if (!surface) continue;
            SDL_Texture *texture = SDL_CreateTextureFromSurface(a->renderer, surface);
            SDL_Rect dst = {0, row * 16, surface->w, surface->h};
            if (texture) {
                SDL_RenderCopy(a->renderer, texture, NULL, &dst);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
        if (a->tactical_caption_visible && a->caption_texture) {
            SDL_Rect caption = {147, 530, 505, 40};
            SDL_RenderCopy(a->renderer, a->caption_texture, NULL, &caption);
        }
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == STORY_CAPTION) {
        if (a->caption_program == CAPTIONS_AFTER_SIMULATION ||
            a->caption_program == CAPTIONS_POST_HACK) {
            /* These FUN_0040e470 modes draw the supplied 505x40 card natively. */
            int y = a->caption_program == CAPTIONS_POST_HACK || a->caption_index < 2
                ? 280 : 530;
            SDL_Rect caption = {147, y, 505, 40};
            if (a->caption_texture) SDL_RenderCopy(a->renderer, a->caption_texture, NULL, &caption);
            SDL_RenderPresent(a->renderer);
            return;
        }
        SDL_SetRenderDrawBlendMode(a->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(a->renderer, 0, 0, 0, 145);
        SDL_Rect strip = {0, 466, W, 110};
        SDL_RenderFillRect(a->renderer, &strip);
        if (a->caption_texture && a->caption_width > 0) {
            float scale = 740.0f / (float)a->caption_width;
            int width = 740;
            int height = (int)(a->caption_height * scale);
            SDL_Rect caption = {(W - width) / 2, 484, width, height};
            SDL_RenderCopy(a->renderer, a->caption_texture, NULL, &caption);
        }
        draw_text(a, "Press Enter to continue", 300, 556, (SDL_Color){105, 255, 184, 255});
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == TIC_TAC_TOE || a->stage == TIC_MARK ||
        a->stage == TIC_MOVE || a->stage == TIC_ANIMATION || a->stage == TIC_AGAIN) {
        SDL_Color cyan = {131, 237, 240, 255};
        /* FUN_0040b590 draws a 3x3 board at source coordinates 200..600 x 0..300,
           with two-pixel DirectDraw strokes and the labels 1 through 9. */
        SDL_SetRenderDrawColor(a->renderer, cyan.r, cyan.g, cyan.b, 255);
        for (int x = 320; x <= 480; x += 160) {
            SDL_RenderDrawLine(a->renderer, x, 0, x, 300);
            SDL_RenderDrawLine(a->renderer, x + 1, 0, x + 1, 300);
        }
        for (int y = 100; y <= 200; y += 100) {
            SDL_RenderDrawLine(a->renderer, 200, y, 600, y);
            SDL_RenderDrawLine(a->renderer, 200, y + 1, 600, y + 1);
        }
        for (int i = 0; i < 9; ++i) {
            int col = i % 3, row = i / 3;
            int cell_x = 200 + col * 400 / 3;
            int cell_y = row * 100;
            char label[2] = {(char)('1' + i), 0};
            if (a->tic_board[i]) {
                draw_tic_mark(a, i, a->tic_board[i], cyan);
            } else {
                draw_text(a, label, cell_x + 62, cell_y + 42, cyan);
            }
        }
        if (a->stage == TIC_TAC_TOE) {
            draw_text(a, "ONE OR TWO PLAYERS?", 16, 356, cyan);
            draw_text(a, "PLEASE ENTER NUMBER OF PLAYERS.", 16, 380, cyan);
        } else if (a->stage == TIC_MARK) {
            draw_text(a, "X or O?", 16, 356, cyan);
        } else if (a->stage == TIC_MOVE || a->stage == TIC_ANIMATION) {
            draw_text(a, "PLEASE SELECT A NUMBER:", 16, 356, cyan);
        } else {
            draw_text(a, "STALEMATE.", 16, 356, cyan);
            draw_text(a, "WANT TO PLAY AGAIN?", 16, 380, cyan);
        }
        char prompt[LINE_LEN + 4];
        snprintf(prompt, sizeof(prompt), "%s_", a->input);
        draw_text(a, prompt, 16, a->stage == TIC_TAC_TOE || a->stage == TIC_AGAIN ? 404 : 380, cyan);
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == GAME_SHELL) {
        SDL_Color cyan = {131, 237, 240, 255};
        draw_text(a, "LOGON:", 16, 356, cyan);
        char prompt[LINE_LEN + 4];
        snprintf(prompt, sizeof(prompt), "%s_", a->input);
        draw_text(a, prompt, 16, 380, cyan);
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == ENDING_PRELUDE) {
        SDL_RenderPresent(a->renderer);
        return;
    }
    if (a->stage == FINAL_GREETING || a->stage == ENDING ||
        a->stage == ENDING_CHESS || a->stage == ENDING_MUSIC) {
        SDL_Color cyan = {131, 237, 240, 255};
        if (a->stage == FINAL_GREETING)
            draw_text(a, "GREETINGS PROFESSOR FALKEN", 16, 356, cyan);
        else if (a->stage == ENDING) {
            draw_text(a, "A STRANGE GAME.", 16, 356, cyan);
            draw_text(a, "THE ONLY WINNING MOVE IS", 16, 380, cyan);
            draw_text(a, "NOT TO PLAY.", 16, 404, cyan);
        }
        else if (a->stage == ENDING_CHESS)
            draw_text(a, "HOW ABOUT A NICE GAME OF CHESS?", 16, 356, cyan);
        SDL_RenderPresent(a->renderer);
        return;
    }
    SDL_Color green = {131, 237, 240, 255};
    int first = a->line_count > 18 ? a->line_count - 18 : 0;
    int y = 42;
    for (int i = first; i < a->line_count; i++) {
        draw_text(a, a->lines[i], 16, y, green);
        y += 22;
        if (y > 430) break;
    }
    char prompt[LINE_LEN + 8];
    snprintf(prompt, sizeof(prompt), "%s_", a->input);
    draw_text(a, prompt, 8, y + 6, green);
    SDL_RenderPresent(a->renderer);
}

/* Set up SDL, run the event loop, and release every audio/video/graphics resource. */
int main(void) {
    App a = {0};
    a.running = 1;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0 || TTF_Init() != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError()); return 1;
    }
    select_default_x11_visual();
    a.window = SDL_CreateWindow("Wargames - C reconstruction", 0, 0, W, H,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    /* The X11 accelerated renderer selected a DirectColor visual on Wine/Xvfb,
       which changed the terminal colors. The software renderer keeps the
       original cyan palette stable and is fast enough for this 800x600 UI. */
    a.renderer = SDL_CreateRenderer(a.window, -1, SDL_RENDERER_SOFTWARE);
    if (!a.renderer) a.renderer = SDL_CreateRenderer(a.window, -1, SDL_RENDERER_ACCELERATED);
    a.font = open_font();
    if (!a.window || !a.renderer || !a.font) {
        fprintf(stderr, "Window or font initialization failed: %s\n", SDL_GetError()); return 1;
    }
    media_audio_init(&a.audio);
    SDL_StartTextInput();
    set_background(&a, "Warintsc");
    SDL_Surface *banner_surface = SDL_LoadBMP("../Gfx/Warints2.dat");
    if (banner_surface) {
        a.intro_banner = SDL_CreateTextureFromSurface(a.renderer, banner_surface);
        SDL_FreeSurface(banner_surface);
    }
    SDL_Surface *warning_surface = SDL_LoadBMP("../Gfx/Warmitxt.dat");
    if (warning_surface) {
        a.battle_warning_texture = SDL_CreateTextureFromSurface(a.renderer, warning_surface);
        SDL_FreeSurface(warning_surface);
    }
    /* A QA-only entry point lets the replay tool compare later branches
       without waiting through the several-minute carrier scan each run. */
    const char *compare_start = getenv("WARGAME_COMPARE_START");
    if (compare_start && strcmp(compare_start, "carriers") == 0) {
        start_stage(&a, DIAL_NUMBERS, "");
    } else if (compare_start && strcmp(compare_start, "fbi-tail") == 0) {
        start_caption_sequence(&a, CAPTIONS_FBI_TAIL, NORAD_PRELUDE, "Wargisld");
    } else if (compare_start && strcmp(compare_start, "norad") == 0) {
        start_caption_sequence(&a, CAPTIONS_NORAD_12_23, NORAD_CLIP_2, "Wargisld");
    } else if (compare_start && strcmp(compare_start, "modem") == 0) {
        start_stage(&a, MODEM_CONNECT, NULL);
    } else if (compare_start && strcmp(compare_start, "montage") == 0) {
        start_simulation_montage(&a);
    } else if (compare_start && strcmp(compare_start, "post-sim") == 0) {
        start_caption_sequence(&a, CAPTIONS_AFTER_SIMULATION, SIMULATION_MONTAGE, "War2400");
    } else if (compare_start && strcmp(compare_start, "post-hack") == 0) {
        start_caption_sequence(&a, CAPTIONS_POST_HACK, MODEM_CONNECT, "Waronews");
    } else if (compare_start && strcmp(compare_start, "simulation-map") == 0) {
        start_caption_sequence(&a, CAPTIONS_AFTER_SIMULATION, SIMULATION_MONTAGE, "War2400");
    } else if (compare_start && strcmp(compare_start, "simulation-battle") == 0) {
        start_simulation_battle(&a);
    } else if (compare_start && strcmp(compare_start, "simulation-overview") == 0) {
        start_simulation_overview(&a);
    } else if (compare_start && strcmp(compare_start, "simulation-tactical") == 0) {
        start_simulation_tactical(&a);
    } else if (compare_start && strcmp(compare_start, "simulation-warning") == 0) {
        a.stage = SIMULATION_BATTLE;
        a.battle_index = 1;
        a.battle_warning_active = 2;
        a.battle_warning_started = SDL_GetTicks();
        a.battle_sound_channel = media_sound_loop_start(&a.audio, "Warmiswn");
        set_background(&a, "Warmiwn2");
        a.battle_deadline = SDL_GetTicks() + 60000u;
    } else if (compare_start && strcmp(compare_start, "tic-tac-toe") == 0) {
        memset(a.tic_board, 0, sizeof(a.tic_board));
        start_stage(&a, TIC_TAC_TOE, "");
    } else if (compare_start && strcmp(compare_start, "real-or-game") == 0) {
        start_stage(&a, REAL_OR_GAME, "");
    } else if (compare_start && strcmp(compare_start, "still-playing") == 0) {
        start_stage(&a, STILL_PLAYING, "");
    } else if (compare_start && strcmp(compare_start, "ending") == 0) {
        start_stage(&a, ENDING, "");
    } else if (compare_start && strcmp(compare_start, "final-logon") == 0) {
        start_stage(&a, LOGON_FINAL, "War2400");
    } else if (compare_start && strcmp(compare_start, "game-list") == 0) {
        start_stage(&a, LIST_GAMES, "Wargenbg");
    } else if (compare_start && strcmp(compare_start, "phone-scan") == 0) {
        start_stage(&a, PHONE, "");
    } else if (compare_start && strcmp(compare_start, "intro") == 0) {
        start_intro(&a);
    } else {
        if (has_completed_carrier_scan()) {
            start_stage(&a, DIAL_NUMBERS, "");
        } else {
            start_intro(&a);
        }
    }
    SDL_Event event;
    while (a.running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) a.running = 0;
            else if (event.type == SDL_KEYDOWN) {
                if (a.stage == LIST_GAMES && a.game_list_waiting &&
                    event.key.keysym.sym != SDLK_ESCAPE) {
                    continue;
                }
                if ((a.stage == SIMULATION_OVERVIEW || a.stage == SIDE || a.stage == CITIES) &&
                    !a.overview_ready && event.key.keysym.sym != SDLK_ESCAPE) {
                    continue;
                }
                if (a.stage == SIMULATION_TACTICAL && event.key.keysym.sym == SDLK_s) {
                    a.tactical_exit_requested = 1;
                    if (!a.tactical_deadline) a.tactical_deadline = SDL_GetTicks();
                }
                else if (a.stage == DIAL_MENU) {
                    a.input[0] = 0;
                    a.ignore_next_menu_text = event.key.keysym.sym >= SDLK_SPACE &&
                                              event.key.keysym.sym <= SDLK_z;
                    start_stage(&a, DIAL_DESTINATION, NULL);
                }
                else if (a.stage == DIAL_TICKET) {
                    a.ignore_next_menu_text = event.key.keysym.sym >= SDLK_SPACE &&
                                              event.key.keysym.sym <= SDLK_z;
                    start_stage(&a, DIAL_NUMBERS, "");
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (a.stage == SCANNING) submit(&a);
                    else if (a.stage == BANK_PROMPT) {
                        if (SDL_GetTicks() - a.stage_started >= 480u)
                            start_stage(&a, DIAL_NUMBERS, "");
                        else a.bank_key_pending = 1;
                    }
                    else if (a.stage == DIAL_NUMBERS && a.bank_connecting)
                        a.bank_key_pending = 1;
                    else a.running = 0;
                }
                else if (a.stage == BANK_PROMPT) {
                    if (SDL_GetTicks() - a.stage_started >= 480u)
                        start_stage(&a, DIAL_NUMBERS, "");
                    else a.bank_key_pending = 1;
                }
                else if (a.stage == DIAL_NUMBERS && a.bank_connecting)
                    a.bank_key_pending = 1;
                else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (a.stage != PHONE && a.stage != SCANNING && a.stage != BANK_PROMPT)
                        media_enter_key(&a.audio);
                    if (media_video_active(&a.video)) finish_current_video(&a);
                    else if (a.stage == STORY_CAPTION) advance_story_caption(&a);
                    else if (a.stage == INTRO) start_stage(&a, PHONE, "");
                    else submit(&a);
                }
                else if (event.key.keysym.sym == SDLK_SPACE && a.stage == STORY_CAPTION) advance_story_caption(&a);
                else if (event.key.keysym.sym == SDLK_m && a.stage == MODEM_CONNECT) {
                    media_stop_voice(&a.audio);
                    media_sound(&a.audio, "Wargtr");
                    start_stage(&a, GREETING3, "Warpswde");
                }
                else if (a.stage == DIAL_NUMBERS && event.key.keysym.sym == SDLK_DOWN) {
                    a.carrier_selection = a.carrier_selection < 15 ? a.carrier_selection + 1 : 7;
                    a.carrier_navigated = 1;
                    refresh_carrier_selection(&a);
                }
                else if (a.stage == DIAL_NUMBERS && event.key.keysym.sym == SDLK_UP) {
                    a.carrier_selection = a.carrier_selection > 7 ? a.carrier_selection - 1 : 15;
                    a.carrier_navigated = 1;
                    refresh_carrier_selection(&a);
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    size_t n = strlen(a.input); if (n) a.input[n - 1] = 0;
                }
            } else if (event.type == SDL_TEXTINPUT) {
                if (a.stage == LIST_GAMES && a.game_list_waiting) continue;
                if ((a.stage == SIMULATION_OVERVIEW || a.stage == SIDE || a.stage == CITIES) &&
                    !a.overview_ready) {
                    continue;
                }
                if (a.ignore_next_menu_text) {
                    a.ignore_next_menu_text = 0;
                } else if (a.stage == PHONE) {
                    static const char required_scan_digits[] = "311399311437311767311936";
                    for (size_t i = 0; event.text.text[i] && a.stage == PHONE; ++i) {
                        char key = event.text.text[i];
                        if (a.scan_digit_index < sizeof(required_scan_digits) - 1 &&
                            key == required_scan_digits[a.scan_digit_index]) {
                            a.scan_digits[a.scan_digit_index++] = key;
                            a.scan_digits[a.scan_digit_index] = 0;
                            media_type_key(&a.audio);
                            if (a.scan_digit_index == sizeof(required_scan_digits) - 1) {
                                start_stage(&a, SCANNING, "");
                                a.scan_started = SDL_GetTicks();
                                media_voice(&a.audio, "Warhello");
                                a.scan_sfm_played = 0;
                                a.scan_caption_started = 0;
                                a.scan_dynamic_started = 0;
                                a.scan_dynamic_count = 0;
                            }
                        }
                    }
                } else if (a.stage == DIAL_NUMBERS && a.bank_connecting) {
                    a.bank_key_pending = 1;
                } else if (a.stage != BANK_PROMPT && a.stage != DIAL_NUMBERS) {
                    size_t n = strlen(a.input), m = strlen(event.text.text);
                    if (n + m < sizeof(a.input) - 1) {
                        strcat(a.input, event.text.text);
                        media_type_key(&a.audio);
                        /* The numeric side selector reads a key, not a line:
                           2 starts the confirmation without waiting for Return. */
                        if (a.stage == SIDE) submit(&a);
                    }
                }
            }
        }
        if (a.stage == DIAL_NUMBERS && a.bank_connecting &&
            SDL_GetTicks() - a.bank_connect_started >= 800u) {
            a.bank_connecting = 0;
            start_stage(&a, BANK_PROMPT, "");
        }
        if (a.stage == BANK_PROMPT) update_bank_typewriter_audio(&a);
        if (a.stage == BANK_PROMPT && a.bank_key_pending &&
            SDL_GetTicks() - a.stage_started >= 480u)
            start_stage(&a, DIAL_NUMBERS, "");
        if (media_video_active(&a.video)) {
            if (media_video_update(&a.video)) finish_current_video(&a);
        }
        if (a.stage == LIST_GAMES && a.game_list_waiting &&
            (int32_t)(SDL_GetTicks() - a.game_list_deadline) >= 0) {
            a.game_list_waiting = 0;
            stage_prompt(&a);
        }
        if (a.stage == SIMULATION_MONTAGE && a.montage_deadline != 0 &&
            (int32_t)(SDL_GetTicks() - a.montage_deadline) >= 0)
            advance_simulation_montage(&a);
        if (a.stage == SIMULATION_BATTLE && a.battle_deadline != 0 &&
            (int32_t)(SDL_GetTicks() - a.battle_deadline) >= 0)
            advance_simulation_battle(&a);
        if (a.stage == SIMULATION_OVERVIEW || a.stage == SIDE || a.stage == CITIES)
            update_simulation_overview(&a);
        if (a.stage == SIMULATION_ACCEPTED &&
            (int32_t)(SDL_GetTicks() - a.overview_deadline) >= 0)
            start_simulation_overview(&a);
        if (a.stage == SIMULATION_TACTICAL && a.tactical_ready && a.tactical_deadline != 0 &&
            ((int32_t)(SDL_GetTicks() - a.tactical_deadline) >= 0 ||
             (a.tactical_compare_frame >= 16 && a.tactical_compare_frame <= 250 &&
              a.tactical_compare_phase >= 0 && a.tactical_compare_phase <= 4 &&
              getenv("WARGAME_COMPARE_TACTICAL_FAST")))) {
            if (a.tactical_frame == a.tactical_compare_frame &&
                a.tactical_phase == a.tactical_compare_phase && !a.tactical_exit_requested) {
                media_sound_stop(&a.audio, a.tactical_audio_channel);
                a.tactical_deadline = 0;
            } else advance_simulation_tactical(&a);
        }
        if (a.stage == TIC_ANIMATION) advance_tic_animation(&a);
        if (a.stage == SIMULATION_WARMUP && a.caption_deadline != 0 &&
            (int32_t)(SDL_GetTicks() - a.caption_deadline) >= 0) {
            start_caption_sequence(&a, a.caption_program, a.after_caption_stage,
                                   a.after_caption_background[0]
                                       ? a.after_caption_background : NULL);
        }
        if (a.stage == STORY_CAPTION && a.caption_deadline != 0 &&
            (int32_t)(SDL_GetTicks() - a.caption_deadline) >= 0)
            advance_story_caption(&a);
        if (a.stage == LOGON_REJECTED) {
            uint32_t total = (uint32_t)(strlen("CONNECTION NOT RECOGNIZED BY SYSTEM .....") +
                                        strlen("-- CONNECTION TERMINATED -- ")) * 30u + 3000u;
            if (SDL_GetTicks() - a.stage_started >= total) a.running = 0;
        }
        if (a.play_prompt_pending &&
            (int32_t)(SDL_GetTicks() - a.play_prompt_deadline) >= 0) {
            push_line(&a, "SHALL WE PLAY A GAME?");
            a.play_prompt_pending = 0;
        }
        if (a.deferred_voice[0] && (int32_t)(SDL_GetTicks() - a.deferred_voice_deadline) >= 0) {
            media_voice(&a.audio, a.deferred_voice);
            a.deferred_voice[0] = 0;
        }
        if (a.stage == INTRO && (int32_t)(SDL_GetTicks() - a.intro_started) >= 19000)
            start_stage(&a, PHONE, "");
        if (a.stage == SCANNING) {
            uint32_t scan_elapsed = SDL_GetTicks() - a.scan_started;
            uint32_t now = SDL_GetTicks();
            if (scan_elapsed >= 6140u && !a.scan_sfm_played) {
                media_voice(&a.audio, "Warsfm");
                a.scan_sfm_played = 1;
            }
            if (scan_elapsed >= 13585u && !a.scan_caption_started) {
                set_story_caption(&a, "Wartxt1");
                a.scan_caption_started = 1;
            }
            if (scan_elapsed >= 16000u && !a.scan_dynamic_started) {
                a.scan_dynamic_started = 1;
                a.scan_dynamic_count = 1; /* 767-9995 follows the first three scans. */
                media_sound(&a.audio, "Wargtr");
                a.scan_dynamic_deadline = now + (uint32_t)floor(game_rng_unit() * 7000.0) + 3000u;
            } else if (a.scan_dynamic_started && a.scan_dynamic_count < 31 &&
                       (int32_t)(now - a.scan_dynamic_deadline) >= 0) {
                a.scan_dynamic_count++;
                media_sound(&a.audio, "Wargtr");
                a.scan_dynamic_deadline = now + (uint32_t)floor(game_rng_unit() * 7000.0) + 3000u;
            } else if (a.scan_dynamic_started && a.scan_dynamic_count == 31 &&
                       (int32_t)(now - a.scan_dynamic_deadline) >= 0) {
                /* The original returns to carrier selection after the final scan tone. */
                record_carrier_scan();
                start_stage(&a, DIAL_NUMBERS, "");
            }
        }
        if (a.stage == ENDING_PRELUDE &&
            (int32_t)(SDL_GetTicks() - a.ending_deadline) >= 0)
            start_stage(&a, FINAL_GREETING, NULL);
        if (a.stage == FINAL_GREETING &&
            (int32_t)(SDL_GetTicks() - a.ending_deadline) >= 0)
            start_stage(&a, ENDING, NULL);
        if ((a.stage == ENDING || a.stage == ENDING_CHESS) &&
            (int32_t)(SDL_GetTicks() - a.ending_deadline) >= 0)
            start_stage(&a, a.stage == ENDING ? ENDING_CHESS : ENDING_MUSIC,
                        a.stage == ENDING ? "Wardefc1" : NULL);
        if (media_video_active(&a.video)) media_video_draw(&a.video, a.renderer);
        else render(&a);
    }
    SDL_StopTextInput();
    media_video_stop(&a.video);
    media_audio_close(&a.audio);
    if (a.background) SDL_DestroyTexture(a.background);
    if (a.intro_banner) SDL_DestroyTexture(a.intro_banner);
    if (a.caption_texture) SDL_DestroyTexture(a.caption_texture);
    if (a.battle_warning_texture) SDL_DestroyTexture(a.battle_warning_texture);
    if (a.font) TTF_CloseFont(a.font);
    SDL_DestroyRenderer(a.renderer); SDL_DestroyWindow(a.window);
    TTF_Quit(); SDL_Quit();
    return 0;
}
