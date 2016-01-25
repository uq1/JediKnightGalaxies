# Jedi Knight Galaxies project #
IRC: irc.arloria.net / #JKGalaxies

FAQ: http://jkhub.org/topic/6613-jkg-faq/

Downloads:
[Binaries](http://jkhub.org/files/file/2544-jedi-knight-galaxies-binaries/) , [Assets](http://jkhub.org/files/file/2543-jedi-knight-galaxies-assets/) , 
[Map Bundle #1](http://jkhub.org/files/file/2652-jedi-knight-galaxies-map-bundle-1/)

Forum: http://www.tinyurl/JKHJKG


Please use discretion when making issue requests on GitHub. The forum is a better place for larger discussions on changes that aren't actually bugs.

## Build Guide ##
http://jkhub.org/topic/5600-compilation-guide/

## Installation of Game ##

First, you need Jedi Academy installed. If you don't already own the game you can buy it from online stores such as [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [Play](http://www.play.com/Games/PC/4-/127805/Star-Wars-Jedi-Knight-Jedi-Academy/Product.html?searchstring=jedi+academy&searchsource=0&searchtype=allproducts&urlrefer=search). 

There are a few ways to actually install JKG itself, the simplest is to just get the [binaries](http://jkhub.org/files/file/2544-jedi-knight-galaxies-binaries/) and [assets](http://jkhub.org/files/file/2543-jedi-knight-galaxies-assets/), and then follow the directions in the readme included.  Please note that, while we try to keep these relatively up to date, the github will always contain the most current version of the assets and binaries and you can always compile and pack the pk3s yourself to get a more update to version.

Alternatively you can install the assets and binaries inside a directory such as C:\JKG\ and then use a batch file/command line arguments to open jkgalaxies.x86.exe while pointing it to the Jedi Academy assets.  This is the recommended setup for developers, as you can then call your exe's location with appropriate args directly from your IDE.  You can do this by setting the fs_basepath arg to be equal to your Jedi Academy install location.  You'll also want to set the fs_cdpath to be equal to JKG's directory.  Optionally you can use fs_homepath to store screenshots and other user specific information.  For example, use this batch script on steam installs for windows (be sure to change 'Guest' to whatever the windows user profile name is):

	jkgalaxies.x86.exe +set fs_game "JKG" +set fs_cdpath "." +set fs_basepath "C:\Program Files (x86)\Steam\SteamApps\common\Jedi Academy\GameData"  +set fs_homepath "C:\Users\Guest\Documents\My Games" + set r_fullscreen 0 +set r_mode -2


## Dependencies ##

* Boost library (only for some branches)
* SDL2 (2.0.3+) (included on Windows)
* OpenGL
* OpenAL (included on Windows)
* libpng (included on Windows)
* libjpeg (included on Windows)
* zlib (included on Windows)

## Dedicated Server ##

In order to run a dedicated server, you must use the JKGalaxiesDed binary. Running dedicated from the main executable is currently not possible because it was intentionally broken with the addition of modular renderer.

## Developer Notes ##

JKG is licensed under GPLv2 as free software. You are free to use, modify and redistribute JKG following the terms in LICENSE.txt.

Please be aware of the implications of the GPLv2 licence. In short, be prepared to share your code under the same GPLv2 licence.  

### If you wish to contribute to JKGalaxies, please do the following ###
* [Fork](https://github.com/JKGDevs/JediKnightGalaxies/fork) the project on Github
* Create a new branch and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (JKGDevs/JediKnightGalaxies)

### If you wish to base your work off JKGalaxies (mod or engine) ###
* [Fork](https://github.com/JKGDevs/JediKnightGalaxies/fork) the project on Github
* Change the GAMEVERSION define in codemp/game/g_local.h from "jkgalaxies" to your project name
* If you make a nice change, please consider back-porting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.




This project is based on [OpenJK](https://github.com/JACoders/OpenJK)
