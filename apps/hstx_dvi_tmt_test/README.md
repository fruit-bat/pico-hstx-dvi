

Attempt to integrate libtmt

tmt allocates screen space and so does the grid renderer.
Possible it may work better the renderer operates directly from the libtmt screen.

Now working with an 80x60 terminal

I needed to pack the control bits to make this fit in memory:
```c
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
};
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