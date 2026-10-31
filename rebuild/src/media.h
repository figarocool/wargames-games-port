#ifndef WARGAME_MEDIA_H
#define WARGAME_MEDIA_H

#include <SDL.h>
#include <SDL_mixer.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
    pid_t decoder_pid;
    int video_pipe;
    int audio_pipe;
    int video_eof;
    int audio_eof;
    int failed;
    size_t frame_size;
    size_t frame_used;
    uint8_t *frame_pixels;
    SDL_Texture *frame_texture;
    SDL_AudioDeviceID audio_device;
    uint32_t next_frame_at;
} MediaPlayer;

typedef struct {
    Mix_Music *voice;
    Mix_Music *phone_ring;
    Mix_Chunk *effects[19];
    unsigned key_counter;
    int ready;
} GameAudio;

/* Open decoders and the SDL audio output. Audio is optional at runtime. */
int media_audio_init(GameAudio *audio);
void media_audio_close(GameAudio *audio);
void media_voice(GameAudio *audio, const char *asset_name);
void media_stop_voice(GameAudio *audio);
int media_voice_active(const GameAudio *audio);
void media_type_key(GameAudio *audio);
void media_typewriter_start(GameAudio *audio);
void media_enter_key(GameAudio *audio);
void media_dial(GameAudio *audio, int connected);
void media_sound(GameAudio *audio, const char *asset_name);
int media_sound_start(GameAudio *audio, const char *asset_name);
int media_sound_loop_start(GameAudio *audio, const char *asset_name);
void media_sound_stop(GameAudio *audio, int channel);
void media_phone_ring(GameAudio *audio);

/* Start a bundled MPEG clip; frames and sound are decoded by the ffmpeg CLI. */
int media_video_start(MediaPlayer *player, SDL_Renderer *renderer, const char *asset_name);
/* Poll both decoder pipes. Returns nonzero when playback has ended or failed. */
int media_video_update(MediaPlayer *player);
void media_video_draw(MediaPlayer *player, SDL_Renderer *renderer);
void media_video_stop(MediaPlayer *player);
int media_video_active(const MediaPlayer *player);

#endif
