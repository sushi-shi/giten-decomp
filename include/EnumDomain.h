#ifndef GITEN_ENUMDOMAIN_H
#define GITEN_ENUMDOMAIN_H

// Keep a named enum domain while retaining the retail storage width.
// Strict type checking uses the enum at its storage sites as well.
#define GZ_ENUM_BEGIN_SPLIT(name, storage) typedef enum name {
#define GZ_ENUM_END_SPLIT(name)                                                                    \
    }                                                                                              \
    name;

// A bit set stored in `storage`; members combine with `|` and test with `&`.
#define GZ_ENUM_FLAGS_BEGIN(name, storage) typedef enum name {
#define GZ_ENUM_FLAGS_END(name)                                                                    \
    }                                                                                              \
    name;

// Named numeric constants that do not form a value domain of their own, such
// as encoding biases, masks and table extents.
#define GZ_ENUM_CONST_BEGIN(name) enum {
#define GZ_ENUM_CONST_END(name)                                                                    \
    }                                                                                              \
    ;

// STORAGE marks a field or global of the domain's declared width. PARAM,
// RETURN and LOCAL mark a parameter, return value or temporary whose retail
// width is evidenced independently of that field. The retail build sees
// `storage`, so records, codegen and C++ decorated names are unchanged.
#ifdef GZ_STRICT_ENUMS
#define GZ_ENUM_STORAGE(name, storage) name
#define GZ_ENUM_PARAM(name, storage) name
#define GZ_ENUM_RETURN(name, storage) name
#define GZ_ENUM_LOCAL(name, storage) name
#else
#define GZ_ENUM_STORAGE(name, storage) storage
#define GZ_ENUM_PARAM(name, storage) storage
#define GZ_ENUM_RETURN(name, storage) storage
#define GZ_ENUM_LOCAL(name, storage) storage
#endif

#endif // GITEN_ENUMDOMAIN_H
