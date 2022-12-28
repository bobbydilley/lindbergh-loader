# Supported Titles

## Games that work

These games have been loaded at some point using this software, but this doesn't mean they currently work or there are instructions on how to get them running.

Working is defined by getting into attract mode and running the game, but not necciserily controlling it.

- The House Of The Dead 4
- The House Of The Dead 4 Special
- The House Of The Dead Ex
- Harley Davidson
- Rambo
- 2 Step 2 Spicy
- Ghost Squad
- Outrun 2 SP SDX
- Race TV

## Games that do not work or haven't been tested

- Let's Go Jungle
- Let's Go Jungle Special
- After Burner Climax
- Virtua Fighter
- Virtua Tennis 3
- Primevil
- Initial D 4
- Initial D 5
- Hummer
- R-Tuned

## How to run specific games

### Let's Go Jungle

Not this game will not start.

```LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. LD_PRELOAD=lindbergh.so TEA_DIR=`pwd` ./lgj_final```
