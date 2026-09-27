#ifndef GITEN_PLATFORM_COM_H
#define GITEN_PLATFORM_COM_H

#include <Win32.h>

#define ReleaseComObject(object)                                                                   \
    do {                                                                                           \
        if ((object) != NULL) {                                                                    \
            (object)->Release();                                                                   \
            (object) = NULL;                                                                       \
        }                                                                                          \
    } while (0)

#endif // GITEN_PLATFORM_COM_H
