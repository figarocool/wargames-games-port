#ifndef WARGAME_EXTENDED_H
#define WARGAME_EXTENDED_H

#include <SDL.h>
#include <stdint.h>
#include <stddef.h>

typedef struct App App;

#define STAGE_DIAL_NUMBERS 6
#define STAGE_PLAY         19
#define STAGE_LIST_GAMES   42

/* Bridge helpers implemented in main.c */
void app_draw_text(App *a, const char *text, int x, int y, SDL_Color color);
void app_draw_block_cursor(App *a, int x, int y, SDL_Color color);
void app_start_stage(App *a, int stage, const char *background);
void app_play_sound(App *a, const char *name);
int app_get_lang(const App *a);
const char *app_get_input(const App *a);
void app_clear_input(App *a);
void app_present_frame(App *a);

/* Extended Banking API */
void ext_bank_init(App *a);
void ext_bank_submit(App *a, const char *input);
void ext_bank_update(App *a);
void ext_bank_render(App *a);

/* Extended Pan Am API */
void ext_panam_init(App *a);
void ext_panam_submit(App *a, const char *input);
void ext_panam_render(App *a);

/* Extended Chess API */
void ext_chess_init(App *a);
void ext_chess_submit(App *a, const char *input);
void ext_chess_render(App *a);

/* Extended Falken Maze API */
void ext_maze_init(App *a);
void ext_maze_key(App *a, SDL_Keycode key);
void ext_maze_submit(App *a, const char *input);
void ext_maze_render(App *a);

/* Extended Cards API (Blackjack & Poker) */
void ext_cards_init(App *a, int is_poker);
void ext_cards_submit(App *a, const char *input);
void ext_cards_render(App *a);

#endif /* WARGAME_EXTENDED_H */
