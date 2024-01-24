#include <X11/X.h>
#define _GNU_SOURCE

#include "jvs.h"

#include "config.h"

#include <GL/freeglut.h>
#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
GameType gameType = SHOOTING;

int initInput() {
  gameType = getConfig()->gameType;
  return 0;
}

/**
 * Button mapping used for driving games
 */
int XNextEventDriving(Display *display, XEvent *event_return, int returnValue) {
  EmulatorKeyMap keymap = getConfig()->keymap;
  if (event_return->type == KeyPress || event_return->type == KeyRelease) {
    if (event_return->xkey.keycode == keymap.testKey)
      setSwitch(SYSTEM, BUTTON_TEST, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.serviceKey)
      setSwitch(PLAYER_1, BUTTON_SERVICE, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.p1CoinKey)
      incrementCoin(PLAYER_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.p2CoinKey)
      incrementCoin(PLAYER_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.analogueUp)
      setAnalogue(ANALOGUE_2,
                  event_return->type == KeyPress ? pow(2, 10) - 1 : 0);
    else if (event_return->xkey.keycode == keymap.analogueDown)
      setAnalogue(ANALOGUE_3,
                  event_return->type == KeyPress ? pow(2, 10) - 1 : 0);
    else if (event_return->xkey.keycode == keymap.analogueLeft)
      setAnalogue(ANALOGUE_1, event_return->type == KeyPress
                                  ? pow(2, 10) * 0.2
                                  : pow(2, 10) * 0.5);
    else if (event_return->xkey.keycode == keymap.analogueRight)
      setAnalogue(ANALOGUE_1, event_return->type == KeyPress
                                  ? pow(2, 10) * 0.8
                                  : pow(2, 10) * 0.5);
    else if (event_return->xkey.keycode == keymap.p1StartKey)
      setSwitch(PLAYER_1, BUTTON_START, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button1)
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button2)
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button3)
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button4)
      setSwitch(PLAYER_1, BUTTON_4, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonUp)
      setSwitch(PLAYER_1, BUTTON_UP, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonDown)
      setSwitch(PLAYER_1, BUTTON_DOWN, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonLeft)
      setSwitch(PLAYER_1, BUTTON_LEFT, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonRight)
      setSwitch(PLAYER_1, BUTTON_RIGHT, event_return->type == KeyPress);
  }
  return returnValue;
}

/**
 * Button mapping used for shooting games
 */
int XNextEventShooting(Display *display, XEvent *event_return,
                       int returnValue) {
  EmulatorKeyMap keymap = getConfig()->keymap;
  if (event_return->type == KeyPress || event_return->type == KeyRelease) {
    if (event_return->xkey.keycode == keymap.testKey)
      setSwitch(SYSTEM, BUTTON_TEST, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.serviceKey)
      setSwitch(PLAYER_1, BUTTON_SERVICE, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.p1CoinKey)
      incrementCoin(PLAYER_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.p2CoinKey)
      incrementCoin(PLAYER_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.p1StartKey)
      setSwitch(PLAYER_1, BUTTON_START, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button1)
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button2)
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button3)
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.button4)
      setSwitch(PLAYER_1, BUTTON_4, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonUp)
      setSwitch(PLAYER_1, BUTTON_UP, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonDown)
      setSwitch(PLAYER_1, BUTTON_DOWN, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonLeft)
      setSwitch(PLAYER_1, BUTTON_LEFT, event_return->type == KeyPress);
    else if (event_return->xkey.keycode == keymap.buttonRight)
      setSwitch(PLAYER_1, BUTTON_RIGHT, event_return->type == KeyPress);
  }
  if (event_return->type == MotionNotify) {
    setAnalogue(ANALOGUE_1, ((double)event_return->xmotion.x / (double)getConfig()->width) * pow(2, 10));
    setAnalogue(ANALOGUE_2, ((double)event_return->xmotion.y / (double)getConfig()->height) * pow(2, 10));
  }
  if (event_return->type == ButtonPress || event_return->type == ButtonRelease) {
    printf("Keymap: T %d - R %d - G %d\nButton: %d",keymap.triggerButton,keymap.reloadButton,keymap.gunButton,event_return->xbutton.button);
    if (event_return->xbutton.button == keymap.triggerButton)
      setSwitch(PLAYER_1, BUTTON_1, event_return->type == ButtonPress);
    else if (event_return->xbutton.button == keymap.reloadButton)
      setSwitch(PLAYER_1, BUTTON_2, event_return->type == ButtonPress);
    else if (event_return->xbutton.button == keymap.gunButton)
      setSwitch(PLAYER_1, BUTTON_3, event_return->type == ButtonPress);
  }
  return returnValue;
}

int XNextEvent(Display *display, XEvent *event_return) {

  int (*_XNextEvent)(Display *display, XEvent *event_return) =
      dlsym(RTLD_NEXT, "XNextEvent");
  int returnValue = _XNextEvent(display, event_return);

  // Return now if we're not emulating JVS
  if (!getConfig()->emulateJVS) {
    return returnValue;
  }

  // Select the appropriate input mapping depending on the game
  switch (gameType) {
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
