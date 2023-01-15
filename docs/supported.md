# Supported Titles

## Games that work

These games have been loaded at some point using this software, but this doesn't mean they currently work or there are instructions on how to get them running.

Working is defined by getting into attract mode and running the game, but not necessarily controlling it.

- After Burner Climax
- 2 Step 2 Spicy
- Ghost Squad Evolution
- Harley Davidson
- Outrun 2 SP SDX
- R-Tuned
- Race TV
- Rambo
- The House Of The Dead 4
- The House Of The Dead 4 Special
- The House Of The Dead Ex
- Let's Go Jungle Special (boots to ride error)
- Virtua Fighter 5 (boots to dongle error)

## Games that do not work or haven't been tested

- Hummer
- Initial D 4
- Initial D 5
- Let's Go Jungle
- Primevil
- Virtua Tennis 3

## How to run specific games

### Let's Go Jungle

No, this game will not start.

```LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. LD_PRELOAD=lindbergh.so TEA_DIR=`pwd` ./lgj_final```
