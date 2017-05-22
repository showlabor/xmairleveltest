# xmairleveltest #

This is a test for the implementation of fader levels Xrm32Level as defined in xrm32level.hpp.
A Xrm32Level<1024> object aims to behave like the fader on an X Air, M Air or X32 mixer.
For testing its accuracy w.r.t. rounding and dB conversion we send fader level values to the mixer,
query the level afterwards and compare the result to the respective values of the Xrm32Level object.

Feel free to adjust the number of steps used for testing.

## Channel 12 ##

The program controls fader 12. So you don't want channel 12 to be sending output two a speaker.


## liblo version ##

In order to build and run this successfully you'll need a recent version of liblo.
The version in Fedora 25 is not up to date and it won' be updated until there is
'release' of liblo. I don't know if other distros handle this.

Nevertheless, if this program doesn't work please try with a liblo from git:

	      https://github.com/radarsat1/liblo

Please, adjust the location of liblo in the Makefile and in test_xrm32level.sh.
(Forgive my laziness but this program is really to small to turn it into a fullfledged
automake or CMake thing.)

## Build ##

After making the above adjustments run 'make' and you're done.

## Run ##

Run ./test_xrm32level.sh in the source directory.
