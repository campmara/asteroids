TODO
=======================================================================================================

Draw some stars in the background? Just for a little visual flair?

Current Point: Working on setting up the line particle system on player death.

Player death needs to be more drawn out. Upon getting hit, the following should happen in order:
1. Player lines disintegrate and start floating away a little bit. This could be the actual lines of the player or just a particle effect with lines.
2. The life counter is immediately decremented.
3. A small delay occurs while the pieces float away, about 2.0f seconds.
4. The player spawns in the middle of the screen again and gameplay resumes.

User Interface Elements to Implement:
* Player 1 should flash at the top of the screen when the game is started briefly.
* GAME OVER when the game...is over.
* Should we go through the trouble of implementing 3-letter name high score input?

Make the lachlan mouse brothers text sin wave up and down for a fun effect?

Think about implementing math functions for our use here: float abs, cos, sin.

Code Cleanup Pass

We do need sound...

Two-player mode????