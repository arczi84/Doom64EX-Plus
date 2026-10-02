#include <assert.h>
#include <stdio.h>
typedef unsigned short word;
typedef struct { float x, y, z, u, v; unsigned char rgba[4]; } vtx_t;
#include "include/amiga_vertex_batch.h"

int main(void) {
    static vtx_t source[65536], dest[AMIGA_BATCH_VERTICES];
    word input[603], indices[AMIGA_BATCH_VERTICES];
    int i, offset = 0, calls = 0;
    for (i=0; i<65536; ++i) source[i].x = (float)i;
    for (i=0; i<603; ++i) input[i] = (word)(65535-i*31);
    while (offset < 603) {
        int n = Amiga_CopyTriangleBatch(dest,indices,source,65536,input+offset,603-offset);
        assert(n>0 && n<=192 && n%3==0);
        for (i=0; i<n; ++i) {
            assert(indices[i]==i);
            assert(dest[i].x==source[input[offset+i]].x);
        }
        offset += n; ++calls;
    }
    assert(calls==4);
    assert(Amiga_CopyTriangleBatch(dest,indices,source,256,input,3)==-1);
    assert(Amiga_CopyTriangleBatch(dest,indices,source,65536,input,2)==-1);
    assert((AMIGA_BATCH_VERTICES/3)*14 < AMIGA_MGL_VERTEX_CAPACITY/2);
    puts("PASS: high indices rebased, batches preserve triangles, invalid input rejected");
    return 0;
}
