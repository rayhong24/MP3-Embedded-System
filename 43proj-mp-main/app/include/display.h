#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "fonts.h"
#include <stdbool.h>
#include "audio_datatypes.h"

#define PROGRESS_BAR_MAX UINT8_MAX

typedef enum {
  PREV,
  PLAY,
  NEXT,

  NUM_ELEMS
} eMainPageElem;

typedef enum {
  QUEUE,
  MAIN,
  FILES,

  NUM_PAGES
} eDisplay_Page_t;

/** Options for display layout */
typedef struct {
  sFONT* titleFont;
  sFONT* songNameFont;
  sFONT* artistFont;
  sFONT* timeFont;
  uint16_t volumeBarHeight;
} Display_Opts_t;

/** Structure containing request to display domain level information. */
typedef struct {
  eDisplay_Page_t page;
  char* title;
  char* songName;
  char* author;
  double currPlaytime;
  double totalPlaytime;
  bool isPlaying;
  int volume; // 0 - 255 (0% - 100%)
  eMainPageElem elemSelected;
} Display_Request_t;

typedef struct {
  char* title;
  char** songNames;
  double* songLengths;
  int numSongs;
  int selectedSong;
} File_List_Display_Request_t;

/**
 * Initializes the display and its layout using the provided options.
 * @param layout The layout options used to compute display heights.
 */
void Display_init(Display_Opts_t layout);

/**
 * Updates the screen.
 * @param request
 */
void Display_updateHomeScreen(Display_Request_t request);

void Display_updateSongSelectScreen(File_List_Display_Request_t request);

void Display_updateSongListScreen(char** songNames, int numSongs, int selectedSong, int volume, bool, musicMetadata_t*, double playtime);

void Display_updateQueueScreen(File_List_Display_Request_t request);

void Display_cleanup(void);

void Display_drawSong(char*, int);


#endif
