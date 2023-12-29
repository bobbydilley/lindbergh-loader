#define _GNU_SOURCE
#include <GL/freeglut.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/Xatom.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include "config.h"
#include "jvs.h"
#include "securityboard.h"

int gameModeWidth = -1;
int gameModeHeight = -1;

void *glutMainLoopThread()
{
  glutMainLoop();
  return NULL;
}

FGAPI int FGAPIENTRY glutEnterGameMode()
{
  char gameTitle[256] = {0};
  strcat(gameTitle, getGameName());
  glutCreateWindow(gameTitle);

  // Outrun doesn't run the glutMainLoop through, so we'll do that here
  Game game = getConfig()->game;
  if (game == OUTRUN_2_SP_SDX || game == OUTRUN_2_SP_SDX_TEST || game == OUTRUN_2_SP_SDX_REVA || game == OUTRUN_2_SP_SDX_REVA_TEST)
  {
    pthread_t glutMainLoopID;
    pthread_create(&glutMainLoopID, NULL, &glutMainLoopThread, NULL);
  }

  return 1;
}

FGAPI void FGAPIENTRY glutLeaveGameMode()
{
  glutDestroyWindow(glutGetWindow());
  return;
}

FGAPI void FGAPIENTRY glutSetCursor(int cursor)
{
  return;
}

FGAPI void FGAPIENTRY glutGameModeString(const char *string)
{
  // printf("glutGameModeString: %s\n", string);

  char gameModeString[1024];
  strcpy(gameModeString, string);

  char *widthString = gameModeString;
  char *heightString = NULL;

  for (int i = 0; i < 1024; i++)
  {
    if (gameModeString[i] == 'x')
    {
      gameModeString[i] = 0;
      heightString = &gameModeString[i + 1];
    }

    if (gameModeString[i] == ':')
    {
      gameModeString[i] = 0;
      break;
    }
  }

  int width = atoi(widthString);
  int height = atoi(heightString);

  if (getConfig()->width != width || getConfig()->height != height)
  {
    printf("Warning: Game is overriding resolution settings to %dX%d\n", width, height);
    getConfig()->width = width;
    getConfig()->height = height;
  }
}

Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual, unsigned long valueMask, XSetWindowAttributes *attributes)
{

  Window (*_XCreateWindow)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual, unsigned long valueMask, XSetWindowAttributes *attributes) = dlsym(RTLD_NEXT, "XCreateWindow");

  width = getConfig()->width;
  height = getConfig()->height;

  // Ensure that the windows will respond with keyboard and mouse events
  attributes->event_mask = attributes->event_mask | KeyPressMask | KeyReleaseMask | PointerMotionMask;
  // attributes->override_redirect = False;

  Window window = _XCreateWindow(display, parent, x, y, width, height, border_width, depth, class, visual, valueMask, attributes);
  printf("The resolution is %dx%d \n", width, height);

  if (getConfig()->fullscreen)
  {
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", true);
    Atom wm_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", true);
    XChangeProperty(display, window, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_fullscreen, 1);
  }

  return window;
}

int XGrabPointer(Display *display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time)
{
  int (*_XGrabPointer)(Display *display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time) = dlsym(RTLD_NEXT, "XGrabPointer");
  int returnValue = _XGrabPointer(display, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor, time);
  XUngrabPointer(display, time);
  return returnValue;
}

int XGrabKeyboard(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time)
{
  int (*_XGrabKeyboard)(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time) = dlsym(RTLD_NEXT, "XGrabKeyboard");
  int returnValue = _XGrabKeyboard(display, grab_window, owner_events, pointer_mode, keyboard_mode, time);
  XUngrabKeyboard(display, time);
  return returnValue;
}

int XDefineCursor(Display *display, Window w, Cursor cursor)
{
  return 0;
}

int XStoreName(Display *display, Window w, const char *window_name)
{
  int (*_XStoreName)(Display *display, Window w, const char *window_name) = dlsym(RTLD_NEXT, "XStoreName");
  char gameTitle[256] = {0};
  strcat(gameTitle, getGameName());
  return _XStoreName(display, w, gameTitle);
}

int XSetStandardProperties(Display *display, Window window, const char *window_name, const char *icon_name, Pixmap icon_pixmap, char **argv, int argc, XSizeHints *hints)
{
  int (*_XSetStandardProperties)(Display *display, Window window, const char *window_name, const char *icon_name, Pixmap icon_pixmap, char **argv, int argc, XSizeHints *hints) = dlsym(RTLD_NEXT, "XSetStandardProperties");
  char gameTitle[256] = {0};
  strcat(gameTitle, getGameName());
  return _XSetStandardProperties(display, window, gameTitle, icon_name, icon_pixmap, argv, argc, hints);
}

Bool XF86VidModeSwitchToMode(Display *display, int screen, XF86VidModeModeInfo *modesinfo)
{
  return 0;
}

int XF86VidModeGetAllModeLines(Display *display, int screen, int *modecount_return, XF86VidModeModeInfo ***modesinfo)
{
  int (*_XF86VidModeGetAllModeLines)(Display *display, int screen, int *modecount_return, XF86VidModeModeInfo ***modesinfo) = dlsym(RTLD_NEXT, "XF86VidModeGetAllModeLines");

  if (_XF86VidModeGetAllModeLines(display, screen, modecount_return, modesinfo) != 1)
  {
    printf("Error: Could not get list of screen modes.\n");
    exit(1);
  }
  else
  {
    XF86VidModeModeInfo **modes = *modesinfo;
    modes[0]->hdisplay = getConfig()->width;
    modes[0]->vdisplay = getConfig()->height;
  }
  return true;
}

typedef unsigned int uint;

int glXSwapIntervalSGI(int interval)
{
  return 0;
}

int glXGetVideoSyncSGI(uint *count)
{
  static unsigned int frameCount = 0;
  // TODO: Framecount should depend on current system time
  *count = (frameCount++) / 2; // NOTE: Keeps the same frame for 2 calls
  return 0;
}

int glXGetRefreshRateSGI(unsigned int *rate)
{             // TODO: need an actual prototype
  *rate = 60; // TODO: what does this function return?
  return 0;
}

void glGenFencesNV(int n, uint *fences)
{
  static unsigned int curf = 1;
  while (n--)
  {
    *fences++ = curf++;
  }
  return;
}

void glDeleteFencesNV(int a, const uint *b)
{
  return;
}
