#include <GL/freeglut.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/Xatom.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "jvs.h"
#include "securityboard.h"

int gameModeWidth = -1;
int gameModeHeight = -1;

FGAPI int FGAPIENTRY glutEnterGameMode()
{
  char gameTitle[256] = {0};
  strcat(gameTitle, getGameName());
  strcat(gameTitle, " (GLUT)");
  glutCreateWindow(gameTitle);
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
  printf("glutGameModeString: %s\n", string);

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

  Window (*_XCreateWindow)(Display * display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual, unsigned long valueMask, XSetWindowAttributes *attributes) = dlsym(RTLD_NEXT, "XCreateWindow");

  width = getConfig()->width;
  height = getConfig()->height;

  // Ensure that the windows will respond with keyboard and mouse events
  attributes->event_mask = attributes->event_mask | KeyPressMask | KeyReleaseMask | PointerMotionMask;
  // attributes->override_redirect = False;

  Window window = _XCreateWindow(display, parent, x, y, width, height, border_width, depth, class, visual, valueMask, attributes);
  printf("%d %d %d %d\n", x, y, width, height);

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
  int (*_XGrabPointer)(Display * display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time) = dlsym(RTLD_NEXT, "XGrabPointer");
  int returnValue = _XGrabPointer(display, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor, time);
  XUngrabPointer(display, time);
  return returnValue;
}

int XGrabKeyboard(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time)
{
  int (*_XGrabKeyboard)(Display * display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time) = dlsym(RTLD_NEXT, "XGrabKeyboard");
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
  int (*_XStoreName)(Display * display, Window w, const char *window_name) = dlsym(RTLD_NEXT, "XStoreName");
  char gameTitle[256] = {0};
  strcat(gameTitle, getGameName());
  strcat(gameTitle, " (X11)");
  return _XStoreName(display, w, gameTitle);
}

int XNextEvent(Display *display, XEvent *event_return)
{

  int (*_XNextEvent)(Display * display, XEvent * event_return) = dlsym(RTLD_NEXT, "XNextEvent");
  int returnValue = _XNextEvent(display, event_return);
  switch (event_return->type)
  {

  case KeyRelease:
  case KeyPress:
  {
    switch (event_return->xkey.keycode)
    {
    case 28:
      securityBoardSetSwitch(BUTTON_TEST, event_return->type == KeyPress);
      break;
    case 39:
      securityBoardSetSwitch(BUTTON_SERVICE, event_return->type == KeyPress);
      break;
    case 14:
      incrementCoin(PLAYER_1, event_return->type == KeyPress);
      break;
    case 15:
      incrementCoin(PLAYER_2, event_return->type == KeyPress);
      break;
    case 111:
      setSwitch(PLAYER_1, BUTTON_UP, event_return->type == KeyPress);
      break;
    case 116:
      setSwitch(PLAYER_1, BUTTON_DOWN, event_return->type == KeyPress);
      break;
    case 113:
      setSwitch(PLAYER_1, BUTTON_LEFT, event_return->type == KeyPress);
      break;
    case 114:
      setSwitch(PLAYER_1, BUTTON_RIGHT, event_return->type == KeyPress);
      break;
    case 10:
      setSwitch(PLAYER_1, BUTTON_START, event_return->type == KeyPress);
      break;
    case 24:
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == KeyPress);
      break;
    case 25:
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == KeyPress);
      break;
    case 26:
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == KeyPress);
      break;
    case 27:
      setSwitch(PLAYER_1, BUTTON_4, event_return->type == KeyPress);
      break;
    default:
      break;
    }
  }
  break;

  case MotionNotify:
  {
    setAnalogue(ANALOGUE_1, ((double)event_return->xmotion.x / (double)getConfig()->width) * 255.0);
    setAnalogue(ANALOGUE_2, ((double)event_return->xmotion.y / (double)getConfig()->height) * 255.0);
  }
  break;

  case ButtonPress:
  case ButtonRelease:
  {
    switch (event_return->xbutton.button)
    {
    case 1: // Trigger
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == ButtonPress);
      break;
    case 3: // Reload
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == ButtonPress);
      break;
    case 9: // Gun Button
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == ButtonPress);
      break;
    default:
      break;
    }
  }
  break;
  }

  return returnValue;
}

int XSetStandardProperties(Display *display, Window window, const char *window_name, const char *icon_name, Pixmap icon_pixmap, char **argv, int argc, XSizeHints *hints)
{
  int (*_XSetStandardProperties)(Display * display, Window window, const char *window_name, const char *icon_name, Pixmap icon_pixmap, char **argv, int argc, XSizeHints *hints) = dlsym(RTLD_NEXT, "XSetStandardProperties");
  char gameTitle[256] = {0};
  strcat(gameTitle, getGameName());
  strcat(gameTitle, " (X11)");
  return _XSetStandardProperties(display, window, gameTitle, icon_name, icon_pixmap, argv, argc, hints);
}

Bool XF86VidModeSwitchToMode(Display *display, int screen, XF86VidModeModeInfo *modeline)
{
  return 0;
}
