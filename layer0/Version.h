#ifndef _PyMOL_VERSION
#define _PyMOL_VERSION "2.1.0+EPAM.OpenVR.2018.10.5"
#endif

/* for session file compatibility */

#ifndef _PyMOL_VERSION_int
// X.Y.Z   -> XYYYZZZ
#define _PyMOL_VERSION_int 2001000
#define _PyMOL_VERSION_double (_PyMOL_VERSION_int / 1000000.)
#endif
