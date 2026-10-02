/* Missing desktop convenience operations implemented using MiniGL's ABI. */
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include <stdlib.h>
void glGetDoublev(GLenum pname, GLdouble *params) {
    GLfloat matrix[16];
    int i;
    /* The engine only requests projection/modelview matrices here. */
    if (pname != GL_PROJECTION_MATRIX && pname != GL_MODELVIEW_MATRIX)
        I_Error("Unsupported MiniGL double query");
    glGetFloatv(pname, matrix);
    for (i=0; i<16; ++i) params[i] = matrix[i];
}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xo, GLint yo,
                       GLint x, GLint y, GLsizei width, GLsizei height) {
    unsigned char *pixels;
    /* The destination can be power-of-two padded beyond the framebuffer. */
    if (x < 0 || y < 0 || x >= video_width || y >= video_height) return;
    if (width > video_width-x) width=video_width-x;
    if (height > video_height-y) height=video_height-y;
    if (width <= 0 || height <= 0) return;
    pixels=malloc((size_t)width*height*4);
    if (!pixels) I_Error("MiniGL framebuffer copy: out of memory");
    glFinish();
    glReadPixels(x,y,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexSubImage2D(target,level,xo,yo,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    free(pixels);
}
