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
    {
        "Everforest",
        MKCOL(0x2d, 0x35, 0x3b, 0xFF), MKCOL(0x47, 0x52, 0x58, 0xFF),
        MKCOL(0x7f, 0xbb, 0xb3, 0xFF), MKCOL(0xa7, 0xc0, 0x80, 0xFF),
        MKCOL(0xd3, 0xc6, 0xaa, 0xFF), MKCOL(0x85, 0x92, 0x89, 0xFF),
        MKCOL(0x7f, 0xbb, 0xb3, 0xFF), MKCOL(0x3d, 0x48, 0x4d, 0xFF),
        MKCOL(0xa7, 0xc0, 0x80, 0xFF)
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

static void drawRoundedRect(float x, float y, float w, float h, float r, u32 color) {
    C2D_DrawRectSolid(x + r, y, 0, w - 2 * r, h, color);
    C2D_DrawRectSolid(x, y + r, 0, w, h - 2 * r, color);
    C2D_DrawCircleSolid(x + r, y + r, 0, r, color);
    C2D_DrawCircleSolid(x + w - r, y + r, 0, r, color);
    C2D_DrawCircleSolid(x + r, y + h - r, 0, r, color);
    C2D_DrawCircleSolid(x + w - r, y + h - r, 0, r, color);
}

void saveTheme(void) {
    mkdir("sdmc:/Tunez3DS", 0777);
    FILE *f = fopen(SETTINGS_PATH, "wb");
    if (!f) return;
    fwrite(&currentTheme, sizeof(int), 1, f);
    fwrite(&disableLRSkipClosed, sizeof(bool), 1, f);
    fclose(f);
}

void loadTheme(void) {
    FILE *f = fopen(SETTINGS_PATH, "rb");
    if (!f) return;
    int t = 0;
    if (fread(&t, sizeof(int), 1, f) == 1)
        if (t >= 0 && t < THEME_COUNT) currentTheme = t;
    fread(&disableLRSkipClosed, sizeof(bool), 1, f);
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
            u32 pixel = (0xFFu)                 | ((u32)rgb[src+2] << 8)  | ((u32)rgb[src+1] << 16) | ((u32)rgb[src] << 24); 

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
    
    char titleBuf[64];
    snprintf(titleBuf, 64, "Settings (Page %d/4)", settingsPage + 1);
    
    {
        C2D_Text txt;
        C2D_TextBufClear(dynBuf);
        C2D_TextParse(&txt, dynBuf, titleBuf);
        C2D_TextOptimize(&txt);
        float tw, th;
        C2D_TextGetDimensions(&txt, 0.65f, 0.65f, &tw, &th);
        C2D_DrawText(&txt, C2D_WithColor, (TOP_WIDTH - tw) / 2.0f, (32 - th) / 2.0f, 0, 0.65f, 0.65f, CLR_TEXT);
    }

    // L/R Page Indicators
    drawText("< L", 10, 6, 0, 0.5f, CLR_HILIGHT);
    drawText("R >", TOP_WIDTH - 35, 6, 0, 0.5f, CLR_HILIGHT);

    if (settingsPage == 0) {
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
    } else if (settingsPage == 1) {
        drawText("Playback Speed", 12, 44, 0, 0.55f, CLR_SUBTEXT);
        
        drawRoundedRect(40, 70, TOP_WIDTH - 80, 80, 12, CLR_PANEL);
        char speedBuf[32];
        snprintf(speedBuf, 32, "%.2fx", playbackSpeed);
        
        C2D_Text txt;
        C2D_TextBufClear(dynBuf);
        C2D_TextParse(&txt, dynBuf, speedBuf);
        C2D_TextOptimize(&txt);
        float tw, th;
        C2D_TextGetDimensions(&txt, 1.2f, 1.2f, &tw, &th);
        drawText(speedBuf, (TOP_WIDTH - tw) / 2.0f, 90, 0, 1.2f, CLR_ACCENT);

        drawText("D-Pad Left/Right: -/+ 0.1x", 45, 160, 0, 0.42f, CLR_TEXT);
        drawText("X Button: Reset to 1.0x", 45, 180, 0, 0.42f, CLR_TEXT);
    } else if (settingsPage == 2) {
        drawText("Input Safety", 12, 44, 0, 0.55f, CLR_SUBTEXT);
        
        drawText("L/R Skip (Lid Closed):", 18, 70, 0, 0.48f, CLR_TEXT);
        drawText(disableLRSkipClosed ? "PROTECTED" : "UNPROTECTED", 200, 70, 0, 0.48f, disableLRSkipClosed ? CLR_HILIGHT : CLR_ACCENT);
        drawText("(Press A to toggle)", 18, 85, 0, 0.35f, CLR_SUBTEXT);

        if (disableLRSkipClosed) {
            drawText("Track skipping via L/R is DISABLED", 18, 120, 0, 0.45f, CLR_HILIGHT);
            drawText("while the 3DS lid is closed.", 18, 138, 0, 0.45f, CLR_HILIGHT);
        } else {
            drawText("Track skipping via L/R is ENABLED", 18, 120, 0, 0.45f, CLR_ACCENT);
            drawText("even while the 3DS lid is closed.", 18, 138, 0, 0.45f, CLR_ACCENT);
        }
        
        drawText("This setting helps prevent accidental", 18, 170, 0, 0.42f, CLR_SUBTEXT);
        drawText("skips while the console is in your pocket.", 18, 185, 0, 0.42f, CLR_SUBTEXT);
    } else if (settingsPage == 3) {
        drawText("Software Update", 12, 44, 0, 0.55f, CLR_SUBTEXT);
        
        char curBuf[64];
        snprintf(curBuf, 64, "Current Version: %s", APP_VERSION);
        drawText(curBuf, 18, 75, 0, 0.48f, CLR_TEXT);

        drawText("Update Status:", 18, 110, 0, 0.48f, CLR_SUBTEXT);
        
        const char *statusText = "Idle";
        u32 statusColor = CLR_TEXT;

        if (updateStatus == UPDATE_CHECKING) {
            statusText = "Checking...";
            statusColor = CLR_ACCENT;
        } else if (updateStatus == UPDATE_UP_TO_DATE) {
            statusText = "Your software is up to date!";
            statusColor = CLR_HILIGHT;
        } else if (updateStatus == UPDATE_AVAILABLE) {
            static char availBuf[64];
            snprintf(availBuf, 64, "New version available: %s", remoteVersion);
            statusText = availBuf;
            statusColor = CLR_ACCENT;
        } else if (updateStatus == UPDATE_ERROR) {
            statusText = "Error checking for updates.";
            statusColor = MKCOL(255, 50, 50, 255);
        }

        drawText(statusText, 18, 130, 0, 0.45f, statusColor);
        
        if (updateStatus == UPDATE_IDLE || updateStatus == UPDATE_ERROR || updateStatus == UPDATE_UP_TO_DATE) {
            drawText("Press A to check for updates", 18, 170, 0, 0.42f, CLR_ACCENT);
        }
    }

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, TOP_WIDTH, 2, CLR_ACCENT);
    drawText("L/R Switch Pages   B/SELECT/START Back", 12, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);

    C2D_TargetClear(botTarget, CLR_BG);
    C2D_SceneBegin(botTarget);
    
    if (settingsPage == 0) {
        C2D_DrawRectSolid(0, 0, 0, BOT_WIDTH, 28, CLR_PANEL);
        C2D_DrawRectSolid(0, 26, 0, BOT_WIDTH, 2, CLR_ACCENT);
        drawText("Theme Preview", 8, 6, 0, 0.50f, CLR_TEXT);

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
    } else if (settingsPage == 1) {
        drawRoundedRect(20, 40, BOT_WIDTH - 40, 160, 12, CLR_PANEL);
        drawText("Playback speed adjusts the", 40, 60, 0, 0.45f, CLR_TEXT);
        drawText("pitch and rate of the audio.", 40, 80, 0, 0.45f, CLR_TEXT);
        drawText("Higher speed = High pitch", 40, 110, 0, 0.45f, CLR_SUBTEXT);
        drawText("Lower speed = Low pitch", 40, 130, 0, 0.45f, CLR_SUBTEXT);
    } else if (settingsPage == 2) {
        drawRoundedRect(20, 40, BOT_WIDTH - 40, 160, 12, CLR_PANEL);
        drawText("Safety Options", 40, 60, 0, 0.45f, CLR_TEXT);
        drawText("Lid sensor detection", 40, 90, 0, 0.40f, CLR_HILIGHT);
        
        drawText("When 'Protected', the app", 40, 120, 0, 0.40f, CLR_TEXT);
        drawText("ignores L/R buttons while", 40, 135, 0, 0.40f, CLR_TEXT);
        drawText("the system is closed.", 40, 150, 0, 0.40f, CLR_TEXT);
    } else if (settingsPage == 3) {
        drawRoundedRect(20, 40, BOT_WIDTH - 40, 160, 12, CLR_PANEL);
        drawText("Software Update", 40, 60, 0, 0.45f, CLR_TEXT);
        drawText("Check for newer versions", 40, 90, 0, 0.40f, CLR_HILIGHT);
        
        drawText("This requires an active Wi-Fi", 40, 120, 0, 0.40f, CLR_TEXT);
        drawText("connection. The app will", 40, 135, 0, 0.40f, CLR_TEXT);
        drawText("compare your version with", 40, 150, 0, 0.40f, CLR_TEXT);
        drawText("the latest on GitHub.", 40, 165, 0, 0.40f, CLR_TEXT);
    }

    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, BOT_WIDTH, 28, CLR_PANEL);
    C2D_DrawRectSolid(0, SCR_HEIGHT - 28, 0, BOT_WIDTH, 2, CLR_ACCENT);
    drawText("Settings page preview", 8, SCR_HEIGHT - 20, 0, 0.38f, CLR_SUBTEXT);
}

void drawTopScreen(void) {
    C2D_TargetClear(topTarget, CLR_BG);
    C2D_SceneBegin(topTarget);

    // Modern Header
    drawRoundedRect(0, -10, TOP_WIDTH, 42, 12, CLR_PANEL);
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
        int artX = 16, artY = 52;
        int infoX = artX + ART_SIZE + 16;
        int infoW = TOP_WIDTH - infoX - 16;

        // Album Art
        if (hasArt) {
            C2D_DrawImageAt(artImage, artX, artY, 0, NULL, 1.0f, 1.0f);
        } else {
            // Placeholder for art
            drawRoundedRect(artX, artY, ART_SIZE, ART_SIZE, 8, CLR_PANEL);
            drawText("No Art", artX + 36, artY + 56, 0, 0.45f, CLR_SUBTEXT);
        }

        // Refined visualizer - Positioned at lower baseline
        int bars = 16;
        float barW = (float)infoW / bars;
        float barBaselineY = 180; // Baseline at the bottom
        for (int i = 0; i < bars; i++) {
            float barH = visualizerAmplitude[i] * 40;
            if (barH < 4) barH = 4;
            drawRoundedRect(infoX + i * barW + 2, barBaselineY - barH, barW - 4, barH, 2, CLR_HILIGHT);
        }

        // Metadata positioned below visualizer
        drawText("NOW PLAYING", infoX, 70, 0, 0.35f, CLR_SUBTEXT);

        char titleBuf[256];
        strncpy(titleBuf, nowPlayingTitle[0] ? nowPlayingTitle : nowPlayingName, 255);
        int titleLen = (int)strlen(titleBuf);
        const char *displayTitle = titleBuf;
        if (titleLen > 22) {
            int cycle = (scrollTick / 6) % (titleLen + 5); 
            if (cycle < titleLen) {
                static char wrappedTitle[512];
                snprintf(wrappedTitle, 512, "%s   %s", titleBuf, titleBuf);
                displayTitle = wrappedTitle + cycle;
            }
        }
        drawText(displayTitle, infoX, 85, 0, 0.60f, CLR_TEXT);

        char metaBuf[256];
        if (nowPlayingArtist[0] && nowPlayingAlbum[0])
            snprintf(metaBuf, 255, "%s • %s", nowPlayingArtist, nowPlayingAlbum);
        else if (nowPlayingArtist[0])
            strncpy(metaBuf, nowPlayingArtist, 255);
        else
            strncpy(metaBuf, "Unknown Artist", 255);
        
        int metaLen = (int)strlen(metaBuf);
        const char *displayMeta = metaBuf;
        if (metaLen > 30) {
            int cycle = (scrollTick / 8) % (metaLen + 5);
            if (cycle < metaLen) {
                static char wrappedMeta[512];
                snprintf(wrappedMeta, 512, "%s   %s", metaBuf, metaBuf);
                displayMeta = wrappedMeta + cycle;
            }
        }
        drawText(displayMeta, infoX, 100, 0, 0.45f, CLR_SUBTEXT);

        char speedBuf[16];
        snprintf(speedBuf, 16, "Speed: %.2fx", playbackSpeed);
        drawText(speedBuf, infoX, 120, 0, 0.40f, CLR_HILIGHT);

        float progress = 0.0f;
        if (trackLen > 0 && mh) {
            off_t pos = mpg123_tell(mh);
            if (pos >= 0) progress = (float)pos / (float)trackLen;
            if (progress < 0) progress = 0;
            if (progress > 1) progress = 1;
        }

        drawRoundedRect(16, 190, TOP_WIDTH - 32, 8, 4, CLR_BAR_BG);
        drawRoundedRect(16, 190, (int)((TOP_WIDTH - 32) * progress), 8, 4, CLR_BAR_FG);
    } else {
        drawRoundedRect(12, 80, TOP_WIDTH - 24, 80, 8, CLR_PANEL);
        drawText("No track playing", 24, 100, 0, 0.55f, CLR_SUBTEXT);
        drawText("Select a song from the browser", 24, 125, 0, 0.45f, CLR_SUBTEXT);
    }
}

static void drawFolderIcon(float x, float y, u32 color) {
    drawRoundedRect(x, y + 2, 12, 9, 2, color);
    C2D_DrawRectSolid(x, y, 0, 5, 3, color);
}

static void drawNoteIcon(float x, float y, u32 color) {
    C2D_DrawRectSolid(x + 7, y, 0, 2, 10, color);
    C2D_DrawCircleSolid(x + 5, y + 10, 0, 3, color);
    drawRoundedRect(x + 7, y, 5, 2, 1, color);
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

    int listY = 40, rowH = 16;
    if (entryCount == 0)
        drawText("(empty folder)", 8, listY + 20, 0, 0.45f, CLR_SUBTEXT);

    for (int i = 0; i < PAGE_SIZE && (i + scrollOffset) < entryCount; i++) {
        int idx = i + scrollOffset;
        Entry *e = &entries[idx];
        int y = listY + i * rowH;
        
        if (idx == selected) {
            // Shadow
            drawRoundedRect(6, y - 1, BOT_WIDTH - 12, rowH, 4, MKCOL(0,0,0,80));
            // Card
            drawRoundedRect(6, y - 2, BOT_WIDTH - 12, rowH, 4, CLR_PANEL);
            // Rounded Accent
            drawRoundedRect(6, y - 2, 6, rowH, 3, CLR_HILIGHT);
        }
        
        u32 color = (idx == selected) ? CLR_TEXT :
                    (e->type == ENTRY_DIR) ? CLR_DIR : CLR_SUBTEXT;

        if (e->type == ENTRY_DIR) drawFolderIcon(16, y + 2, color);
        else drawNoteIcon(16, y + 2, color);

        const char *namePtr = e->name;
        if (idx == selected) {
            int nameLen = (int)strlen(e->name);
            if (nameLen > 34 && scrollTick > 60) {
                int off = (scrollTick - 60) / 10;
                if (off > nameLen - 34) off = nameLen - 34;
                namePtr = e->name + off;
            }
        }

        char label[48];
        snprintf(label, sizeof(label) - 1, "   %.42s", namePtr);
        drawText(label, 30, y, 0, 0.42f, color);
    }

    if (entryCount > PAGE_SIZE) {
        int listH = 160;
        float sbH = (float)PAGE_SIZE / entryCount * listH;
        float sbY = listY + (float)scrollOffset / entryCount * listH;
        // Modern Pill Scrollbar
        drawRoundedRect(BOT_WIDTH - 6, listY, 4, listH, 2, CLR_PANEL);
        drawRoundedRect(BOT_WIDTH - 6, (int)sbY, 4, (int)sbH, 2, CLR_HILIGHT);
    }

    // Modern compact status bar
    drawRoundedRect(4, SCR_HEIGHT - 26, BOT_WIDTH - 8, 22, 11, CLR_PANEL);
    C2D_DrawRectSolid(15, SCR_HEIGHT - 26, 0, BOT_WIDTH - 30, 1, MKCOL(255,255,255,30));
    
    const char *modeNames[] = {"Normal", "Repeat All", "Repeat One", "Shuffle"};
    char modeBuf[32];
    snprintf(modeBuf, sizeof(modeBuf), "Mode: %s", modeNames[playbackMode]);
    drawText(modeBuf, BOT_WIDTH - 110, SCR_HEIGHT - 20, 0, 0.35f, CLR_HILIGHT);

    drawText("A Play/Open  L/R Skip  D-Pad L/R Mode", 16, SCR_HEIGHT - 20, 0, 0.35f, CLR_SUBTEXT);
}
