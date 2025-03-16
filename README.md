# stronghold_auto_market
This mod adds adjustable automatic selling and buying of resources in Stronghold Crusader game.
It is compatibile with:
* [Stronghold Crusader HD](https://fireflyworlds.com/games/strongholdcrusader/)
* [Stronghold Crusader Unofficial Crusader Patch](https://github.com/UnofficialCrusaderPatch/UnofficialCrusaderPatch)
## Build settings
note: this should be 32 bit / 86x to work.
![](images/MyVisualStudioSettings.png)
## How to build
1. Build project in release mode using Visual Studio.
Resulting file is Release//trade_dll.dll
## How to use
1. Inject resulting file `Release//trade_dll.dll` into Stronghold Crusader process using any dll injector.
You need inject dll everytime you run the game.

## notes:
- injector that can be used: https://github.com/adamhlt/DLL-Injector
- when using auto buy/sell the ingame function of the game is used to do so. This comes with some "issues", when choosing 200 as a threshold the game will autosell 50 at once putting the new total at 150 because that is how the market works. This is less of a problem when dealing with lower values as the auto sell then uses lower amounts. Just something to keep in mind. 
![image](https://github.com/user-attachments/assets/5df0943a-9dab-455d-b23a-a6904b9bb0ea)

## How it works
1. Automatically sells resources if their level is above a specified value(default 200).
1. Automatically buy resources until reaching the specified value (default 0, not buying anything). 
1. threshold for each resource can be adjusted in the Resource Settings window.
1. Detects if the market is built and prohibits trading without it.

## Credits

This project uses key functionality for hooking the trade function derived from the [Stronghold Autosell](https://github.com/Vergenter/stronghold_autosell) by [Vergenter]. 
