TODO
=======================================================================================================

Think about implementing math functions for our use here: float abs, cos, sin.

Asteroids need some work, they need to be semi random and allow for breaking into smaller sizes. Time for a function!

We need a way of displaying score values. The original game did this by drawing its font with the lines,
which we could do but would need a different function for every possible letter or number we need for the
game.
   Alternatively, we could import stb_truetype.h and learn about how to render .ttf fonts and just do
   that!