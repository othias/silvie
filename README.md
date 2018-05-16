# Silvie

Silvie is an asset extractor for [Silver](https://en.wikipedia.org/wiki/Silver_(video_game)), an action RPG from 1999 developed by Spiral House.

````
$ silvie
This is Silvie, an asset extractor for Silver.
The following formats are supported:

        chr     3D model, saved as a 3DS file and a GIF file
        eng     Dialog text, saved as an XML file
        raw     Raw image, saved as a GIF file
        spr     Spritesheet, saved as GIF files

For usage information on a given format, type:

        silvie format

A prefix argument denotes the fixed part of the path to the saved files.
````


## Dependencies

* [libgif](http://giflib.sourceforge.net) ≥ 5.0 for saving GIF files
* [lib3ds](https://code.google.com/archive/p/lib3ds) for saving 3DS files
* [libglu](https://cgit.freedesktop.org/mesa/glu) for polygon triangulation

You should be able to install the required dependencies on a debian-like distribution using apt-get:

````
$ apt-get install libgif-dev lib3ds-dev libglu1-mesa-dev
````


## Installation

Once the required dependencies are installed, you can use a C11 compiler to build Silvie:

````
$ gcc *.c -o silvie -std=c11 -lgif -l3ds -lGLU -lGL
````
