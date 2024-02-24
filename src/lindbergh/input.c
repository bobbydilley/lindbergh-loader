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
  KeyMapping keymap = getConfig()->keymap;
  if (event_return->type == KeyPress || event_return->type == KeyRelease) 
  {
    if (event_return->xkey.keycode == keymap.test)
      setSwitch(SYSTEM, BUTTON_TEST, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.service)
      setSwitch(PLAYER_1, BUTTON_SERVICE, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.coin)
      incrementCoin(PLAYER_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player2.coin)
      incrementCoin(PLAYER_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.up)
      setAnalogue(ANALOGUE_2,
                  event_return->type == KeyPress ? pow(2, 10) - 1 : 0);
    else if (event_return->xkey.keycode == keymap.player1.down)
      setAnalogue(ANALOGUE_3,
                  event_return->type == KeyPress ? pow(2, 10) - 1 : 0);
    else if (event_return->xkey.keycode == keymap.player1.left)
      setAnalogue(ANALOGUE_1, event_return->type == KeyPress
                                  ? pow(2, 10) * 0.2
                                  : pow(2, 10) * 0.5);
    else if (event_return->xkey.keycode == keymap.player1.right)
      setAnalogue(ANALOGUE_1, event_return->type == KeyPress
                                  ? pow(2, 10) * 0.8
                                  : pow(2, 10) * 0.5);
    else if (event_return->xkey.keycode == keymap.player1.start)
      setSwitch(PLAYER_1, BUTTON_START, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button1)
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button2)
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button3)
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button4)
      setSwitch(PLAYER_1, BUTTON_4, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.up)
      setSwitch(PLAYER_1, BUTTON_UP, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.down)
      setSwitch(PLAYER_1, BUTTON_DOWN, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.left)
      setSwitch(PLAYER_1, BUTTON_LEFT, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.right)
      setSwitch(PLAYER_1, BUTTON_RIGHT, event_return->type == KeyPress);
  }
  return returnValue;
}

/**
 * Button mapping used for shooting games
 */
int XNextEventShooting(Display *display, XEvent *event_return, int returnValue) 
{
  KeyMapping keymap = getConfig()->keymap;
  if (event_return->type == KeyPress || event_return->type == KeyRelease) 
  {
    if (event_return->xkey.keycode == keymap.test)
      setSwitch(SYSTEM, BUTTON_TEST, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.service)
      setSwitch(PLAYER_1, BUTTON_SERVICE, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.coin)
      incrementCoin(PLAYER_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player2.coin)
      incrementCoin(PLAYER_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.start)
      setSwitch(PLAYER_1, BUTTON_START, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button1)
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button2)
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button3)
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.button4)
      setSwitch(PLAYER_1, BUTTON_4, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.up)
      setSwitch(PLAYER_1, BUTTON_UP, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.down)
      setSwitch(PLAYER_1, BUTTON_DOWN, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.left)
      setSwitch(PLAYER_1, BUTTON_LEFT, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.player1.right)
      setSwitch(PLAYER_1, BUTTON_RIGHT, event_return->type == KeyPress);
  } 
  else if (event_return->type == MotionNotify) 
  {
    setAnalogue(ANALOGUE_1,
                ((double)event_return->xmotion.x / (double)getConfig()->width) *
                    pow(2, 10));
    setAnalogue(ANALOGUE_2, ((double)event_return->xmotion.y /
                             (double)getConfig()->height) *
                                pow(2, 10));
  } 
  else if (event_return->type == ButtonPress || event_return->type == ButtonRelease) 
  {
    // Trigger
    if (event_return->xbutton.button == 1)
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == ButtonPress);
    // Reload
    else if (event_return->xbutton.button == 3)
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == ButtonPress);
    // Gun button
    else if (event_return->xbutton.button == 2)
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == ButtonPress);
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
