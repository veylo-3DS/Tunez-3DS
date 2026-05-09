#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro2d.h>
#include <mpg123.h>

#define TOP_WIDTH    400
#define BOT_WIDTH    320
#define SCR_HEIGHT   240
#define MAX_FILES    1024
#define MAX_PATH     512
#define PAGE_SIZE    10
#define ART_SIZE     128

#define MKCOL(r,g,b,a) ((u32)(r) | ((u32)(g)<<8) | ((u32)(b)<<16) | ((u32)(a)<<24))
#define red(c)   ((u8)((c) >> 0))
#define green(c) ((u8)((c) >> 8))
#define blue(c)  ((u8)((c) >> 16))
#define alpha(c) ((u8)((c) >> 24))

typedef enum { ENTRY_DIR, ENTRY_MP3 } EntryType;
typedef enum { SCREEN_BROWSER, SCREEN_SETTINGS } AppScreen;

typedef struct {
    char      name[256];
    char      fullPath[MAX_PATH];
    EntryType type;
} Entry;

typedef struct {
    const char *name;
    u32 bg, panel, accent, hilight, text, subtext, dir, barBg, barFg;
} Theme;

typedef enum { MODE_NORMAL, MODE_REPEAT_ALL, MODE_REPEAT_ONE, MODE_SHUFFLE } PlaybackMode;
extern PlaybackMode playbackMode;

// Global state declarations
extern Theme themes[];
extern const int THEME_COUNT;
extern int currentTheme;

extern Entry entries[MAX_FILES];
extern int entryCount;
extern int selected;
extern int scrollOffset;
extern char currentDir[MAX_PATH];

extern mpg123_handle *mh;
extern ndspWaveBuf waveBuf[2];
extern u8 *audioBuf;
extern bool playing, paused;
extern char nowPlayingName[256];
extern char nowPlayingPath[MAX_PATH];
extern char nowPlayingArtist[256];
extern char nowPlayingTitle[256];
extern char nowPlayingAlbum[256];
extern off_t trackLen;

extern int scrollTick;
extern int lastSelected;
extern AppScreen currentScreen;

extern C3D_RenderTarget *topTarget;
extern C3D_RenderTarget *botTarget;
extern C2D_TextBuf dynBuf;

extern C3D_Tex artTex;
extern C2D_Image artImage;
extern bool hasArt;

// Playback Queue (for auto-advance fix)
extern Entry playlist[MAX_FILES];
extern int playlistCount;

// Functions
void saveTheme(void);
void loadTheme(void);
void scanDir(const char *path);
bool goUp(void);

void startPlayback(const char *path);
void stopPlayback(void);
void playNext(void);
void playPrevious(void);
void fillAudio(void);
void autoAdvance(void);

void loadID3Tags(const char *mp3path);
void loadCoverArt(const char *mp3path);

void drawTopScreen(void);
void drawBotScreen(void);
void drawSettingsScreen(void);
extern float visualizerAmplitude[16];
extern void updateVisualizer(void);

// Theme macros
#define CLR_BG      themes[currentTheme].bg
#define CLR_PANEL   themes[currentTheme].panel
#define CLR_ACCENT  themes[currentTheme].accent
#define CLR_HILIGHT themes[currentTheme].hilight
#define CLR_TEXT    themes[currentTheme].text
#define CLR_SUBTEXT themes[currentTheme].subtext
#define CLR_DIR     themes[currentTheme].dir
#define CLR_BAR_BG  themes[currentTheme].barBg
#define CLR_BAR_FG  themes[currentTheme].barFg

#endif
