TODO
=======================================================================================================

High Score Name Input:
* game ends, a message appears if the score is higher than any of the top 10 scores.
* player enters a three-letter name to distinguish themselves, the name gets added.
* the list is updated and this new list of names is written to a high score file on disk.
* ONLY write to the original file if the whole write operation succeeded, then move the contents to the original file.

Code Cleanup Pass

We do need sound...

Two-player mode????

Draw some stars in the background? Just for a little visual flair?

BUGS:
* Discrete collision checking seems to not work 100% of the time, noticed this around the screen boundary, so maybe is has something to do with our collision grid and their north, south, east, west reference structure...

Should we do an optimization pass over the math functions?