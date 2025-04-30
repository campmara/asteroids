# Mara's Asteroids

This project is intended to be an exercise in recreating asteroids in the most barebones manner possible. It will only run on Win32 and contain the absolute minimum featureset to qualify for an asteroids game.

This is the first in a series of learning exercises I am doing to get myself comfortable and familiar with the practice of both writing games in C/C++ and mostly from base platform code. It follows the general programming ethos outlined in the first thirty days of Casey Muratori's Handmade Hero, but is intended to be structured in its own way and aims to bear no affiliation to that project. I am trying to do my own homework here! That being said, there are a lot of smart decisions made in his series involving the delineation between "game" and "platform" code, and I chose to employ that structure here.

## Notes and Takeaways

Since we're now at a point we could feasibly call "done", I'd like to jot down some quick unordered notes on my feelings after completing this project.

* It's possible that re-making Asteroids in this way helped me land my most recent job! Kind of crazy that happened at all but I am immensely excited to get started there soon. I was able to bring this project up quite a significant amount throughout the interview process, and I think it served as a great example of somebody taking the work of building things from scratch seriously.
* When you think you need a dynamic array, think again. It's possible that what you are trying to do can be done with a flat array of fixed size, which will keep the element memory contiguous in a much simpler way. I did think at multiple times that I should implement some sort of dynamic list structure, but in the end was able to make this work just fine without it!
* Always be aware of what your project is, and what the specific requirements of the project are. *Don't create generic solutions to specific problems when you don't have to* and things get done in a fraction of the time.
  * For example, because I knew that there were only a few sounds that would be played at all, none of them being a length longer than two seconds, there was no need to bother with sound streaming.
  * Another example using sound: I could realistically assume, given the constraints of the game's design, that a very small amount of sounds would need to be playing concurrently at all. Because of this, I was able to allocate fixed size arrays of source voices and just keep reusing those same resources without ever needing to allocate more later.
* Building things from scratch is not that hard! (Once you have a good example to learn from!). Casey Muratori's series, though he may feel complicated about it now, is still the best introduction I could have hoped for in learning the fundamentals necessary to make this game. Parsing a WAV file is not that complicated, really, and prior to watching that series I would have been too afraid to even attempt such a thing.
* All told, this project took maybe a month or so to complete, and that was with a few little breaks here and there. This game just didn't take very long to make.
* I'm really happy with how this turned out, and feel like I learned so much in such a short time from it. Subsequent learning projects might not take nearly as long to complete since I will have this bedrock underfoot.
* From here, I hope to use this project as a basis for learning about integrating very specific features: platform abstraction with SDL3, and hardware acceleration with the various graphics APIs.

## License: MIT

Copyright 2025 Mara Diana Campbell

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.