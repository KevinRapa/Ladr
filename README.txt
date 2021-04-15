This is 'Ladr', which is a testing framework geared towards
use in signal processing. More generically, it is a framework
which facilitates comparing 2 data vectors processed in the
same way among two processes at arbitrary points along the
processes' execution.

To explain, the idea is that you have a C++ algorithm which
processes data in a pipelined fashion, and a second program
(called the client) which processes the data in the same way,
and is considered to do it correctly. For now, that client
program is MATLAB. Ladr allows you to easily automate
comparing of the data at points along the program execution.

The name ladr is a shortening of 'ladder' for ease of typing.
Describing the testing as two processes exeucting in parallel,
with intermediate connections to compare data, sort of describes
the shape of a ladder also, so yeah.

===============================================================

HOW TO RUN:

1. You must implicitly link to ladr.dll using ladr.lib, so first
   add ladr/lib/ to your library path.

   Add ladr/lib/ to your PATH environment variable so that your
   program can find the DLL.

C++ side:
   2. Include ladr.h in each file you plan to use the API in.

   3. Insert calls to ladr::check() wherever you want to
      test the data. You must supply the data, number of
      data points, and epsilon to user. You may supply an
      optional ID for that call as well.

   4. Call ladr::config() as needed to change Ladr's behavior.

   * To toggle Ladr on and off, define/undefine LADR above the
     inclusion of the header file, or have the compiler define
     it at the start of your build.

MATLAB side:
   5. Call 'addpath' to add ladr/include/ to the path.

   6. Insert an identical number of calls to ladr_check() in
      places analogous to the positions they were added to in
      the C++ program. Data will be compared at these points.

   * To toggle Ladr on and off, insert the lines
         "global LADR; LADR = 1;"
     at the start of the MATLAB script. If LADR == 0, ladr_check()
     will return immediately.

6. Run both the C++ executable and the MATLAB script. You may
   automate this process however you choose.

===============================================================

Ladr will synchronize the C++ and MATLAB processes so that the
data is compared at each call to the check function. Ladr will
output the results to stdout by default.

If mismatches are found, then the data is corrected according
to the MATLAB results so that execution can continue. This way,
the user can see all the bugs instead of the earliest one.

===============================================================

Things I'd like to work on:

- Increase portability. The Boost library offers some solutions to this.
  Currently I use a lot of low-level Windows system calls.
