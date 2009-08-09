#ifndef KDE_SMOKE_H
#define KDE_SMOKE_H

#include <smoke.h>

// Defined in smokedata.cpp, initialized by init_kde_Smoke(), used by all .cpp files
extern SMOKE_EXPORT Smoke* kde_Smoke;
extern SMOKE_EXPORT void init_kde_Smoke();

#ifndef QGLOBALSPACE_CLASS
#define QGLOBALSPACE_CLASS
class QGlobalSpace { };
#endif

#endif
