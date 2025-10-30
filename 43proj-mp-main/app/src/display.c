#include "display.h"
#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include <stdbool.h>
#include <signal.h>
#include <assert.h>
#include "audio_datatypes.h"

#define HIGHLIGHT_COLOUR MAGENTA
#define ACCENT_COLOUR BRRED
#define ACCENT_COLOUR2 RED

#define DISPLAY_X_MARGIN 5
#define DISPLAY_Y_MARGIN 5
#define LINE_SPACING 5

// Values on Home Screen
#define HEADER_LINE_LEN 150
#define PROGESS_HEIGHT  15
#define PLAYBACK_STATE_HEIGHT  50

#define SONG_NAME_LEN_PER_LINE 12
#define ARTIST_LEN_ON_LINE 12

#define PLAY_ICON_SIZE 40
#define PREV_ICON_HEIGHT 25
#define PREV_ICON_WIDTH 30
#define PREV_ICON_OUTLINE_WIDTH 3
#define PREV_ICON_SMALLER_TRIANGLE_RATIO (3.0f/4.0f)

#define MAX_VOLUME 100
#define VOLUME_MAX_HEIGHT 75
#define VOLUME_BAR_WIDTH 15
#define VOLUME_TOP_PADDING 30
#define VOLUME_OUTLINE_WIDTH 3

// values for songList screen
#define SONG_NAME_MAX_LEN 20
#define MAX_SONGS_DISP 7
#define SONG_BOX_HEIGHT 25 
#define SONG_BOX_WIDTH 210

// prototypes
static void Display_drawString(char* str, sFONT* font, int line);
static UWORD Display_computeLineYStart(int line);
static bool splitAndTruncate(char* input, char *out1, char *out2);
static void getSongDispStr(char* orig, char* dest, int bufLen);

static void drawPauseIcon(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, bool);
static void drawPlayIcon(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, bool);
static void drawCommands(bool isPlaying, eMainPageElem elemSelected);
static void drawPlayback(double currPlaytime, double totalPlaytime);

static void drawVolumeVertical(int volume);
static void drawSongList(char** songNames, int numSongs, int selectedSong, UWORD);
static void drawQueueList(char** songNames, int numSongs, int selectedSong, UWORD startY);

static const UDOUBLE IMAGE_SIZE = LCD_1IN54_HEIGHT * LCD_1IN54_WIDTH * 2;

typedef enum eLines
{
  TITLE_LINE,
  HEADER_SEP_LINE,
  SONG_NAME_LINE,
  AUTHOR_LINE,
  PROGRESS_BAR_LINE,
  NUM_LINES
} eLines;

/** Structure representing the layout of the display */
typedef struct {
  struct Display_Layout_Line_t {
    uint16_t height;
    uint16_t spacing;
  } lines[NUM_LINES];
} Display_Layout_t;

// For volume (not currently used)
static bool s_initialized = false;
static Display_Opts_t s_opts;
static Display_Layout_t s_layout;
static UWORD *s_imageBuffer;

void Display_updateHomeScreen(Display_Request_t request)
{
  assert(s_initialized);

  // initialize the RAM frame buffer to be blank
  Paint_NewImage(s_imageBuffer, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
  Paint_Clear(WHITE);

  Display_drawString(request.title, s_opts.titleFont, TITLE_LINE); // title on line 0

  UWORD currY = Display_computeLineYStart(HEADER_SEP_LINE);
  Paint_DrawLine(DISPLAY_X_MARGIN, currY, DISPLAY_X_MARGIN + HEADER_LINE_LEN, currY, HIGHLIGHT_COLOUR, 2, LINE_STYLE_SOLID);

  currY += s_layout.lines[HEADER_SEP_LINE].spacing;

  char songNameLine1[SONG_NAME_LEN_PER_LINE + 1];
  char songNameLine2[SONG_NAME_LEN_PER_LINE + 1];

  bool secondLineNeeded = splitAndTruncate(request.songName, songNameLine1, songNameLine2);
  Paint_DrawString_EN(DISPLAY_X_MARGIN, currY, songNameLine1, s_opts.songNameFont, WHITE, BLACK);
  currY += s_layout.lines[SONG_NAME_LINE].height;
  if (secondLineNeeded)
  {
    Paint_DrawString_EN(DISPLAY_X_MARGIN, currY, songNameLine2, s_opts.songNameFont, WHITE, BLACK);
    currY += s_layout.lines[SONG_NAME_LINE].height;
  }

  char artistStr[ARTIST_LEN_ON_LINE];
  getSongDispStr(request.author, artistStr, ARTIST_LEN_ON_LINE);
  Paint_DrawString_EN(DISPLAY_X_MARGIN, currY, artistStr, s_opts.artistFont, WHITE, HIGHLIGHT_COLOUR);

  drawVolumeVertical(request.volume);

  drawPlayback(request.currPlaytime, request.totalPlaytime);

  drawCommands(request.isPlaying, request.elemSelected);

  LCD_1IN54_Display(s_imageBuffer);
}

void Display_updateSongSelectScreen(File_List_Display_Request_t request)
{
  Paint_NewImage(s_imageBuffer, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
  Paint_Clear(WHITE);

  Display_drawString(request.title, s_opts.titleFont, TITLE_LINE); // title on line 0

  UWORD currY = Display_computeLineYStart(HEADER_SEP_LINE);
  Paint_DrawLine(DISPLAY_X_MARGIN, currY, DISPLAY_X_MARGIN + HEADER_LINE_LEN, currY, HIGHLIGHT_COLOUR, 2, LINE_STYLE_SOLID);
  drawSongList(request.songNames, request.numSongs, request.selectedSong, currY);

  LCD_1IN54_Display(s_imageBuffer);
}

void Display_updateQueueScreen(File_List_Display_Request_t request)
{
  Paint_NewImage(s_imageBuffer, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
  Paint_Clear(WHITE);

  Display_drawString(request.title, s_opts.titleFont, TITLE_LINE); // title on line 0

  UWORD currY = Display_computeLineYStart(HEADER_SEP_LINE);
  Paint_DrawLine(DISPLAY_X_MARGIN, currY, DISPLAY_X_MARGIN + HEADER_LINE_LEN, currY, HIGHLIGHT_COLOUR, 2, LINE_STYLE_SOLID);
  drawQueueList(request.songNames, request.numSongs, request.selectedSong, currY);

  LCD_1IN54_Display(s_imageBuffer);
}


// For song list
// static Display_Layout_t listScreenLayout;

static void getSongDispStr(char* orig, char* dest, int bufLen)
{
  if ((int)strlen(orig) > bufLen)
  {
    snprintf(dest, bufLen-4, "%s", orig);
    snprintf(dest + strlen(dest), sizeof(dest) - strlen(dest), "...");
  }
  else
  {
    snprintf(dest, bufLen, "%s", orig);
    // printf("%s\n", orig);
  }


}

/** Computes Y offset for a given line. */
static UWORD Display_computeLineYStart(int line)
{
  UWORD yInfoDisp = DISPLAY_Y_MARGIN;
  for(int i = 0; i < line; i++)
    yInfoDisp += s_layout.lines[i].height + s_layout.lines[i].spacing;
  
  return yInfoDisp;
}


/**
 * Displays a string.
 * WARNING: Don't print strings with `\n`; will crash!
 * @param font The font to use.
 * @param line The line to display on.
 */
static void Display_drawString(char* str, sFONT* font, int line)
{
  char dispStr[SONG_NAME_MAX_LEN];
  getSongDispStr(str, dispStr, SONG_NAME_MAX_LEN);

  UWORD x = DISPLAY_X_MARGIN;
  UWORD yInfoDisp = Display_computeLineYStart(line);
  Paint_DrawString_EN(x, yInfoDisp, dispStr, font, WHITE, BLACK);
}


/**
 * Initialize the layout with domain information.
 */
static void Display_layoutInit(Display_Opts_t opts)
{
  Display_Layout_t layout = {
    .lines = {
      [TITLE_LINE] = { .height = opts.titleFont->Height, .spacing = 2},
      [HEADER_SEP_LINE] = { .height = 2 , .spacing = 8},
      [SONG_NAME_LINE] = { .height = opts.songNameFont->Height , .spacing = LINE_SPACING},
      [AUTHOR_LINE] = { .height = opts.artistFont->Height , .spacing = LINE_SPACING},
      [PROGRESS_BAR_LINE] = { .height = opts.volumeBarHeight }
    }
  };
  s_opts = opts;
  s_layout = layout;
}

// static void Display_songListInit(Display_Opts_t opts)

void Display_init(Display_Opts_t opts)
{
  assert(!s_initialized);

  // exception handling: ctrl + c
  signal(SIGINT, Handler_1IN54_LCD);

  // module init
  if(DEV_ModuleInit() != 0){
    DEV_ModuleExit();
    exit(0);
  }

  // lcd init
  DEV_Delay_ms(2000);
  LCD_1IN54_Init(HORIZONTAL);
  LCD_1IN54_Clear(WHITE);
  LCD_SetBacklight(1023);

  if((s_imageBuffer = (UWORD*) malloc(IMAGE_SIZE)) == NULL) {
    perror("Failed to apply for black memory");
    exit(0);
  }

  Display_layoutInit(opts);

  s_initialized = true;
}

static void drawPlayback(double currPlaytime, double totalPlaytime)
{
  UWORD yStart = LCD_1IN54_HEIGHT - DISPLAY_Y_MARGIN - PROGESS_HEIGHT ;
  UWORD timeStrWidth = s_opts.timeFont->Width * 4;

  // curr playtime
  int numMins = (int)currPlaytime / 60;
  int numSecs = (int)currPlaytime % 60;

  char currPlaytimeStr[16];
  snprintf(currPlaytimeStr, 16, "%01d:%02d", numMins, numSecs);
  Paint_DrawString_EN(DISPLAY_X_MARGIN, yStart, currPlaytimeStr, s_opts.timeFont, WHITE, BLACK);

  // tot playtime
  int numMinsTot = (int)totalPlaytime / 60;
  int numSecsTot = (int)totalPlaytime % 60;

  char totalPlaytimeStr[16];
  snprintf(totalPlaytimeStr, 16, "%01d:%02d", numMinsTot, numSecsTot);
  UWORD xTotStart = LCD_1IN54_WIDTH - DISPLAY_X_MARGIN - timeStrWidth;
  Paint_DrawString_EN(xTotStart, yStart, totalPlaytimeStr, s_opts.timeFont, WHITE, BLACK);

  // bar
  UWORD barPadding = 8;
  UWORD xBarStart = DISPLAY_X_MARGIN + timeStrWidth + barPadding;

  UWORD barHeight = 2;
  UWORD xBarEnd = xTotStart - barPadding;
  UWORD yBarBase = yStart + s_opts.timeFont->Height/2;
  Paint_DrawLine(xBarStart, yBarBase, xBarEnd, yBarBase, BLACK, barHeight, LINE_STYLE_SOLID);


  UWORD barHeightFill = 3;
  if (totalPlaytime > 0)
  {
    UWORD xBarFillEnd = xBarStart + (xBarEnd - xBarStart) * (currPlaytime/totalPlaytime);
    Paint_DrawLine(xBarStart, yBarBase, xBarFillEnd, yBarBase, ACCENT_COLOUR, barHeightFill, LINE_STYLE_SOLID);
    Paint_DrawCircle(xBarFillEnd, yBarBase, barHeightFill, ACCENT_COLOUR, 0, DRAW_FILL_FULL);
  }
  else
  {
    Paint_DrawCircle(xBarStart, yBarBase, barHeightFill, ACCENT_COLOUR, 0, DRAW_FILL_FULL);
  }
}

static void lineHelper(UWORD xStart, UWORD yStart, UWORD yEnd, int i, bool isBorderLine, int borderColour, int fillColour)
{
  UWORD yStartNew = isBorderLine ? yStart : yStart+PREV_ICON_OUTLINE_WIDTH;
  UWORD yEndNew = isBorderLine ? yEnd : yEnd-PREV_ICON_OUTLINE_WIDTH;
  int colour = isBorderLine ? borderColour : fillColour;
  if (yEndNew < yStartNew) return;

  Paint_DrawLine(xStart+i, yStartNew, xStart+i, yEndNew, colour, 1, LINE_STYLE_SOLID);
}

static void drawPrev(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, bool isRev, bool isSelected)
{
  UWORD width = xEnd-xStart;

  UWORD yCenter = yStart + (yEnd-yStart) / 2;

  int borderColour = isSelected ? HIGHLIGHT_COLOUR : BLACK;
  int fillColour = WHITE;
  // left triangle
  for (int i=0; i<(width/2); i++)
  {
    UWORD ySmallLen;
    UWORD yNewStart;
    UWORD yNewEnd;
    if (!isRev)
    {
      ySmallLen = ((yCenter+i) - (yCenter-i)) * PREV_ICON_SMALLER_TRIANGLE_RATIO;
      yNewStart = yCenter - ySmallLen/2;
      yNewEnd = yCenter + ySmallLen/2;
    }
    else
    {
      int j = width/2 - 1 - i;
      ySmallLen = ((yCenter+j) - (yCenter-j));
      yNewStart = yCenter - ySmallLen/2;
      yNewEnd = yCenter + ySmallLen/2;
    }


    lineHelper(xStart, yNewStart, yNewEnd, i, true, borderColour, fillColour);
    if ((i <= (width)/2 - PREV_ICON_OUTLINE_WIDTH && !isRev) ||
      (i >= PREV_ICON_OUTLINE_WIDTH-1 && isRev)
    )
    {
      lineHelper(xStart, yNewStart, yNewEnd, i, false, borderColour, fillColour);
    }
  }

  // right triangle
  UWORD xRightStart = xStart + width/2;
  for (int i=0; i<(width/2); i++)
  {
    UWORD ySmallLen;
    UWORD yNewStart;
    UWORD yNewEnd;
    if (!isRev)
    {
      ySmallLen = ((yCenter+i) - (yCenter-i)) ;
      yNewStart = yCenter - ySmallLen/2;
      yNewEnd = yCenter + ySmallLen/2;
    }
    else
    {
      int j = width/2 - 1 - i;
      ySmallLen = ((yCenter+j) - (yCenter-j))* PREV_ICON_SMALLER_TRIANGLE_RATIO;
      yNewStart = yCenter - ySmallLen/2;
      yNewEnd = yCenter + ySmallLen/2;
    }


    lineHelper(xRightStart, yNewStart, yNewEnd, i, true, borderColour, fillColour);
    if ((i <= (width)/2 - PREV_ICON_OUTLINE_WIDTH && !isRev) ||
      (i >= PREV_ICON_OUTLINE_WIDTH-1 && isRev)
    )
    {
      lineHelper(xRightStart, yNewStart, yNewEnd, i, false, borderColour, fillColour);
    }
  }

}

static void drawCommands(bool isPlaying, eMainPageElem elemSelected)
{
  UWORD yEnd = LCD_1IN54_HEIGHT - PROGESS_HEIGHT - DISPLAY_Y_MARGIN - 30;
  UWORD yStart = yEnd - PLAYBACK_STATE_HEIGHT;
  UWORD xStart = DISPLAY_X_MARGIN;
  UWORD xEnd = LCD_1IN54_WIDTH - DISPLAY_X_MARGIN;
  UWORD xCenter = xStart + (xEnd-xStart) / 2;
  UWORD yCenter = yStart + (yEnd-yStart) / 2;

  UWORD xStartPlayIcon = xCenter - PLAY_ICON_SIZE/2;
  UWORD xEndPlayIcon = xCenter + PLAY_ICON_SIZE/2;

  UWORD yStartPlayIcon = yCenter - PLAY_ICON_SIZE/2;
  UWORD yEndPlayIcon = yCenter + PLAY_ICON_SIZE/2;

  // pause/play
  if (isPlaying)
  {
    drawPauseIcon(xStartPlayIcon, yStartPlayIcon, xEndPlayIcon, yEndPlayIcon, elemSelected==PLAY);
  }
  else
  {
    drawPlayIcon(xStartPlayIcon, yStartPlayIcon, xEndPlayIcon, yEndPlayIcon, elemSelected==PLAY);
  }

  // prev
  UWORD xCenterPrev = DISPLAY_X_MARGIN + (xStartPlayIcon - DISPLAY_X_MARGIN)/2;
  UWORD xStartPrev = xCenterPrev - PREV_ICON_WIDTH/2;
  UWORD xEndPrev = xCenterPrev + PREV_ICON_WIDTH/2;
  UWORD yStartPrev = yCenter - PREV_ICON_HEIGHT/2;
  UWORD yEndPrev = yCenter + PREV_ICON_HEIGHT/2;

  drawPrev(xStartPrev, yStartPrev, xEndPrev, yEndPrev, false, elemSelected==PREV);

  UWORD xCenterNext = xEndPlayIcon + (LCD_1IN54_WIDTH - DISPLAY_X_MARGIN - xEndPlayIcon) / 2;
  UWORD xStartNext = xCenterNext - PREV_ICON_WIDTH/2;
  UWORD xEndNext = xCenterNext + PREV_ICON_WIDTH/2;

  drawPrev(xStartNext, yStartPrev, xEndNext, yEndPrev, true, elemSelected==NEXT);
}

static bool splitAndTruncate(char* input, char *out1, char *out2)
{
    strncpy(out1, input, SONG_NAME_LEN_PER_LINE);
    out1[SONG_NAME_LEN_PER_LINE] = '\0';

    if (strlen(input) > SONG_NAME_LEN_PER_LINE) {
        strncpy(out2, input + SONG_NAME_LEN_PER_LINE, SONG_NAME_LEN_PER_LINE);
        out2[10] = '\0';

        if (strlen(input) > 2*SONG_NAME_LEN_PER_LINE) {
            int len = strlen(out2);
            if (len > 3) {
                strcpy(out2 + len - 3, "...");
            } else {
                strcpy(out2, "...");
            }
        }
        return true; // Second part was needed
    } else {
        out2[0] = '\0';
        return false;
    }
}


static void drawSongList(char** songNames, int numSongs, int selectedSong, UWORD startY)
{
  UWORD currY = startY+SONG_BOX_HEIGHT;
  for (int i=0; i<MAX_SONGS_DISP; i++)
  {
    UWORD yStart = currY+(SONG_BOX_HEIGHT*i);
    UWORD yEnd = yStart+SONG_BOX_HEIGHT;
    UWORD xStart = DISPLAY_X_MARGIN;
    UWORD xEnd = xStart + SONG_BOX_WIDTH;

    // draw border
    Paint_DrawRectangle(xStart, yStart, xEnd, yEnd, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    if (i < numSongs)
    {
      char songName[SONG_NAME_MAX_LEN];
      getSongDispStr(songNames[i], songName, SONG_NAME_MAX_LEN - 1);
      uint16_t strColour = i == selectedSong ? HIGHLIGHT_COLOUR : BLACK;

      Paint_DrawString_EN(xStart + 10, yStart, songName, s_opts.artistFont, WHITE, strColour);
    }
  }
}

static void drawQueueList(char** songNames, int numSongs, int selectedSong, UWORD startY)
{
  UWORD currY = startY+SONG_BOX_HEIGHT;
  for (int i=0; i<MAX_SONGS_DISP; i++)
  {
    UWORD yStart = currY+(SONG_BOX_HEIGHT*i);
    UWORD yEnd = yStart+SONG_BOX_HEIGHT;
    UWORD xStart = DISPLAY_X_MARGIN;
    UWORD xEnd = xStart + SONG_BOX_WIDTH;

    // draw border
    Paint_DrawRectangle(xStart, yStart, xEnd, yEnd, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    if (i < numSongs)
    {
      char songName[SONG_NAME_MAX_LEN];
      getSongDispStr(songNames[i], songName, SONG_NAME_MAX_LEN - 2);
      uint16_t strColour = i == selectedSong ? HIGHLIGHT_COLOUR : BLACK;

      char numStr[2];
      snprintf(numStr, 2, "%1d", i+1);

      Paint_DrawString_EN(xStart + 3, yStart+3, numStr, s_opts.titleFont, WHITE, ACCENT_COLOUR);
      Paint_DrawString_EN(xStart + 25, yStart+3, songName, s_opts.artistFont, WHITE, strColour);
    }
  }
}

static void drawVolumeVertical(int volume)
{
  float volumeFilledRatio = ((float)volume / (float)MAX_VOLUME);
  UWORD filledHeight = (UWORD)(volumeFilledRatio * VOLUME_MAX_HEIGHT);

  UWORD xEnd = LCD_1IN54_WIDTH - DISPLAY_X_MARGIN;

  UWORD volumeTop = VOLUME_TOP_PADDING;
  UWORD volumeBot = volumeTop+VOLUME_MAX_HEIGHT;


  // draw outer rectangle
  Paint_DrawRectangle(xEnd-VOLUME_BAR_WIDTH, volumeTop, xEnd, volumeBot, ACCENT_COLOUR2, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);

  // draw inner rectangle
  Paint_DrawRectangle(
    xEnd-VOLUME_BAR_WIDTH+(VOLUME_OUTLINE_WIDTH), 
    volumeBot - filledHeight + VOLUME_OUTLINE_WIDTH,
    xEnd-(VOLUME_OUTLINE_WIDTH), 
    volumeBot - VOLUME_OUTLINE_WIDTH, ACCENT_COLOUR, DOT_PIXEL_2X2, DRAW_FILL_FULL);


}


static void drawPauseIcon(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, bool isSelected)
{
  UWORD width = xEnd - xStart;
  UWORD height = yEnd - yStart;

  UWORD sideLength = width < height ? width : height;

  UWORD xFirstThirdEnd = xStart + (xEnd-xStart)/3;
  UWORD xSecondThirdEnd = xStart + (xEnd-xStart)*2/3;

  UWORD yCenter = yStart + (yEnd - yStart) / 2;

  UWORD yIconStart = yCenter - (sideLength / 2);
  UWORD yIconEnd = yCenter + (sideLength / 2);


  int colour = isSelected ? HIGHLIGHT_COLOUR : BLACK;
  Paint_DrawRectangle(xStart, yIconStart, xFirstThirdEnd, yIconEnd, colour, DOT_PIXEL_2X2, DRAW_FILL_FULL);
  Paint_DrawRectangle(xSecondThirdEnd, yIconStart, xEnd, yIconEnd, colour, DOT_PIXEL_2X2, DRAW_FILL_FULL);
}

static void drawPlayIcon(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, bool isSelected)
{
  UWORD width = xEnd - xStart;
  UWORD height = yEnd - yStart;

  UWORD sideLength = width < height ? width : height;

  UWORD yCenter = yStart + (yEnd - yStart) / 2;

  UWORD yIconStart = yCenter - (sideLength / 2);
  UWORD yIconEnd = yCenter + (sideLength / 2);

  int colour = isSelected ? HIGHLIGHT_COLOUR : BLACK;

  for (int i=0; i<sideLength; i++)
  {
    Paint_DrawRectangle(xStart+i, yIconStart+(i/2), xStart+i, yIconEnd-(i/2), colour, DOT_PIXEL_2X2, DRAW_FILL_FULL);
  }
}

void Display_cleanup(void)
{
  assert(s_initialized);

  LCD_1IN54_Clear(BLACK);
  LCD_SetBacklight(0);

  free(s_imageBuffer);
  s_imageBuffer = NULL;
  DEV_ModuleExit();

  s_initialized = false;
}
