#define _POSIX_C_SOURCE 200809L
#include "media.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define R_OK 4
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

enum { VIDEO_WIDTH = 800, VIDEO_HEIGHT = 592, VIDEO_RATE = 25, AUDIO_RATE = 48000 };

const char *media_asset_prefix(void) {
    if (access("Snd", F_OK) == 0) return "";
    if (access("../Snd", F_OK) == 0) return "../";
    return "";
}

static const char *const effect_names[19] = {
    "Waracdnd", "Warcords", "Wargrr", "Wargtr", "Warhtent", "Warlot1",
    "Warlstgm", "Warmiswn", "Warnocmp", "Warottt", "Warstwt", "Warswof2",
    "Warswoff", "Wartttsc", "Wartttts", "Wartxtsd", "Wartxtsdnew", "Warxor0", "Warxttt"
};

/* Resolve a sound effect filename without requiring installation or data extraction. */
static const char *sound_path(const char *name) {
    static char path[256];
    snprintf(path, sizeof(path), "%sSnd/%s.dat", media_asset_prefix(), name);
    return path;
}

/* Load one local PCM sound effect. */
static Mix_Chunk *load_effect(const char *name) {
    return Mix_LoadWAV(sound_path(name));
}

/* Initialize SDL_mixer and cache the supplied PCM effects and phone-ring MP3. */
int media_audio_init(GameAudio *audio) {
    memset(audio, 0, sizeof(*audio));
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return 0;
    Mix_Init(MIX_INIT_MP3);
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048) != 0) return 0;
    for (size_t i = 0; i < sizeof(effect_names) / sizeof(effect_names[0]); i++)
        audio->effects[i] = load_effect(effect_names[i]);
    char ring_path[256];
    snprintf(ring_path, sizeof(ring_path), "%sSnd/Warphrng.dat", media_asset_prefix());
    SDL_RWops *ring_stream = SDL_RWFromFile(ring_path, "rb");
    if (ring_stream) audio->phone_ring = Mix_LoadMUSType_RW(ring_stream, MUS_MP3, 1);
    audio->ready = 1;
    return 1;
}

/* Stop and release the current WOPR voice/music stream. */
void media_stop_voice(GameAudio *audio) {
    if (!audio || !audio->ready) return;
    Mix_HaltMusic();
    if (audio->voice) Mix_FreeMusic(audio->voice);
    audio->voice = NULL;
}

/* Spoken terminal prompts finish before the next line is printed. */
int media_voice_active(const GameAudio *audio) {
    return audio && audio->ready && audio->voice && Mix_PlayingMusic();
}

/* Release all mixer resources before SDL shuts down. */
void media_audio_close(GameAudio *audio) {
    if (!audio) return;
    media_stop_voice(audio);
    if (audio->phone_ring) Mix_FreeMusic(audio->phone_ring);
    for (size_t i = 0; i < sizeof(audio->effects) / sizeof(audio->effects[0]); i++)
        if (audio->effects[i]) Mix_FreeChunk(audio->effects[i]);
    if (audio->ready) Mix_CloseAudio();
    Mix_Quit();
    memset(audio, 0, sizeof(*audio));
}

void media_set_language(GameAudio *audio, int lang) {
    if (audio) audio->lang = lang;
}

/* Play a named MP3 voice clip from the game's Wopr asset folder. */
void media_voice(GameAudio *audio, const char *asset_name) {
    if (!audio || !audio->ready || !asset_name) return;
    media_stop_voice(audio);
    char path[256];
    if (audio->lang == 1) {
        snprintf(path, sizeof(path), "%sWopr_it/%s.dat", media_asset_prefix(), asset_name);
        if (access(path, R_OK) != 0) {
            snprintf(path, sizeof(path), "%sWopr/%s.dat", media_asset_prefix(), asset_name);
        }
    } else {
        snprintf(path, sizeof(path), "%sWopr/%s.dat", media_asset_prefix(), asset_name);
    }
    SDL_RWops *stream = SDL_RWFromFile(path, "rb");
    if (!stream) return;
    audio->voice = Mix_LoadMUSType_RW(stream, MUS_MP3, 1);
    if (audio->voice) Mix_PlayMusic(audio->voice, 0);
}

/* Play the original typewriter sound on key input. */
void media_type_key(GameAudio *audio) {
    if (!audio || !audio->ready) return;
    media_sound(audio, (audio->key_counter++ & 1u) ? "Wartxtsdnew" : "Wartxtsd");
}

/* The original loads the automatic terminal-writing sample with looping on. */
void media_typewriter_start(GameAudio *audio) {
    (void)media_sound_loop_start(audio, "Wartxtsd");
}

/* Play the original return-key sound on command submission. */
void media_enter_key(GameAudio *audio) {
    media_sound(audio, "Warhtent");
}

/* Play a dial tone or a successful modem connection cue. */
void media_dial(GameAudio *audio, int connected) {
    if (!audio || !audio->ready) return;
    media_sound(audio, connected ? "Warnocmp" : "Warcords");
}

/* Play one cached effect by its original asset filename. */
void media_sound(GameAudio *audio, const char *asset_name) {
    (void)media_sound_start(audio, asset_name);
}

/* The original routes its PCM effects through FMOD channel zero. */
static int play_effect(GameAudio *audio, const char *asset_name, int loops) {
    if (!audio || !audio->ready || !asset_name) return -1;
    for (size_t i = 0; i < sizeof(effect_names) / sizeof(effect_names[0]); i++) {
        if (strcmp(effect_names[i], asset_name) == 0 && audio->effects[i]) {
            return Mix_PlayChannel(0, audio->effects[i], loops);
        }
    }
    return -1;
}

int media_sound_start(GameAudio *audio, const char *asset_name) {
    return play_effect(audio, asset_name, 0);
}

int media_sound_loop_start(GameAudio *audio, const char *asset_name) {
    return play_effect(audio, asset_name, -1);
}

void media_sound_stop(GameAudio *audio, int channel) {
    if (audio && audio->ready && channel >= 0) Mix_HaltChannel(channel);
}

/* Play the ring heard when David's modem receives a call. */
void media_phone_ring(GameAudio *audio) {
    if (!audio || !audio->ready || !audio->phone_ring) return;
    media_stop_voice(audio);
    Mix_PlayMusic(audio->phone_ring, 0);
}

/* Close a file descriptor once and mark it as unavailable. */
static void close_fd(int *fd) {
    if (*fd >= 0) close(*fd);
    *fd = -1;
}

/* Close both FFmpeg output streams. */
static void close_child_pipes(MediaPlayer *player) {
    close_fd(&player->video_pipe);
    close_fd(&player->audio_pipe);
}

#ifdef _WIN32
int media_video_start(MediaPlayer *player, SDL_Renderer *renderer, const char *asset_name) {
    (void)renderer; (void)asset_name;
    memset(player, 0, sizeof(*player));
    player->video_pipe = player->audio_pipe = -1;
    player->failed = 1;
    return 0;
}

int media_video_update(MediaPlayer *player) {
    (void)player;
    return 1;
}

void media_video_draw(MediaPlayer *player, SDL_Renderer *renderer) {
    (void)player; (void)renderer;
}

void media_video_stop(MediaPlayer *player) {
    if (!player) return;
    player->decoder_pid = -1;
    close_child_pipes(player);
}
#else
/* Make a decoder output pipe non-blocking so SDL keeps responding to input. */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

/* Fork FFmpeg to decode a game MPEG into raw video frames and PCM audio. */
int media_video_start(MediaPlayer *player, SDL_Renderer *renderer, const char *asset_name) {
    (void)renderer;
    memset(player, 0, sizeof(*player));
    player->video_pipe = player->audio_pipe = -1;
    player->frame_size = (size_t)VIDEO_WIDTH * VIDEO_HEIGHT * 3;
    player->frame_pixels = malloc(player->frame_size);
    player->frame_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                               SDL_TEXTUREACCESS_STREAMING, VIDEO_WIDTH, VIDEO_HEIGHT);
    if (!player->frame_pixels || !player->frame_texture) {
        player->failed = 1;
        media_video_stop(player);
        return 0;
    }

    char path[256];
    snprintf(path, sizeof(path), "%sVid/%s.dat", media_asset_prefix(), asset_name);
    int video_fds[2], audio_fds[2];
    if (pipe(video_fds) != 0) {
        player->failed = 1;
        media_video_stop(player);
        return 0;
    }
    if (pipe(audio_fds) != 0) {
        close(video_fds[0]); close(video_fds[1]);
        player->failed = 1;
        media_video_stop(player);
        return 0;
    }
    pid_t child = fork();
    if (child < 0) {
        close(video_fds[0]); close(video_fds[1]);
        close(audio_fds[0]); close(audio_fds[1]);
        player->failed = 1;
        media_video_stop(player);
        return 0;
    }
    if (child == 0) {
        dup2(video_fds[1], STDOUT_FILENO);
        dup2(audio_fds[1], 100);
        int null_device = open("/dev/null", O_WRONLY);
        if (null_device >= 0) dup2(null_device, STDERR_FILENO);
        int descriptors[] = {video_fds[0], video_fds[1], audio_fds[0], audio_fds[1], null_device};
        for (size_t i = 0; i < sizeof(descriptors) / sizeof(descriptors[0]); i++)
            if (descriptors[i] >= 0 && descriptors[i] != STDOUT_FILENO && descriptors[i] != STDERR_FILENO && descriptors[i] != 100)
                close(descriptors[i]);
        execlp("ffmpeg", "ffmpeg", "-nostdin", "-loglevel", "error", "-i", path,
               "-map", "0:v:0", "-an", "-f", "rawvideo", "-pix_fmt", "rgb24", "-fps_mode", "passthrough", "pipe:1",
               "-map", "0:a:0", "-vn", "-f", "s16le", "-acodec", "pcm_s16le", "-ar", "48000", "-ac", "2", "pipe:100",
               (char *)NULL);
        _exit(127);
    }
    close(video_fds[1]); close(audio_fds[1]);
    player->decoder_pid = child;
    player->video_pipe = video_fds[0];
    player->audio_pipe = audio_fds[0];
    if (!set_nonblocking(player->video_pipe) || !set_nonblocking(player->audio_pipe)) {
        player->failed = 1;
        media_video_stop(player);
        return 0;
    }

    SDL_AudioSpec wanted = {0}, obtained = {0};
    wanted.freq = AUDIO_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 2048;
    player->audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    if (player->audio_device && obtained.freq == AUDIO_RATE && obtained.channels == 2 && obtained.format == AUDIO_S16SYS)
        SDL_PauseAudioDevice(player->audio_device, 0);
    else if (player->audio_device) {
        SDL_CloseAudioDevice(player->audio_device);
        player->audio_device = 0;
    }
    player->next_frame_at = SDL_GetTicks();
    return 1;
}

/* Feed decoded PCM into SDL's audio queue without allowing excessive delay. */
static void fill_audio(MediaPlayer *player) {
    if (player->audio_pipe < 0) return;
    uint8_t buffer[8192];
    while (!player->audio_eof) {
        if (player->audio_device && SDL_GetQueuedAudioSize(player->audio_device) > AUDIO_RATE / 2 * 4) return;
        ssize_t count = read(player->audio_pipe, buffer, sizeof(buffer));
        if (count > 0) {
            if (player->audio_device) SDL_QueueAudio(player->audio_device, buffer, (uint32_t)count);
            continue;
        }
        if (count == 0) { player->audio_eof = 1; close_fd(&player->audio_pipe); }
        else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            player->audio_eof = 1;
            close_fd(&player->audio_pipe);
        }
        break;
    }
}

/* Poll decoder output, present a frame at 25 fps, and detect end of playback. */
int media_video_update(MediaPlayer *player) {
    if (!player || player->failed || player->decoder_pid <= 0) return 1;
    fill_audio(player);
    uint32_t now = SDL_GetTicks();
    if (!player->video_eof && (int32_t)(now - player->next_frame_at) >= 0) {
        while (player->frame_used < player->frame_size) {
            ssize_t count = read(player->video_pipe, player->frame_pixels + player->frame_used,
                                 player->frame_size - player->frame_used);
            if (count > 0) player->frame_used += (size_t)count;
            else if (count == 0) { player->video_eof = 1; close_fd(&player->video_pipe); break; }
            else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) break;
            else { player->video_eof = 1; close_fd(&player->video_pipe); break; }
        }
        if (player->frame_used == player->frame_size) {
            SDL_UpdateTexture(player->frame_texture, NULL, player->frame_pixels, VIDEO_WIDTH * 3);
            player->frame_used = 0;
            player->next_frame_at = now + (1000 / VIDEO_RATE);
        }
    }

    int status;
    pid_t ended = waitpid(player->decoder_pid, &status, WNOHANG);
    if (ended == player->decoder_pid) {
        player->decoder_pid = -1;
        player->video_eof = 1;
        player->audio_eof = 1;
        close_child_pipes(player);
    }
    if (player->video_eof && (!player->audio_device || SDL_GetQueuedAudioSize(player->audio_device) == 0)) {
        media_video_stop(player);
        return 1;
    }
    return 0;
}

/* Scale the original 800x592 video frame into the 800x600 game window. */
void media_video_draw(MediaPlayer *player, SDL_Renderer *renderer) {
    if (!player || !player->frame_texture) return;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect display = {0, 4, VIDEO_WIDTH, VIDEO_HEIGHT};
    SDL_RenderCopy(renderer, player->frame_texture, NULL, &display);
    SDL_RenderPresent(renderer);
}

/* Stop FFmpeg and release its pipes, audio device, frame buffer, and texture. */
void media_video_stop(MediaPlayer *player) {
    if (!player) return;
    if (player->decoder_pid > 0) {
        kill(player->decoder_pid, SIGTERM);
        waitpid(player->decoder_pid, NULL, 0);
    }
    player->decoder_pid = -1;
    close_child_pipes(player);
    if (player->audio_device) {
        SDL_ClearQueuedAudio(player->audio_device);
        SDL_CloseAudioDevice(player->audio_device);
        player->audio_device = 0;
    }
    if (player->frame_texture) SDL_DestroyTexture(player->frame_texture);
    free(player->frame_pixels);
    player->frame_texture = NULL;
    player->frame_pixels = NULL;
    player->frame_used = player->frame_size = 0;
}
#endif

/* Return whether a decoder child is currently active. */
int media_video_active(const MediaPlayer *player) {
    return player && player->decoder_pid > 0;
}
