

#define _SYMSTR(str) #str
#define SYMSTR(str) _SYMSTR(str)

#define SYMVER(compat_sym, orig_sym, ver_sym) \
    __asm__(".symver " SYMSTR(compat_sym) "," SYMSTR(orig_sym) "@LIBAIO_" SYMSTR(ver_sym));

#define DEFSYMVER(compat_sym, orig_sym, ver_sym) \
    __asm__(".symver " SYMSTR(compat_sym) "," SYMSTR(orig_sym) "@@LIBAIO_" SYMSTR(ver_sym));

DEFSYMVER(io_getevents_0_4, io_getevents, 0.4)

