#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <id3tag.h>
#include <jpeglib.h>
#include <png.h>
#include <sys/stat.h>

Theme themes[] = {
    {
        "Dark Blue (Default)",
        MKCOL(0x1a,0x1a,0x2e,0xFF), MKCOL(0x16,0x21,0x3e,0xFF),
        MKCOL(0x0f,0x3c,0x78,0xFF), MKCOL(0xe9,0x4f,0x37,0xFF),
        MKCOL(0xFF,0xFF,0xFF,0xFF), MKCOL(0xaa,0xaa,0xcc,0xFF),
        MKCOL(0x64,0xb5,0xf6,0xFF), MKCOL(0x33,0x33,0x55,0xFF),
        MKCOL(0xe9,0x4f,0x37,0xFF)
    },
    {
        "Forest Green",
        MKCOL(0x0d,0x1f,0x12,0xFF), MKCOL(0x10,0x2a,0x18,0xFF),
        MKCOL(0x1a,0x5c,0x2a,0xFF), MKCOL(0x76,0xc4,0x42,0xFF),
        MKCOL(0xFF,0xFF,0xFF,0xFF), MKCOL(0x88,0xbb,0x88,0xFF),
        MKCOL(0x76,0xc4,0x42,0xFF), MKCOL(0x1a,0x3a,0x1a,0xFF),
        MKCOL(0x76,0xc4,0x42,0xFF)
    },
    {
        "Rose Pink",
        MKCOL(0x2e,0x1a,0x1a,0xFF), MKCOL(0x3e,0x1e,0x2a,0xFF),
        MKCOL(0x78,0x1e,0x4a,0xFF), MKCOL(0xff,0x69,0x9a,0xFF),
        MKCOL(0xFF,0xFF,0xFF,0xFF), MKCOL(0xcc,0xaa,0xbb,0xFF),
        MKCOL(0xff,0x99,0xcc,0xFF), MKCOL(0x55,0x22,0x33,0xFF),
        MKCOL(0xff,0x69,0x9a,0xFF)
    },
    {
        "Midnight Purple",
        MKCOL(0x12,0x0d,0x1f,0xFF), MKCOL(0x1c,0x14,0x30,0xFF),
        MKCOL(0x40,0x1a,0x6e,0xFF), MKCOL(0xb3,0x6f,0xff,0xFF),
        MKCOL(0xFF,0xFF,0xFF,0xFF), MKCOL(0xaa,0x99,0xcc,0xFF),
        MKCOL(0xb3,0x6f,0xff,0xFF), MKCOL(0x28,0x18,0x44,0xFF),
        MKCOL(0xb3,0x6f,0xff,0xFF)
    },
    {
        "Classic Light",
        MKCOL(0xf0,0xf0,0xf0,0xFF), MKCOL(0xdd,0xdd,0xdd,0xFF),
        MKCOL(0x88,0xbb,0xff,0xFF), MKCOL(0x22,0x66,0xcc,0xFF),
        MKCOL(0x11,0x11,0x11,0xFF), MKCOL(0x55,0x55,0x77,0xFF),
        MKCOL(0x22,0x66,0xcc,0xFF), MKCOL(0xcc,0xcc,0xdd,0xFF),
        MKCOL(0x22,0x66,0xcc,0xFF)
    },
    {
        "Blood Red",
        MKCOL(0x08,0x08,0x08,0xFF), MKCOL(0x12,0x00,0x00,0xFF),
        MKCOL(0x3a,0x00,0x00,0xFF), MKCOL(0xcc,0x00,0x00,0xFF),
        MKCOL(0xFF,0xFF,0xFF,0xFF), MKCOL(0x99,0x66,0x66,0xFF),
        MKCOL(0xff,0x44,0x44,0xFF), MKCOL(0x22,0x00,0x00,0xFF),
        MKCOL(0xcc,0x00,0x00,0xFF)
    },
};

const int THEME_COUNT = (int)(sizeof(themes)/sizeof(themes[0]));
int currentTheme = 0;

#define SETTINGS_PATH "sdmc:/Tunez3DS/settings.bin"

C3D_RenderTarget *topTarget = NULL;
C3D_RenderTarget *botTarget = NULL;
C2D_TextBuf dynBuf = NULL;

C3D_Tex artTex;
C2D_Image artImage;
bool hasArt = false;

void saveTheme(void) {
    mkdir("sdmc:/Tunez3DS", 0777);
    FILE *f = fopen(SETTINGS_PATH, "wb");
    if (!f) return;
    fwrite(&currentTheme, sizeof(int), 1, f);
    fclose(f);
}

void loadTheme(void) {
    FILE *f = fopen(SETTINGS_PATH, "rb");
    if (!f) return;
    int t = 0;
    if (fread(&t, sizeof(int), 1, f) == 1)
        if (t >= 0 && t < THEME_COUNT) currentTheme = t;
    fclose(f);
}

void drawText(const char *str, float x, float y, float z, float scale, u32 color) {
    C2D_Text txt;
    C2D_TextBufClear(dynBuf);
    C2D_TextParse(&txt, dynBuf, str);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor, x, y, z, scale, scale, color);
}

static int nextPow2(int x) { int p = 1; while (p < x) p <<= 1; return p; }

static bool loadTexFromRGB(u8 *rgb, int w, int h) {
    if (hasArt) {
        if (artImage.subtex) { free((void*)artImage.subtex); artImage.subtex = NULL; }
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

void loadID3Tags(const char *mp3path) {
    nowPlayingTitle[0]  = '\0';
    nowPlayingArtist[0] = '\0';
    nowPlayingAlbum[0]  = '\0';

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

    frame = id3_tag_findframe(tag, "TALB", 0);
    if (frame) {
        field = id3_frame_field(frame, 1);
        if (field) {
            ucs4 = id3_field_getstrings(field, 0);
            if (ucs4) {
                utf8 = id3_ucs4_utf8duplicate(ucs4);
                if (utf8) { strncpy(nowPlayingAlbum, (char*)utf8, 255); free(utf8); }
            }
        }
    }

    id3_file_close(f);
}

void loadCoverArt(const char *mp3path) {
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

void drawSettingsScreen(void) {
    C2D_TargetClear(topTarget, CLR_BG);
    C2D_SceneBegin(topTarget);
    C2D_DrawRectSolid(0, 0, 0, TOP_WIDTH, 32, CLR_PANEL);
    C2D_DrawRectSolid(0, 30, 0, TOP_WIDTH, 2, CLR_HILIGHT);
    {
        C2D_Text txt;
        C2D_TextBufClear(dynBuf);
        C2D_TextParse(&txt, dynBuf, "Tunez3DS - Settings");
        C2D_TextOptimize(&txt);
        float tw, th;
        C2D_TextGetDimensions(&txt, 0.65f, 0.65f, &tw, &th);
        C2D_DrawText(&txt, C2D_WithColor, (TOP_WIDTH - tw) / 2.0f, (32 - th) / 2.0f, 0, 0.65f, 0.65f, CLR_TEXT);
    }

    drawText("Color Theme", 12, 44, 0, 0.55f, CLR_SUBTEXT);
    for (int i = 0; i < THEME_COUNT; i++) {
        int y = 66 + i * 22;
        if (i == currentTheme) {
            C2D_DrawRectSolid(8, y - 2, 0, TOP_WIDTH - 16, 20, CLR_ACCENT);
            C2D_DrawRectSolid(8, y - 2, 0, 3, 20, CLR_HILIGHT);
        }
        drawText(themes[i].name, 18, y, 0, 0.48f,
                 i == currentTheme ? CLR_TEXT : CLR_SUBTEXT);
    }

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 2, CLR_ACCENT);
    drawText("Up/Down Select   A Apply   B Back", 12, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);

    C2D_TargetClear(botTarget, CLR_BG);
    C2D_SceneBegin(botTarget);
    C2D_DrawRectSolid(0, 0, 0, BOT_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, 26, 0, BOT_WIDTH, 2, CLR_ACCENT);
    drawText("Preview", 8, 6, 0, 0.50f, CLR_TEXT);

    int swY = 40, swSize = 28, swGap = 8;
    int totalW = THEME_COUNT * (swSize + swGap) - swGap;
    int swStartX = (BOT_WIDTH - totalW) / 2;
    for (int i = 0; i < THEME_COUNT; i++) {
        int x = swStartX + i * (swSize + swGap);
        C2D_DrawRectSolid(x, swY, 0, swSize, swSize, themes[i].bg);
        C2D_DrawRectSolid(x, swY, 0, swSize, 2, themes[i].hilight);
        C2D_DrawRectSolid(x, swY + swSize - 2, 0, swSize, 2, themes[i].hilight);
        C2D_DrawRectSolid(x, swY, 0, 2, swSize, themes[i].hilight);
        C2D_DrawRectSolid(x + swSize - 2, swY, 0, 2, swSize, themes[i].hilight);
        C2D_DrawRectSolid(x + swSize/2 - 4, swY + swSize/2 - 4, 0, 8, 8, themes[i].accent);
        if (i == currentTheme) {
            C2D_DrawRectSolid(x, swY + swSize + 3, 0, swSize, 4, themes[i].hilight);
        }
    }

    int pY = 90;
    C2D_DrawRectSolid(8, pY, 0, BOT_WIDTH - 16, 80, CLR_PANEL);
    C2D_DrawRectSolid(8, pY, 0, BOT_WIDTH - 16, 2, CLR_ACCENT);
    C2D_DrawRectSolid(8, pY + 78, 0, BOT_WIDTH - 16, 2, CLR_ACCENT);
    drawText("Sample Track", 16, pY + 8, 0, 0.45f, CLR_TEXT);
    drawText("Sample Artist", 16, pY + 24, 0, 0.40f, CLR_SUBTEXT);
    C2D_DrawRectSolid(16, pY + 44, 0, BOT_WIDTH - 32, 6, CLR_BAR_BG);
    C2D_DrawRectSolid(16, pY + 44, 0, (BOT_WIDTH - 32) / 2, 6, CLR_BAR_FG);
    drawText("> PLAYING", 16, pY + 58, 0, 0.42f, CLR_HILIGHT);

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, BOT_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, BOT_WIDTH, 2, CLR_ACCENT);
    drawText("Theme preview", 8, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);
}

void drawTopScreen(void) {
    C2D_TargetClear(topTarget, CLR_BG);
    C2D_SceneBegin(topTarget);

    C2D_DrawRectSolid(0, 0, 0, TOP_WIDTH, 32, CLR_PANEL);
    C2D_DrawRectSolid(0, 30, 0, TOP_WIDTH, 2, CLR_HILIGHT);

    {
        C2D_Text txt;
        C2D_TextBufClear(dynBuf);
        C2D_TextParse(&txt, dynBuf, "Tunez3DS");
        C2D_TextOptimize(&txt);
        float tw, th;
        C2D_TextGetDimensions(&txt, 0.65f, 0.65f, &tw, &th);
        C2D_DrawText(&txt, C2D_WithColor, (TOP_WIDTH - tw) / 2.0f, (32 - th) / 2.0f, 0, 0.65f, 0.65f, CLR_TEXT);
    }

    if (playing || paused) {
        int artX = 12, artY = 44;
        int infoX = artX + ART_SIZE + 14;
        int infoW = TOP_WIDTH - infoX - 8;

        drawText("NOW PLAYING", infoX, 48, 0, 0.40f, CLR_SUBTEXT);

        char titleBuf[48];
        strncpy(titleBuf, nowPlayingTitle[0] ? nowPlayingTitle : nowPlayingName, 47);
        titleBuf[47] = '\0';
        drawText(titleBuf, infoX, 64, 0, 0.48f, CLR_TEXT);

        if (nowPlayingArtist[0]) {
            char artistBuf[48];
            strncpy(artistBuf, nowPlayingArtist, 47);
            artistBuf[47] = '\0';
            drawText(artistBuf, infoX, 80, 0, 0.42f, CLR_SUBTEXT);
        }

        if (nowPlayingAlbum[0]) {
            char albumBuf[48];
            strncpy(albumBuf, nowPlayingAlbum, 47);
            albumBuf[47] = '\0';
            drawText(albumBuf, infoX, 92, 0, 0.35f, CLR_SUBTEXT);
        }

        float progress = 0.0f;
        if (trackLen > 0 && mh) {
            off_t pos = mpg123_tell(mh);
            if (pos >= 0) progress = (float)pos / (float)trackLen;
            if (progress < 0) progress = 0;
            if (progress > 1) progress = 1;
        }

        int barY = 108;
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
            drawText(timeBuf, infoX, 120, 0, 0.38f, CLR_SUBTEXT);
        }

        drawText(paused ? "|| PAUSED" : "> PLAYING", infoX, 134, 0, 0.44f,
                 paused ? CLR_SUBTEXT : CLR_HILIGHT);

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
    } else {
        drawText("No track playing", 12, 100, 0, 0.55f, CLR_SUBTEXT);
        drawText("Select a song from the browser", 12, 125, 0, 0.45f, CLR_SUBTEXT);
    }

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 2, CLR_ACCENT);
    drawText("A Play  X Pause  Y Stop  B Back  START Quit", 12, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);
}

void drawBotScreen(void) {
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
                 e->type == ENTRY_DIR ? "[D] %s" : " %s", namePtr);
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
    drawText("A Open/Play   B Back   SELECT Settings", 8, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);
}
