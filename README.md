# 2D Terrain Generation with procedural generation
The aim of this program is to generate a 2D terrain using a simple algorithm for procedural generation.
It uses the Lehmer algorithm to generate a sequence of numbers based on a seed.
The terrain is then generated with the sequence of numbers, which means that the same world is always generated for a given seed.
So far the program creates land and populates it with tress.
The visualisation was handled by a simple-to-use library called [Pixel Game Engine](https://github.com/OneLoneCoder/olcPixelGameEngine).

The first time the terrain is generated a preselected seed is selected, after that a random seed is picked.
For a given seed the same world is generated every time.

### Libraries used
* [Pixel Game Engine](https://github.com/OneLoneCoder/olcPixelGameEngine)