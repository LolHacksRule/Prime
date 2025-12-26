# Prime
 Abandoned and unfinished decompilation of the multiplatform PopCap/SexyApp framework used since Bejeweled 3 and onward

Heavily based off reversing the debug engine build of the advertising plugin of Japanese Bejeweled 3 for Windows and Windows Phone 7 (XNA) framework ports.

This revision is based off WideString builds of the framework so I'm not sure if disabling them would still be supported.

Dependencies:
- CMake 3.18+: https://cmake.org/
- DirectX August 2007+2009 SDK (Untested, you may be able to use the June 2010 one but I haven't tested, definitely not the latest one)

The main focus is to provide a better development environment for users of the engine in current time, freeing users from limitations of the public engine like being only designed for Windows, not supporting DirectX 9, and being closer to PopCap's internal standards.

Any modifications are licensed under MIT.

Public tools, demos and DRM code aren't included.