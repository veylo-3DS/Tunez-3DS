#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <mpg123.h>
#include <id3tag.h>
#include <jpeglib.h>
#include <png.h>

#define TOP_WIDTH    400
#define BOT_WIDTH    320
#define SCR_HEIGHT   240
#define MAX_FILES    256
#define MAX_PATH     512
#define PAGE_SIZE    13
#define ART_SIZE     128

#define CLR_BG      C2D_Color32(0x1a, 0x1a, 0x2e, 0xFF)
#define CLR_PANEL   C2D_Color32(0x16, 0x21, 0x3e, 0xFF)
#define CLR_ACCENT  C2D_Color32(0x0f, 0x3c, 0x78, 0xFF)
#define CLR_HILIGHT C2D_Color32(0xe9, 0x4f, 0x37, 0xFF)
#define CLR_TEXT    C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define CLR_SUBTEXT C2D_Color32(0xaa, 0xaa, 0xcc, 0xFF)
#define CLR_DIR     C2D_Color32(0x64, 0xb5, 0xf6, 0xFF)
#define CLR_BAR_BG  C2D_Color32(0x33, 0x33, 0x55, 0xFF)
#define CLR_BAR_FG  C2D_Color32(0xe9, 0x4f, 0x37, 0xFF)

typedef enum { ENTRY_DIR, ENTRY_MP3 } EntryType;
typedef struct {
    char      name[256];
    char      fullPath[MAX_PATH];
    EntryType type;
} Entry;

static Entry entries[MAX_FILES];
static int   entryCount = 0, selected = 0, scrollOffset = 0;
static char  currentDir[MAX_PATH] = "sdmc:/Music";

static mpg123_handle *mh       = NULL;
static ndspWaveBuf    waveBuf[2];
static u8            *audioBuf = NULL;
static bool           playing  = false, paused = false;
static char           nowPlayingName[256]    = "";
static char           nowPlayingPath[MAX_PATH] = "";  // [1] full path for auto-advance
static char           nowPlayingArtist[256]  = "";   // [2] ID3 artist
static char           nowPlayingTitle[256]   = "";   // [2] ID3 title
static off_t          trackLen = 0;

static int  scrollTick   = 0;  // [3] filename ticker
static int  lastSelected = -1; // [3] detect selection change

#define CHANNEL     0
#define BUF_SAMPLES 4096
#define BUF_SIZE    (BUF_SAMPLES * 2 * 2)

static C3D_RenderTarget *topTarget = NULL;
static C3D_RenderTarget *botTarget = NULL;
static C2D_TextBuf       dynBuf    = NULL;

static C3D_Tex   artTex;
static C2D_Image artImage;
static bool      hasArt = false;

// -----------------------------------------------------------------------
static int nextPow2(int x) { int p = 1; while (p < x) p <<= 1; return p; }

static bool loadTexFromRGB(u8 *rgb, int w, int h) {
    if (hasArt) {
        if (artImage.subtex) {
            free((void*)artImage.subtex);
            artImage.subtex = NULL;
        }
        C3D_TexDelete(&artTex);
        hasArt = false;
    }

    int tw = nextPow2(w), th = nextPow2(h);
    if (!C3D_TexInit(&artTex, tw, th, GPU_RGBA8)) return false;

    u32 *dst = (u32*)artTex.data;
    memset(dst, 0, tw * th * 4);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src = (x * w + (h - 1 - y)) * 3;
            u32 pixel = (u32)rgb[src]
                      | ((u32)rgb[src+1] << 8)
                      | ((u32)rgb[src+2] << 16)
                      | (0xFFu << 24);

            int mx = x & 7, my = y & 7;
            int morton = 0;
            for (int b = 0; b < 3; b++) {
                morton |= ((mx >> b) & 1) << (2*b);
                morton |= ((my >> b) & 1) << (2*b+1);
            }
            dst[((y/8) * (tw/8) + (x/8)) * 64 + morton] = pixel;
        }
    }

    C3D_TexFlush(&artTex);

    Tex3DS_SubTexture *st = malloc(sizeof(Tex3DS_SubTexture));
    if (!st) { C3D_TexDelete(&artTex); return false; }

    st->width  = (u16)w;
    st->height = (u16)h;
    st->left   = 0.0f;
    st->right  = (float)w / tw;
    st->top    = 0.0f;
    st->bottom = (float)h / th;

    artImage.tex    = &artTex;
    artImage.subtex = st;
    hasArt = true;
    return true;
}

static u8 *scaleRGB(u8 *src, int sw, int sh, int dw, int dh) {
    u8 *dst = (u8 *)malloc(dw * dh * 3);
    if (!dst) return NULL;
    for (int y = 0; y < dh; y++)
        for (int x = 0; x < dw; x++) {
            int sx = x * sw / dw, sy = y * sh / dh;
            int si = (sy * sw + sx) * 3, di = (y * dw + x) * 3;
            dst[di] = src[si]; dst[di+1] = src[si+1]; dst[di+2] = src[si+2];
        }
    return dst;
}

static u8 *decodeJPEG(u8 *data, size_t len, int *outW, int *outH) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, len);
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo); return NULL;
    }
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);
    *outW = cinfo.output_width; *outH = cinfo.output_height;
    u8 *rgb = (u8 *)malloc((*outW) * (*outH) * 3);
    if (!rgb) { jpeg_destroy_decompress(&cinfo); return NULL; }
    while (cinfo.output_scanline < cinfo.output_height) {
        u8 *row = rgb + cinfo.output_scanline * (*outW) * 3;
        jpeg_read_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return rgb;
}

static u8 *decodePNG(u8 *data, size_t len, int *outW, int *outH) {
    png_image img;
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&img, data, len)) return NULL;
    img.format = PNG_FORMAT_RGB;
    *outW = img.width; *outH = img.height;
    u8 *rgb = (u8 *)malloc(PNG_IMAGE_SIZE(img));
    if (!rgb) { png_image_free(&img); return NULL; }
    if (!png_image_finish_read(&img, NULL, rgb, 0, NULL)) {
        free(rgb); png_image_free(&img); return NULL;
    }
    return rgb;
}

// [2] Read TIT2 (title) and TPE1 (artist) from ID3 tags
static void loadID3Tags(const char *mp3path) {
    nowPlayingTitle[0]  = '\0';
    nowPlayingArtist[0] = '\0';

    struct id3_file *f = id3_file_open(mp3path, ID3_FILE_MODE_READONLY);
    if (!f) return;
    struct id3_tag *tag = id3_file_tag(f);
    if (!tag) { id3_file_close(f); return; }

    struct id3_frame *frame;
    union id3_field  *field;
    const id3_ucs4_t *ucs4;
    id3_utf8_t       *utf8;

    frame = id3_tag_findframe(tag, "TIT2", 0);
    if (frame) {
        field = id3_frame_field(frame, 1);
        if (field) {
            ucs4 = id3_field_getstrings(field, 0);
            if (ucs4) {
                utf8 = id3_ucs4_utf8duplicate(ucs4);
                if (utf8) { strncpy(nowPlayingTitle, (char*)utf8, 255); free(utf8); }
            }
        }
    }

    frame = id3_tag_findframe(tag, "TPE1", 0);
    if (frame) {
        field = id3_frame_field(frame, 1);
        if (field) {
            ucs4 = id3_field_getstrings(field, 0);
            if (ucs4) {
                utf8 = id3_ucs4_utf8duplicate(ucs4);
                if (utf8) { strncpy(nowPlayingArtist, (char*)utf8, 255); free(utf8); }
            }
        }
    }

    id3_file_close(f);
}

static void loadCoverArt(const char *mp3path) {
    if (hasArt) {
        if (artImage.subtex) { free((void*)artImage.subtex); artImage.subtex = NULL; }
        C3D_TexDelete(&artTex);
        hasArt = false;
    }
    struct id3_file *f = id3_file_open(mp3path, ID3_FILE_MODE_READONLY);
    if (!f) return;
    struct id3_tag *tag = id3_file_tag(f);
    if (!tag) { id3_file_close(f); return; }
    struct id3_frame *frame = id3_tag_findframe(tag, "APIC", 0);
    if (!frame) { id3_file_close(f); return; }
    union id3_field *field = id3_frame_field(frame, 4);
    if (!field) { id3_file_close(f); return; }
    id3_length_t dataLen;
    const id3_byte_t *rawData = id3_field_getbinarydata(field, &dataLen);
    if (!rawData || dataLen == 0) { id3_file_close(f); return; }

    u8 *rgb = NULL; int w = 0, h = 0;
    if (dataLen > 2 && rawData[0] == 0xFF && rawData[1] == 0xD8)
        rgb = decodeJPEG((u8*)rawData, dataLen, &w, &h);
    else if (dataLen > 4 && rawData[0] == 0x89 && rawData[1] == 'P')
        rgb = decodePNG((u8*)rawData, dataLen, &w, &h);
    id3_file_close(f);
    if (!rgb) return;

    // Center-crop to square then scale to ART_SIZE
    int sq    = (w < h) ? w : h;
    int cropX = (w - sq) / 2;
    int cropY = (h - sq) / 2;

    u8 *cropped = malloc(sq * sq * 3);
    if (!cropped) { free(rgb); return; }
    for (int y = 0; y < sq; y++)
        memcpy(cropped + y * sq * 3,
               rgb + ((cropY + y) * w + cropX) * 3,
               sq * 3);
    free(rgb);

    u8 *scaled = scaleRGB(cropped, sq, sq, ART_SIZE, ART_SIZE);
    free(cropped);
    if (!scaled) return;

    loadTexFromRGB(scaled, ART_SIZE, ART_SIZE);
    free(scaled);
}

// -----------------------------------------------------------------------
static int entrySort(const void *a, const void *b) {
    const Entry *ea = (const Entry *)a, *eb = (const Entry *)b;
    if (ea->type != eb->type) return ea->type == ENTRY_DIR ? -1 : 1;
    return strcasecmp(ea->name, eb->name);
}

static void scanDir(const char *path) {
    entryCount = selected = scrollOffset = 0;
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *e;
    while ((e = readdir(dir)) && entryCount < MAX_FILES) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        Entry en;
        strncpy(en.name, e->d_name, 255); en.name[255] = '\0';
        snprintf(en.fullPath, MAX_PATH, "%s/%s", path, e->d_name);
        if (e->d_type == DT_DIR) {
            en.type = ENTRY_DIR;
            entries[entryCount++] = en;
        } else {
            size_t l = strlen(e->d_name);
            if (l > 4 && !strcasecmp(e->d_name + l - 4, ".mp3")) {
                en.type = ENTRY_MP3;
                entries[entryCount++] = en;
            }
        }
    }
    closedir(dir);
    qsort(entries, entryCount, sizeof(Entry), entrySort);
}

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

static void stopPlayback(void) {
    if (!playing) return;
    ndspChnReset(CHANNEL);
    if (mh) { mpg123_close(mh); mpg123_delete(mh); mh = NULL; }
    playing = paused = false;
    nowPlayingName[0]   = '\0';
    nowPlayingPath[0]   = '\0';
    nowPlayingArtist[0] = '\0';
    nowPlayingTitle[0]  = '\0';
    trackLen = 0;
}

static void startPlayback(const char *path) {
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
        waveBuf[i].nsamples   = total / 4;
        waveBuf[i].looping    = false;
        waveBuf[i].status     = 0;
        ndspChnWaveBufAdd(CHANNEL, &waveBuf[i]);
    }
    playing = true; paused = false;
    strncpy(nowPlayingPath, path, MAX_PATH - 1); // [1] store for auto-advance
    const char *n = strrchr(path, '/');
    strncpy(nowPlayingName, n ? n + 1 : path, 255);
    size_t nl = strlen(nowPlayingName);
    if (nl > 4) nowPlayingName[nl - 4] = '\0';
    loadID3Tags(path);   // [2] load title/artist
    loadCoverArt(path);
}

// [1] At end of track, find current entry and play the next MP3
static void autoAdvance(void) {
    if (!nowPlayingPath[0]) { stopPlayback(); return; }
    for (int i = 0; i < entryCount; i++) {
        if (!strcmp(entries[i].fullPath, nowPlayingPath)) {
            for (int j = i + 1; j < entryCount; j++) {
                if (entries[j].type == ENTRY_MP3) {
                    selected = j;
                    if (selected >= scrollOffset + PAGE_SIZE)
                        scrollOffset = selected - PAGE_SIZE + 1;
                    if (selected < scrollOffset)
                        scrollOffset = selected;
                    startPlayback(entries[j].fullPath);
                    return;
                }
            }
            break; // was last track, fall through to stop
        }
    }
    stopPlayback();
}

static void fillAudio(void) {
    if (!playing || paused || !mh) return;
    for (int i = 0; i < 2; i++) {
        if (waveBuf[i].status == NDSP_WBUF_DONE) {
            u8 *buf = audioBuf + i * BUF_SIZE;
            size_t total = 0;
            fillBufferFromMPG(buf, &total);
            if (total == 0) { autoAdvance(); return; } // [1] advance instead of stop
            DSP_FlushDataCache(buf, BUF_SIZE);
            waveBuf[i].nsamples = total / 4;
            waveBuf[i].status   = 0;
            ndspChnWaveBufAdd(CHANNEL, &waveBuf[i]);
        }
    }
}

static bool goUp(void) {
    if (!strcmp(currentDir, "sdmc:/") || !strcmp(currentDir, "sdmc:")) return false;
    char *slash = strrchr(currentDir, '/');
    if (!slash) return false;
    if (slash == currentDir + 6) *(slash + 1) = '\0';
    else *slash = '\0';
    return true;
}

// -----------------------------------------------------------------------
static void drawText(const char *str, float x, float y, float z, float scale, u32 color) {
    C2D_Text txt;
    C2D_TextBufClear(dynBuf);
    C2D_TextParse(&txt, dynBuf, str);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor, x, y, z, scale, scale, color);
}

static void drawTopScreen(void) {
    C2D_TargetClear(topTarget, CLR_BG);
    C2D_SceneBegin(topTarget);

    // Header
    C2D_DrawRectSolid(0, 0, 0, TOP_WIDTH, 32, CLR_PANEL);
    C2D_DrawRectSolid(0, 30, 0, TOP_WIDTH, 2, CLR_HILIGHT);
    drawText("Tunez3DS - Developed by Veylo :p", 12, 6, 0, 0.65f, CLR_TEXT);

    if (playing || paused) {
        int artX = 12, artY = 44;
        if (hasArt) {
            C2D_DrawImageAt(artImage, artX, artY, 0, NULL, 1.0f, 1.0f);
        } else {
            C2D_DrawRectSolid(artX, artY, 0, ART_SIZE, ART_SIZE, CLR_PANEL);
            C2D_DrawRectSolid(artX, artY, 0, ART_SIZE, 2, CLR_ACCENT);
            C2D_DrawRectSolid(artX, artY + ART_SIZE - 2, 0, ART_SIZE, 2, CLR_ACCENT);
            C2D_DrawRectSolid(artX, artY, 0, 2, ART_SIZE, CLR_ACCENT);
            C2D_DrawRectSolid(artX + ART_SIZE - 2, artY, 0, 2, ART_SIZE, CLR_ACCENT);
            drawText("No Art", artX + 36, artY + 56, 0, 0.45f, CLR_SUBTEXT);
        }

        int infoX = artX + ART_SIZE + 14;
        int infoW = TOP_WIDTH - infoX - 8;

        drawText("NOW PLAYING", infoX, 48, 0, 0.40f, CLR_SUBTEXT);

        // [2] Prefer ID3 title, fall back to filename
        char titleBuf[48];
        strncpy(titleBuf, nowPlayingTitle[0] ? nowPlayingTitle : nowPlayingName, 47);
        titleBuf[47] = '\0';
        drawText(titleBuf, infoX, 64, 0, 0.48f, CLR_TEXT);

        // [2] Show artist if available
        if (nowPlayingArtist[0]) {
            char artistBuf[48];
            strncpy(artistBuf, nowPlayingArtist, 47);
            artistBuf[47] = '\0';
            drawText(artistBuf, infoX, 80, 0, 0.42f, CLR_SUBTEXT);
        }

        float progress = 0.0f;
        if (trackLen > 0 && mh) {
            off_t pos = mpg123_tell(mh);
            if (pos >= 0) progress = (float)pos / (float)trackLen;
            if (progress < 0) progress = 0;
            if (progress > 1) progress = 1;
        }

        int barY = 102;
        C2D_DrawRectSolid(infoX, barY, 0, infoW, 6, CLR_BAR_BG);
        C2D_DrawRectSolid(infoX, barY, 0, (int)(infoW * progress), 6, CLR_BAR_FG);

        if (trackLen > 0 && mh) {
            long rate = 44100;
            mpg123_getformat(mh, &rate, NULL, NULL);
            int totalSec = (int)(trackLen / rate);
            off_t pos    = mpg123_tell(mh);
            int curSec   = (pos >= 0) ? (int)(pos / rate) : 0;
            char timeBuf[20];
            snprintf(timeBuf, sizeof(timeBuf), "%d:%02d / %d:%02d",
                     curSec / 60, curSec % 60, totalSec / 60, totalSec % 60);
            drawText(timeBuf, infoX, 114, 0, 0.38f, CLR_SUBTEXT);
        }

        drawText(paused ? "|| PAUSED" : "> PLAYING", infoX, 128, 0, 0.44f,
                 paused ? CLR_SUBTEXT : CLR_HILIGHT);
    } else {
        drawText("No track playing", 12, 100, 0, 0.55f, CLR_SUBTEXT);
        drawText("Select a song from the browser", 12, 125, 0, 0.45f, CLR_SUBTEXT);
    }

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 2, CLR_ACCENT);
    drawText("A Play  X Pause  Y Stop  B Back  START Quit", 12, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);
}

static void drawBotScreen(void) {
    C2D_TargetClear(botTarget, CLR_BG);
    C2D_SceneBegin(botTarget);

    C2D_DrawRectSolid(0, 0, 0, BOT_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, 26, 0, BOT_WIDTH, 2, CLR_ACCENT);
    const char *rel = currentDir + 6;
    if (!*rel) rel = "/";
    char dirLabel[48];
    snprintf(dirLabel, sizeof(dirLabel) - 1, "/%.*s", 44, rel);
    drawText(dirLabel, 8, 6, 0, 0.42f, CLR_SUBTEXT);

    int listY = 32, rowH = 15;
    if (entryCount == 0)
        drawText("(empty folder)", 8, listY + 20, 0, 0.45f, CLR_SUBTEXT);

    for (int i = 0; i < PAGE_SIZE && (i + scrollOffset) < entryCount; i++) {
        int idx = i + scrollOffset;
        Entry *e = &entries[idx];
        int y = listY + i * rowH;
        if (idx == selected) {
            C2D_DrawRectSolid(0, y - 1, 0, BOT_WIDTH, rowH, CLR_ACCENT);
            C2D_DrawRectSolid(0, y - 1, 0, 3, rowH, CLR_HILIGHT);
        }
        u32 color = (idx == selected) ? CLR_TEXT :
                    (e->type == ENTRY_DIR) ? CLR_DIR : CLR_SUBTEXT;

        // [3] Scroll long filenames on the selected entry.
        // After a 60-frame pause, advance one char every 10 frames.
        const char *namePtr = e->name;
        if (idx == selected) {
            int nameLen = (int)strlen(e->name);
            if (nameLen > 38 && scrollTick > 60) {
                int off = (scrollTick - 60) / 10;
                if (off > nameLen - 38) off = nameLen - 38;
                namePtr = e->name + off;
            }
        }

        char label[48];
        snprintf(label, sizeof(label) - 1,
                 e->type == ENTRY_DIR ? "/ %.42s" : "  %.42s", namePtr);
        drawText(label, 8, y, 0, 0.42f, color);
    }

    if (entryCount > PAGE_SIZE) {
        int listH = SCR_HEIGHT - 28 - 28;
        float sbH = (float)PAGE_SIZE / entryCount * listH;
        float sbY = 28 + (float)scrollOffset / entryCount * listH;
        C2D_DrawRectSolid(BOT_WIDTH - 4, 28, 0, 4, listH, CLR_PANEL);
        C2D_DrawRectSolid(BOT_WIDTH - 4, (int)sbY, 0, 4, (int)sbH, CLR_HILIGHT);
    }

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, BOT_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, BOT_WIDTH, 2, CLR_ACCENT);
    drawText("A Open/Play   B Back", 8, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);
}

// -----------------------------------------------------------------------
int main(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    dynBuf    = C2D_TextBufNew(1024);

    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    audioBuf = (u8 *)linearAlloc(BUF_SIZE * 2);
    mpg123_init();

    aptSetSleepAllowed(false);

    scanDir(currentDir);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();

        if (down & KEY_START) break;

        if (down & KEY_DOWN) {
            if (selected < entryCount - 1) {
                selected++;
                if (selected >= scrollOffset + PAGE_SIZE) scrollOffset++;
            }
        }
        if (down & KEY_UP) {
            if (selected > 0) {
                selected--;
                if (selected < scrollOffset) scrollOffset--;
            }
        }
        if (down & KEY_A && entryCount > 0) {
            Entry *e = &entries[selected];
            if (e->type == ENTRY_DIR) {
                strncpy(currentDir, e->fullPath, MAX_PATH - 1);
                scanDir(currentDir);
            } else {
                startPlayback(e->fullPath);
            }
        }
        if (down & KEY_B) {
            // [4] Remember which subdirectory we're leaving so we can re-select it
            const char *slash = strrchr(currentDir, '/');
            char childName[256] = "";
            if (slash) strncpy(childName, slash + 1, 255);
            if (goUp()) {
                scanDir(currentDir);
                if (childName[0]) {
                    for (int i = 0; i < entryCount; i++) {
                        if (entries[i].type == ENTRY_DIR &&
                            !strcmp(entries[i].name, childName)) {
                            selected = i;
                            scrollOffset = selected - PAGE_SIZE / 2;
                            if (scrollOffset < 0) scrollOffset = 0;
                            if (scrollOffset + PAGE_SIZE > entryCount)
                                scrollOffset = entryCount - PAGE_SIZE;
                            if (scrollOffset < 0) scrollOffset = 0;
                            break;
                        }
                    }
                }
            }
        }
        if (down & KEY_X && playing) {
            paused = !paused;
            ndspChnSetPaused(CHANNEL, paused);
        }
        if (down & KEY_Y) stopPlayback();

        // [3] Reset scroll ticker when selection changes
        if (selected != lastSelected) {
            scrollTick   = 0;
            lastSelected = selected;
        } else {
            scrollTick++;
        }

        fillAudio();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTopScreen();
        drawBotScreen();
        C3D_FrameEnd(0);
    }

    stopPlayback();
    if (hasArt) {
        if (artImage.subtex) free((void*)artImage.subtex);
        C3D_TexDelete(&artTex);
    }
    mpg123_exit();
    linearFree(audioBuf);
    ndspExit();
    C2D_TextBufDelete(dynBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}