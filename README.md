# Jedi Knight Galaxies #
[Webpage](https://www.jkgalaxies.net)  
[Discord](https://discord.gg/YuG8Zks)   

[FAQ](http://jkhub.org/topic/6613-jkg-faq/)  
[Forum]( https://jkhub.org/forum/117-jedi-knight-galaxies/) (hosted by JKHub)  
[Official Announcements](https://jkhub.org/forum/119-jkgalaxies-news/)   
[ModDB Page](http://www.moddb.com/mods/jkgalaxies) (not updated frequently)  
[Lbry.tv](https://lbry.tv/@JediKnightGalaxies:c?view=about)  
[YouTube](https://www.youtube.com/channel/UCnnAUSngUk8l3fycYMVXkKQ/featured)  
[Twitch](https://www.twitch.tv/jkgalaxies)  




#### Downloads ####
The most update links are available on the download page on the website, under the [test releases section](https://www.jkgalaxies.net/download.html#testdiv), but you can also use the following links which are occasionally updated:

[Binaries](https://github.com/JKGDevs/JediKnightGalaxies/releases/) , [Assets](https://uvu.box.com/shared/static/lztt5mayu4daj9szvg8v0rtds6vwmnmq.zip) , 
[Map Bundle #1](http://jkhub.org/files/file/2652-jedi-knight-galaxies-map-bundle-1/) , [Map Bundle #2](https://uvu.box.com/shared/static/kb0wvyqtz0sarzs0kn1c8h03wrpjgb7c.zip)

Please use discretion when making issue requests on GitHub. The forum is a better place for larger discussions on changes that aren't actually bugs.


### What is Jedi Knight Galaxies? ###
Jedi Knight Galaxies is a competitive and innovative multiplayer shooter, played out in the Star Wars universe. Currently a full conversion mod, the project was originally based on and modified from Raven Software’s [Jedi Knight: Jedi Academy](https://en.wikipedia.org/wiki/Star_Wars_Jedi_Knight:_Jedi_Academy).  Jedi Knight Galaxies is open source and available completely free of charge; however, you will need the original game in order to play.  Jedi Academy is frequently available on [Steam](http://store.steampowered.com/app/6020/STAR_WARS_Jedi_Knight__Jedi_Academy/) and [GoG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy) during sales for less than $4. We aim to develop the project into a Large Multiplayer Online game, crossing traditional RPG (role playing game) elements with the fast paced excitement of an FPS (first person shooter).

The project will be released in phases, each building on to the previous one with more aspects of the final goal. The current Phase, JKG: Versus, was first released September 17th, 2012 as a public beta, showcasing the primary FPS elements of the game.  Subsequent updates and versions have been produced since then.  As development progresses, sandbox environments will be added, initial RPG elements will appear, and player vs. enemy combat will be implemented into the faction oriented gameplay. Improved dialog and cinematic features along with skills, powers, and more aspects that belong in a Role-Play Game will be developed, set between the events of Episode IV: A New Hope and Episode III: Revenge of the Sith. Finally, the full-fledged JKG: LMO takes the experience to a much grander level.

The game is a much loved hobby project supported by its community and developers who work on it in their freetime, contribution is encouraged and newbies are welcome.  Originally founded by a group of Jedi Academy modders led by Jake (now retired), the project has since been in and out of development since 2008. The project is currently headed by our hardworking, lead programmer: eezstreet.  The current development team comes from a variety of backgrounds, countries, and age, united by our desire to make a great game. 

Welcome to Jedi Knight Galaxies, where we break the limits!


## Build Guide ##
[Old Guide](https://jkhub.org/topic/5600-compilation-guide/)  
[OpenJK Compilation Guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide)  
[Flate's Ubuntu/Debian Guide](https://github.com/JKGDevs/JediKnightGalaxies/wiki/Flate%27s-Debian-Ubuntu-Compiling-Guide)

## Installation of Game ##
JKG is available on Windows, but it will compile with linux and OSX and has been briefly tested to run on Ubuntu.

To install, you will first need Jedi Academy installed. If you don't already own the game you can buy it from online stores such as [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN), or [GOG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy).

There are a few ways to actually install JKG itself, the simplest is to just get the [binaries](https://github.com/JKGDevs/JediKnightGalaxies/releases) and [assets](https://uvu.box.com/shared/static/lztt5mayu4daj9szvg8v0rtds6vwmnmq.zip), and then follow the directions in the readme included.  Please note that while we try to keep these relatively up to date, the github will always contain the most current version of the source code and you can always compile and pack the pk3s yourself to get a more up to date version.  Most of the JKG assets are not stored on this github repository, only source code and code-like assets.

Alternatively you can install the assets and binaries inside a directory such as C:\JKG\ and then use a batch file/command line arguments to open jkgalaxies.x86.exe while pointing it to the Jedi Academy assets.  This is the recommended setup for developers, as you can then call your exe's location with appropriate args directly from your IDE.  You can do this by setting the fs_basepath arg to be equal to your Jedi Academy install location.  You'll also want to set the fs_cdpath to be equal to JKG's directory.  Optionally you can use fs_homepath to store screenshots and other user specific information, if not specified fs_homepath will be set to "...Documents\My Games\JKGalaxies"  For example, use this batch script on steam installs for windows:

	jkgalaxies.x86.exe +set fs_game "JKG" +set fs_cdpath "." +set fs_basepath "C:\Program Files (x86)\Steam\SteamApps\common\Jedi Academy\GameData" +set r_mode -2

If you're stuck, you can also watch [this video](https://www.youtube.com/watch?v=odx-3f07_eA) to see how to install the game.

## Dependencies ##

* SDL2 (2.0.3+) (included on Windows)
* OpenGL
* OpenAL (included on Windows)
* libpng (included on Windows)
* libjpeg (included on Windows)
* zlib (included on Windows)

## Dedicated Server ##

In order to run a dedicated server, you must use the JKGalaxiesDed binary. Running dedicated from the main executable is currently not possible because it was intentionally broken with the addition of modular renderer.

## Developer Notes ##

JKG is licensed under GPLv2 as free software. You are free to use, modify and redistribute JKG following the terms in [LICENSE.txt](https://github.com/JKGDevs/JediKnightGalaxies/blob/master/LICENSE.txt).

Please be aware of the implications of the GPLv2 licence. In short, be prepared to share your code under the same GPLv2 licence.  

### If you wish to contribute to JKGalaxies, please do the following ###
* [Fork](https://github.com/JKGDevs/JediKnightGalaxies/fork) the project on Github
* Create a new branch and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (JKGDevs/JediKnightGalaxies)

### If you wish to base your work off JKGalaxies (mod or engine) ###
* [Fork](https://github.com/JKGDevs/JediKnightGalaxies/fork) the project on Github
* Change the GAMEVERSION define in codemp/game/g_local.h from "jkgalaxies" to your project name
* If you make a nice change, please consider back-porting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.


## Current Project Goals ##
These are very much subject to change, especially phases later than Versus.

### Phase 1:  Versus (current iteration). ###
* The goal here is to have a functioning arena based shooter (similar to games such as Call of Duty or Unreal Tournament) with the beginnings of some later RPG elements such as armor, pazaak card game, etc.  There are three major milestones currently being worked on: [Milestone 3](https://github.com/JKGDevs/JediKnightGalaxies/issues?q=issue+milestone%3A%22Versus+Revision+3%22) and [Milestone 4](https://github.com/JKGDevs/JediKnightGalaxies/issues?q=is%3Aopen+is%3Aissue+milestone%3A%22Versus+Revision+4%22).  Milestone 3 includes new features like better melee, armor, shields, jetpacks, debuff system etc. in addition to some bug fixes not addressed in Milestone 2. Milestone 4 primarily focuses on the all new saber system and related features such as duel mode.  Later on other milestones will be worked on.  In Milestone 5, the Force will be reworked and greatly expanded on.  Later milestones will introduce the skill trees and a rudimentary system for spending xp in preparation for the eventual leveling system in Phase 2 and 3.

#### Phase 2: Coop ####
* The main features that are added here include the NPC system and fleshing out of the dialogue system. They also include overhauls to the chat systems, fonts, UI, and more.  Prep work for the leveling system and quest system to be introduced here.  We may also attempt some MoBA like features (still needs discussion).

#### Phase 3: RPG ####
* Open World RPG. The main features added here include quests, (more) minigames, and AI for existing minigames, like Pazaak. Also includes levelling up, experience, single person quests, large planet-themed maps, character creator, fully fleshed out skill system etc. This mode will mainly focus on implementing things from a single player's perspective.  Data will be persistent and exist as long as the server is running.

#### Phase 4: LMO ####
* Features to the main game include group quests, Looking For Group system, and a lot of the dungeons that were designed for Phase 2 brought back. Includes a master server based architecture and functioning account system.  At this point, the game is pretty much done.
* Later possible expansions: space battles, more worlds, additional minigames


## Version Information ##
JKG uses the following version schema: Phase.Major.Minor with an optional "patch" letter following the minor version for hotfixes (these are unplanned versions).  Phases represent collosal changes to the game (these are often called expansions in other games) that include major new features and gameplay changes and even engine changes!  Phases can considered seperate games.  Major versions represent completions of milestones that include several new features and bug fixes.  Minor versions represent small incremental changes within a milestone and usually represent a single new feature or bug fixes.  Other software produced by the developers (such as the launcher) uses its own versioning scheme and is not covered in this readme.



## Contributors ##
### Active Developers ###
* Darth Futuza
* Silverfang


### Retired Developers ###
#### Coding ####
* BobaFett
* DeathSpike
* Didz
* eezstreet
* Raz0r
* Stoiss
* UniqueOne
* Vek892
* Xycaleth

#### Mapping ####
* dvg94
* MaceCrusherMadunusus
* Pande
* Sato
* SJC
* Yzmo

#### Modeling ####
* Blastech
* CaptainCrazy
* DarthPhae
* DT
* Dusk
* IG64
* Tri
* Pande
* Psycho
* Resuru

#### Animations ####
* Hirman

#### Artists ####
* BlasTech
* HellKobra
* Pande
* Resuru
* Suibuku

#### Sound and EFX ####
* Blastech
* Sareth

#### Misc ####
* BlueCasket
* Caelum
* DarthLex
* HellKobra
* Fighter
* Jake (Project Founder)
* JohnGWolf
* Konradwerks
* Mart
* Sharpie
* TheDarkness

#### Special Thanks ####
Thank you to the following for your support of JKG and your efforts in contributing to it with your time in various different ways even though it may not have been through directly developing it.  We appreciate the encouragement, advertising, resources, and feedback you've provided:  




* Arkan (Mapping)
* Dalo Lorn
* Hapslash (Stormtrooper Model)
* Inyri Forge (Instruments & KotOR Objects)
* Krattle (Chalmun’s Cantina)
* MountainDew
* Nightcrawler
* Noodle
* Obliviion
* Ori'Ramikad
* Orj (Mosquito Vibroblade)
* Plasma (Mapping)
* Smoo
* Szico VII (Nightfall Map)
* Tommy
* WizardMKBK (help with icons)
* Xel
* And many more!
 
 

This project is based on [OpenJK](https://github.com/JACoders/OpenJK) , which in turn is based on the source release of Raven's Jedi Academy.  Thank you all who have contributed to either project!   JKG IS NOT MADE, DISTRIBUTED, OR SUPPORTED BY LUCASARTS, A DIVISION OF LUCASFILM ENTERTAINMENT COMPANY LTD. ELEMENTS™ & (©) LUCASARTS, A DIVISION OF LUCASFILM ENTERTAINMENT COMPANY LTD.
