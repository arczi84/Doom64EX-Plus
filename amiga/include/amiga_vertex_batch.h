#ifndef AMIGA_VERTEX_BATCH_H
#define AMIGA_VERTEX_BATCH_H

#define AMIGA_MGL_VERTEX_CAPACITY 4096
#define AMIGA_BATCH_VERTICES 192

/* 64 triangles per call: even 14 generated clipping vertices per triangle
   fit in the 2048-entry upper half reserved by MiniGL. vtx_t/word are supplied
   by the engine (or the standalone regression test). */
static inline int Amiga_CopyTriangleBatch(vtx_t *dest, word *indices,
        const vtx_t *source, int vertex_count, const word *input, int remaining) {
    int i, n = remaining < AMIGA_BATCH_VERTICES ? remaining : AMIGA_BATCH_VERTICES;
    if (n <= 0 || n % 3 || vertex_count <= 0) return -1;
    for (i = 0; i < n; ++i) {
        if (input[i] >= vertex_count) return -1;
        dest[i] = source[input[i]];
        indices[i] = (word)i;
    }
    return n;
}
#endif
