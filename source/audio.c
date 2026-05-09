#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

mpg123_handle *mh = NULL;
ndspWaveBuf waveBuf[2];
u8 *audioBuf = NULL;
bool playing = false, paused = false;
char nowPlayingName[256] = "";
char nowPlayingPath[MAX_PATH] = "";
char nowPlayingArtist[256] = "";
char nowPlayingTitle[256] = "";
char nowPlayingAlbum[256] = "";
off_t trackLen = 0;
PlaybackMode playbackMode = MODE_NORMAL;

#define CHANNEL 0
#define BUF_SAMPLES 4096
#define BUF_SIZE (BUF_SAMPLES * 2 * 2)

static void fillBufferFromMPG(u8 *buf, size_t *outTotal) {
    size_t total = 0;
    while (total < BUF_SIZE) {
        size_t done = 0;
        int ret = mpg123_read(mh, buf + total, BUF_SIZE - total, &done);
        if (ret == MPG123_ERR) break;
        if (done == 0) break;
        total += done;
        if (ret == MPG123_DONE) break;
    }
    *outTotal = total;
}

void stopPlayback(void) {
    if (!playing) return;
    ndspChnReset(CHANNEL);
    if (mh) { mpg123_close(mh); mpg123_delete(mh); mh = NULL; }
    playing = paused = false;
    nowPlayingName[0] = '\0';
    nowPlayingPath[0] = '\0';
    nowPlayingArtist[0] = '\0';
    nowPlayingTitle[0] = '\0';
    nowPlayingAlbum[0] = '\0';
    trackLen = 0;
}

float visualizerAmplitude[16] = {0};

void updateVisualizer(void) {
    if (!playing || paused || !audioBuf) {
        for (int i = 0; i < 16; i++) visualizerAmplitude[i] *= 0.9f;
        return;
    }

    // Read from the active buffer
    u8 *buf = audioBuf; // Simple approximation, read from start of linear buffer
    // In a real implementation, we'd find which half is playing, but for a visualizer,
    // reading recent data is often "good enough" for a 3DS homebrew.
    
    s16 *samples = (s16*)buf;
    int samplesPerBar = BUF_SAMPLES / 16;

    for (int i = 0; i < 16; i++) {
        float max = 0;
        for (int j = 0; j < samplesPerBar; j++) {
            s16 sample = samples[i * samplesPerBar + j];
            float amp = abs(sample) / 32768.0f;
            if (amp > max) max = amp;
        }
        // Smooth transition
        visualizerAmplitude[i] = visualizerAmplitude[i] * 0.6f + max * 0.4f;
    }
}

void startPlayback(const char *path) {
    // If we're starting a new folder, update the playlist queue
    // This is part of the auto-advance fix
    bool newFolder = false;
    char newDirPath[MAX_PATH];
    strncpy(newDirPath, path, MAX_PATH-1);
    char *slash = strrchr(newDirPath, '/');
    if (slash) *slash = '\0';

    char currentPlayingDir[MAX_PATH];
    strncpy(currentPlayingDir, nowPlayingPath, MAX_PATH-1);
    char *slash2 = strrchr(currentPlayingDir, '/');
    if (slash2) *slash2 = '\0';

    if (strcmp(newDirPath, currentPlayingDir) != 0) {
        newFolder = true;
    }

    if (newFolder || playlistCount == 0) {
        playlistCount = 0;
        for (int i = 0; i < entryCount; i++) {
            if (entries[i].type == ENTRY_MP3) {
                playlist[playlistCount++] = entries[i];
            }
        }
    }

    stopPlayback();
    int err;
    mh = mpg123_new(NULL, &err);
    if (!mh) return;
    mpg123_param(mh, MPG123_FLAGS, MPG123_FORCE_STEREO, 0);
    if (mpg123_open(mh, path) != MPG123_OK) { mpg123_delete(mh); mh = NULL; return; }
    long rate; int ch, enc;
    mpg123_getformat(mh, &rate, &ch, &enc);
    trackLen = mpg123_length(mh);
    ndspChnReset(CHANNEL);
    ndspChnSetInterp(CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(CHANNEL, (float)rate);
    ndspChnSetFormat(CHANNEL, NDSP_FORMAT_STEREO_PCM16);
    memset(waveBuf, 0, sizeof(waveBuf));
    for (int i = 0; i < 2; i++) {
        u8 *buf = audioBuf + i * BUF_SIZE;
        size_t total = 0;
        fillBufferFromMPG(buf, &total);
        if (total == 0) break;
        DSP_FlushDataCache(buf, BUF_SIZE);
        waveBuf[i].data_vaddr = buf;
        waveBuf[i].nsamples = total / 4;
        waveBuf[i].looping = false;
        waveBuf[i].status = 0;
        ndspChnWaveBufAdd(CHANNEL, &waveBuf[i]);
    }
    playing = true; paused = false;
    strncpy(nowPlayingPath, path, MAX_PATH - 1);
    const char *n = strrchr(path, '/');
    strncpy(nowPlayingName, n ? n + 1 : path, 255);
    size_t nl = strlen(nowPlayingName);
    if (nl > 4) nowPlayingName[nl - 4] = '\0';
    loadID3Tags(path);
    loadCoverArt(path);
}

void playNext(void) {
    if (playlistCount == 0) return;

    if (playbackMode == MODE_REPEAT_ONE) {
        char pathCopy[MAX_PATH];
        strncpy(pathCopy, nowPlayingPath, MAX_PATH - 1);
        pathCopy[MAX_PATH - 1] = '\0';
        startPlayback(pathCopy);
        return;
    }

    if (playbackMode == MODE_SHUFFLE) {
        int next = rand() % playlistCount;
        startPlayback(playlist[next].fullPath);
        return;
    }

    int currentIndex = -1;
    for (int i = 0; i < playlistCount; i++) {
        if (strcmp(playlist[i].fullPath, nowPlayingPath) == 0) {
            currentIndex = i;
            break;
        }
    }

    if (currentIndex != -1) {
        if (currentIndex < playlistCount - 1) {
            startPlayback(playlist[currentIndex + 1].fullPath);
        } else if (playbackMode == MODE_REPEAT_ALL) {
            startPlayback(playlist[0].fullPath);
        } else {
            stopPlayback();
        }
    }
}

void playPrevious(void) {
    if (playlistCount == 0) return;

    if (playbackMode == MODE_REPEAT_ONE) {
        char pathCopy[MAX_PATH];
        strncpy(pathCopy, nowPlayingPath, MAX_PATH - 1);
        pathCopy[MAX_PATH - 1] = '\0';
        startPlayback(pathCopy);
        return;
    }

    if (playbackMode == MODE_SHUFFLE) {
        int next = rand() % playlistCount;
        startPlayback(playlist[next].fullPath);
        return;
    }

    int currentIndex = -1;
    for (int i = 0; i < playlistCount; i++) {
        if (strcmp(playlist[i].fullPath, nowPlayingPath) == 0) {
            currentIndex = i;
            break;
        }
    }

    if (currentIndex != -1) {
        if (currentIndex > 0) {
            startPlayback(playlist[currentIndex - 1].fullPath);
        } else if (playbackMode == MODE_REPEAT_ALL) {
            startPlayback(playlist[playlistCount - 1].fullPath);
        } else {
            startPlayback(playlist[0].fullPath);
        }
    }
}

void autoAdvance(void) {
    playNext();
}

void fillAudio(void) {
    if (!playing || paused || !mh) return;
    for (int i = 0; i < 2; i++) {
        if (waveBuf[i].status == NDSP_WBUF_DONE) {
            u8 *buf = audioBuf + i * BUF_SIZE;
            size_t total = 0;
            fillBufferFromMPG(buf, &total);
            if (total == 0) { autoAdvance(); return; }
            DSP_FlushDataCache(buf, BUF_SIZE);
            waveBuf[i].nsamples = total / 4;
            waveBuf[i].status = 0;
            ndspChnWaveBufAdd(CHANNEL, &waveBuf[i]);
        }
    }
}
