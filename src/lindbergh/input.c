#define _GNU_SOURCE

#include "jvs.h"

#include "config.h"

#include <GL/freeglut.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/Xatom.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>

GameType gameType = SHOOTING;

int initInput()
{
  gameType = getConfig()->gameType;
  return 0;
}

/**
 * Button mapping used for driving games
 */
int XNextEventDriving(Display *display, XEvent *event_return, int returnValue)
{
  switch (event_return->type)
  {

  case KeyRelease:
  case KeyPress:
  {
    switch (event_return->xkey.keycode)
    {
    case 28:
      setSwitch(SYSTEM, BUTTON_TEST, event_return->type == KeyPress);
      break;
    case 39:
      setSwitch(PLAYER_1, BUTTON_SERVICE, event_return->type == KeyPress);
      break;
    case 14:
      incrementCoin(PLAYER_1, event_return->type == KeyPress);
      break;
    case 15:
      incrementCoin(PLAYER_2, event_return->type == KeyPress);
      break;
    case 111: // Up
      setAnalogue(ANALOGUE_2, event_return->type == KeyPress ? pow(2, 10) - 1 : 0);
      break;
    case 116: // Down
      setAnalogue(ANALOGUE_3, event_return->type == KeyPress ? pow(2, 10) - 1 : 0);
      break;
    case 113: // Left
      setAnalogue(ANALOGUE_1, event_return->type == KeyPress ? pow(2, 10) * 0.2 : pow(2, 10) * 0.5);
      break;
    case 114: // Right
      setAnalogue(ANALOGUE_1, event_return->type == KeyPress ? pow(2, 10) * 0.8 : pow(2, 10) * 0.5);
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
    case 29:
      setSwitch(PLAYER_1, BUTTON_UP, event_return->type == KeyPress);
      break;
    case 30:
      setSwitch(PLAYER_1, BUTTON_DOWN, event_return->type == KeyPress);
      break;
    case 31:
      setSwitch(PLAYER_1, BUTTON_LEFT, event_return->type == KeyPress);
      break;
    case 32:
      setSwitch(PLAYER_1, BUTTON_RIGHT, event_return->type == KeyPress);
      break;
    default:
      break;
    }
  }
  break;
  }

  return returnValue;
}

/**
 * Button mapping used for shooting games
 */
int XNextEventShooting(Display *display, XEvent *event_return, int returnValue)
{
  switch (event_return->type)
  {

  case KeyRelease:
  case KeyPress:
  {
    switch (event_return->xkey.keycode)
    {
    case 28:
      setSwitch(SYSTEM, BUTTON_TEST, event_return->type == KeyPress);
      break;
    case 39:
      setSwitch(PLAYER_1, BUTTON_SERVICE, event_return->type == KeyPress);
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
    setAnalogue(ANALOGUE_1, ((double)event_return->xmotion.x / (double)getConfig()->width) * pow(2, 10));
    setAnalogue(ANALOGUE_2, ((double)event_return->xmotion.y / (double)getConfig()->height) * pow(2, 10));
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

int XNextEvent(Display *display, XEvent *event_return)
{

  int (*_XNextEvent)(Display *display, XEvent *event_return) = dlsym(RTLD_NEXT, "XNextEvent");
  int returnValue = _XNextEvent(display, event_return);

  // Return now if we're not emulating JVS
  if (!getConfig()->emulateJVS)
  {
    return returnValue;
  }

  // Select the appropriate input mapping depending on the game
  switch (gameType)
  {
  case DRIVING:
    return XNextEventDriving(display, event_return, returnValue);
    break;

  case SHOOTING:
  case FIGHTING:
  default:
    return XNextEventShooting(display, event_return, returnValue);
    break;
  }

  return returnValue;
}
