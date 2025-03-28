# Mara's Asteroids

This project is intended to be an exercise in recreating asteroids in the most barebones manner possible. It will only run on Win32 and contain the absolute minimum featureset to qualify for an asteroids game.

This is the first in a series of learning exercises I am doing to get myself comfortable and familiar with the practice of both writing games in C/C++ and mostly from base platform code. It follows the general programming ethos outlined in the first thirty days of Casey Muratori's Handmade Hero, but is intended to be structured in its own way and aims to bear no affiliation to that project. I am trying to do my own homework here!

## Notes and Takeaways

One of the things I realized early on was that I would need to have some sort of API for generating simple random numbers in a given range (for stuff like asteroid positions, directions, speeds, etc.). In order to do this I set down a small exploratory path in trying out the implementation of a few random number generation algorithms, namely the following:
* Linear Congruential Generator
* Xorshift Algorithm
* PCG Random Generation

## License: MIT

Copyright 2025 Mara Diana Campbell

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.