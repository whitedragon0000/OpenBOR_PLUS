 # OpenBOR PLUS
OpenBOR is a continuation of the Beats Of Rage 2D game engine, which was originally
created by the wonderful folks over at [Senile Team](http://www.senileteam.com).
The plus version is based on official version and it has all official features plus
new features!

## History
In 2004, [Senile Team](http://www.senileteam.com) released Beats of Rage, a free beat-'em-up for DOS inspired
by SEGA's Streets of Rage series and using sprites from SNK Playmore's King of
Fighters series.  The game spread only by word of mouth, but it nonetheless
amassed popularity very quickly.  Senile Team soon released an edit pack allowing
anyone interested to create a module for the BOR engine.

In 2005, Kirby2000 asked Senile Team to open the source code to BOR.  They
agreed, and OpenBOR was born.  Development on the engine was continued by the
community, and still is to this day.

## Platforms
OpenBOR has a very modular and portable design inherited from Beats of Rage - several ports have been made available.

### Current
These platforms are actively supported and may be compiled with the latest OpenBOR engine.

* Android
* Windows
* Linux
* Mac OS X
* Wii
* PSP

### Discontinued

The following platforms are still available as legacy binaries, but are no longer supported and may not be compatible with current iterations of OpenBOR.

* Dreamcast
* GP2X
* GP2X Wiz
* OpenDingux (Dingoo A320 & GCW-Zero)

## OpenBOR Team

### Current Members

#### [White Dragon](https://github.com/whitedragon0000) (2016-)
A long time module author and extremely knowledgeable coder who joined the development team in 2016 and immediately began making an impact. White Dragon generally focuses on level and menu properties, but has branched out into various other facets of the engine over time.

### Former members and contributors

#### [Damon Caskey](https://github.com/DCurrent) (2007-)
OpenBOR project manager and site owner of the OpenBOR community. Primary contributions are core engine and scripting development, code cleanup, and organization. Main focus is keeping OpenBOR future proof and modular by replacing specialized hardcoding and overlap with generalized features that allow for more author creativity.

#### [Plombo](https://github.com/plombo) (2009-)
A developer who prefers to work on OpenBOR's supporting libraries and platform-specific backends.  Known for maintaining the Wii port, writing the GPU-accelerated video code for Wii and OpenGL, and a few engine features.

#### [Douglas Baldan](https://github.com/dbaldan) (2018-)
Known as O'Ilusionista, Douglas is a highly respected administrator of the OpenBOR community and also a prolific member of the Mugen scene. Douglas is new to coding but brings a plethoera of graphic and game design experience to the team. We look for exciting things from Mr. Baldan soon!

#### [Malik](https://github.com/msmalik681) (2018-)
Malik comes to the team with a good scripting background. He is still learning his way around application development, but shows a lot of promise and a great willingness to learn. As his skills progress, he will no doubt be a an invaluable asset to the team!

#### uTunnels (2007-2014)
Among many other powerful additions, contributed the original scripting engine to OpenBOR, single handedly
breaking nearly every limitation module authors faced. While not officially retired, uTunnels' presence became gradually more infrequent before stopping altogether in early 2014.

#### Anallyst (2011)
This developer's work centered mainly around trimming the fat and optimizing the codebase.

#### SumolX (2006-2011)
Former project manager and lead programmer, retired from the scene in 2011. Known for porting PSP,
PS3, Linux, Wii, GP2X and maintaining all other platforms and code base.

#### KBbandressen (2007-2011)
Contributed a plethora of features, including the powerful text object and filestream capabilities.

#### CGRemakes (2005-2006)
Main developer after Kirby2K.  Introduced many exicting features to engine.

#### LordBall (2006)
Developed offshoot engine based on OpenBOR.  Shared features with both engines.

#### Tails (2006)
Developed offshoot engine based on OpenBOR.  Shared features with both engines.

#### Fugue (2006)
Developed offshoot engine based on OpenBOR.  Shared features with both engines.

#### Kirby2K (2004-2005)
The original developer of OpenBOR who asked Senile Team for permission to open
up Beats Of Rage.

### [Senile Team](http://www.senileteam.com)
Senile team was not directly involved with developing OpenBOR, but their opening
of the orginal Beats of Rage codebase was vital. Parts of the orginal BOR still
reside in OpenBOR to this day.

#### [Roel](http://www.roelvanmastbergen.nl) (credited as "Opla" in BoR)
The team's chieftain. Does most of the game design, programming and artwork.

#### Jeroen (credited as "Leila" in BoR)
Does all the things no one else does.

#### Sander (credited as "Albatross" in BoR)
3D artist and animation sequence editor.

#### Ben
Senile Team's composer.

#### [Neill](http://www.neillcorlett.com)
Neill was the first to port Beats of Rage to other systems, namely Playstation 2
and Dreamcast. He now supports Senile Team with advice regarding console hardware
and code compatibility.

## Websites
### [ChronoCrash](http://www.ChronoCrash.com)

Home of the OpenBOR community and OpenBOR team. This is the place to go if you want to discuss discuss OpenBOR development, find ready to play game modules, or get started building one of your own. 

### [Senile Team](http://www.senileteam.com)

Senile Team is not responsible for OpenBOR, and has also dropped all support for the original Beats of Rage. Instead you should stop in to see their latest projects - you’ll no doubt find something interesting!

## New Features
* added "movex", "movez" to entityproperty: the potential entity directions
* added "collidedentity" to script: it returns the collided entity handler
* opened animation platform properties to script

#### ***ENTITY COLLISION***
accessible in debug mode too.<br/>

constants:<br/>
ANI_PROP_ENTITY_COLLISION<br/>
ENTITY_COLLISION_PROP_COORDINATES<br/>
ENTITY_COLLISION_PROP_TAG<br/>
ENTITY_COLLISION_PROP_INDEX<br/>

animation commands:<br/>
ebox {x} {y} {width} {height} {z1} {z2}<br/>
ebox.x {value}<br/>
ebox.y {value}<br/>
ebox.width {value}<br/>
ebox.height {value}<br/>
ebox.z1 {value}<br/>
ebox.z2 {value}<br/>
eboxz {z1} {z2}<br/>

model commands:<br/>
entitypushing {int}: if 1 entity pushing target on collision<br/>
pushingfactor {float}: pushing factor on collision. Default: 1.0<br/>

openborscript functions:
get_entity_collision_collection(void handle, int frame);<br/>
get_entity_collision_instance(void handle, int index);<br/>
get_entity_collision_property(void handle, int property);<br/>
set_entity_collision_property(void handle, int property, value);<br/>

openborscript:<br/>
added "entitypushing", "pushingfactor" to entityproperty<br/>
added "collidedentity" to entityproperty<br/>
added "maxcollisions" to openborvariants<br/>

events:<br/>
onentitycollisionscript<br/>
localvars:<br/>
"self": entity<br/>
"target": entity<br/>
"self_ebox_handler": the handler for ebox of self to use with openborscript functions<br/>
"target_ebox_handler": the handler for ebox of target to use with openborscript functions<br/>


#### ***MULTPLE COLLISION BOXES***
opened multiple collision boxes to openbor:<br/>
you establish the max collision boxes into model.txt file with<br/>
maxcollisions {int} (default: 2)<br/>
then you can change into animation the index of boxes:<br/>
setaboxindex {int} for attack collision boxes (default: 0)<br/>
setbboxindex {int} for body collision boxes (default: 0)<br/>
seteboxindex {int} for entity collision boxes (default: 0)<br/>

by default the index relative to a box is set to 0 of course.<br/>

example:<br/>
```
ANIM IDLE
bbox 1 2 3 4 5
setbboxindex 1
bbox 5 6 7 8 9
```

in this example you set 2 bboxes.

see this example too:

```
ANIM IDLE
bbox 1 2 3 4 5
setbboxindex 1
bbox 5 6 7 8 9
bbox 1 2 3 4 5
```

in this example you set 2 bboxes both: bbox 1 2 3 4 5<br/>
because bbox 1 2 3 4 5 at index 1 overrides bbox 5 6 7 8 9 at index 1<br/>

#### ***BOOMERANG***

model keys:<br/>
subtype boomerang<br/>
aimove boomerang<br/>

model commands:<br/>
boomerang {name}<br/>
boomerangvalues {acceleration} {horizontal_distance}<br/>

animation commands:<br/>
custboomerang {name}<br/>

animations:<br/>
getboomerang<br/>
getboomeranginair<br/>
_use range {min} {max} to get the boomerang by range_

openbor script:<br/>
changed access to boomerang props in openbor script:<br/>
get/change entityproperty "boomerang" "acceleration" {val}<br/>
get/change entityproperty "boomerang" "hdistance" {val}<br/>

using subentity and spawnframe example:<br/>
for player set type npc, for enemies set type enemy<br/>
```
subentity boomerang
spawnframe 0 5 0 30
```

using custboomerang example:
```
custboomerang boomerang
throwframe 0 30
```
