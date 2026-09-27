#ifndef GITEN_ENUMDOMAIN_H
#define GITEN_ENUMDOMAIN_H

// Keep a named enum domain while retaining the retail storage width.
// Strict type checking uses the enum at its storage sites as well.
#define GZ_ENUM_BEGIN_SPLIT(name, storage) typedef enum name {
#define GZ_ENUM_END_SPLIT(name)                                                                    \
    }                                                                                              \
    name;

#ifdef GZ_STRICT_ENUMS
#define GZ_ENUM_STORAGE(name, storage) name
#else
#define GZ_ENUM_STORAGE(name, storage) storage
#endif

#endif // GITEN_ENUMDOMAIN_H
