

Attempt to integrate [libtmt](https://github.com/deadpixi/libtmt)

Terminal working at 80 by 60 characters.

Currently, both tmt and this library allocate screen space, which is wastefull on memory.
It may be better to render directly from the libtmt screen memory.

I needed to pack the control bits to make this fit in memory:
```c
typedef struct TMTATTRS TMTATTRS;
struct TMTATTRS{
    unsigned int bold      : 1;
    unsigned int dim       : 1;
    unsigned int underline : 1;
    unsigned int blink     : 1;
    unsigned int reverse   : 1;
    unsigned int invisible : 1;
    unsigned int           : 2; // padding to align to next byte
    uint8_t fg        ;
    uint8_t bg        ;
} __attribute__((packed));

typedef struct TMTCHAR TMTCHAR;
struct TMTCHAR{
    uint8_t c;
    TMTATTRS a;
} __attribute__((packed));
```

Also, tmt_open seemed to be missing:
```c
    vt->attrs = vt->oldattrs = defattrs;
```
Modfied version below:
```c
TMT *
tmt_open(size_t nline, size_t ncol, TMTCALLBACK cb, void *p,
         const wchar_t *acs)
{
    TMT *vt = calloc(1, sizeof(TMT));
    if (!nline || !ncol || !vt) return free(vt), NULL;

    /* ASCII-safe defaults for box-drawing characters. */
    vt->acschars = acs? acs : L"><^v#+:o##+++++~---_++++|<>*!fo";
    vt->cb = cb;
    vt->p = p;
    vt->attrs = vt->oldattrs = defattrs;

    if (!tmt_resize(vt, nline, ncol)) return tmt_close(vt), NULL;
    return vt;
}
```

I think bright colour and 256 colour support is missing; see https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences