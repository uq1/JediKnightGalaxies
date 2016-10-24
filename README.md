# Jedi Knight Galaxies project #
IRC: irc.arloria.net / #JKGalaxies ([webchat](http://www.jkgalaxies.com/irc/))

Forum: https://jkhub.org/forum/117-jedi-knight-galaxies/

Please use discretion when making issue requests on GitHub. The forum is a better place for larger discussions on changes that aren't actually bugs.

## Build Guide ##
https://jkhub.org/topic/5600-compilation-guide/

## Installation of Game ##

First, you need Jedi Academy installed. If you don't already own the game you can buy it from online stores such as [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [Play](http://www.play.com/Games/PC/4-/127805/Star-Wars-Jedi-Knight-Jedi-Academy/Product.html?searchstring=jedi+academy&searchsource=0&searchtype=allproducts&urlrefer=search).

Next, unpack the game binaries into any folder you choose. Desktop/JKG/ works fine.
Copy assets0-3.pk3s from your base game into JKG's base folder.

## Dependencies ##

* Boost library (only for some branches)
* SDL2 (2.0.3+) (included on Windows)
* OpenGL
* OpenAL (included on Windows)
* libpng (included on Windows)
* libjpeg (included on Windows)
* zlib (included on Windows)

## Dedicated Server ##

In order to run a dedicated server, you must use the JKGalaxiesDed binary. Running dedicated from the main executable is currently not possible because it was broken with the addition of modular renderer.

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
