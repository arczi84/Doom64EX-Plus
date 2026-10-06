#ifndef DOOM_AMIGA_GL_H
#define DOOM_AMIGA_GL_H
#pragma push_macro("LONG")
#undef LONG
#include <proto/minigl.h>
#include <mgl/gl.h>
#pragma pop_macro("LONG")
typedef unsigned int GLhandleARB;
/* Optional desktop-only enums, never advertised as MiniGL capabilities. */
#define GL_TEXTURE 0x1702
#define GL_ADD 0x0104
#define GL_COMBINE_ARB 0x8570
#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_COMBINE_ALPHA 0x8572
#define GL_RGB_SCALE 0x8573
#define GL_ADD_SIGNED 0x8574
#define GL_INTERPOLATE 0x8575
#define GL_CONSTANT 0x8576
#define GL_PRIMARY_COLOR 0x8577
#define GL_PREVIOUS 0x8578
#define GL_SOURCE0_RGB 0x8580
#define GL_SOURCE1_RGB 0x8581
#define GL_SOURCE2_RGB 0x8582
#define GL_SOURCE0_ALPHA 0x8588
#define GL_SOURCE1_ALPHA 0x8589
#define GL_SOURCE2_ALPHA 0x858a
#define GL_OPERAND0_RGB 0x8590
#define GL_OPERAND1_RGB 0x8591
#define GL_OPERAND2_RGB 0x8592
#define GL_OPERAND0_ALPHA 0x8598
#define GL_OPERAND1_ALPHA 0x8599
#define GL_OPERAND2_ALPHA 0x859a
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84ff
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84fe
#define GL_CLAMP_TO_EDGE GL_CLAMP
#define glRecti(x1,y1,x2,y2) glRectf(x1,y1,x2,y2)
static inline void Amiga_TexImage2D(GLenum target, GLint level, GLint internal,
        GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    if (internal == GL_RGBA8 || internal == 4) internal = GL_RGBA;
    if (internal == GL_RGB8 || internal == 3) internal = GL_RGB;
    glTexImage2D(target,level,internal,width,height,border,format,type,pixels);
}
#undef glTexImage2D
#define glTexImage2D Amiga_TexImage2D
/* Keep the tested Classic fallbacks; V29 exposes these as unsupported slots. */
#undef glGetDoublev
#define glGetDoublev Amiga_GetDoublev
#undef glCopyTexSubImage2D
#define glCopyTexSubImage2D Amiga_CopyTexSubImage2D
void glGetDoublev(GLenum pname, GLdouble *params);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static inline void glRectf(float x1,float y1,float x2,float y2) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x1,y1); glVertex2f(x2,y1); glVertex2f(x2,y2); glVertex2f(x1,y2);
    glEnd();
}
#endif
