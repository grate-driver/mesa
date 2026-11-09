#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static GLuint shader(GLenum t, const char *s) {
  GLuint x = glCreateShader(t);
  glShaderSource(x, 1, &s, 0);
  glCompileShader(x);
  GLint ok;
  glGetShaderiv(x, GL_COMPILE_STATUS, &ok);
  if (!ok)
    exit(5);
  return x;
}
static GLuint program(int tex) {
  const char *vs =
      "#version 120\nattribute vec2 position;attribute vec2 coord;varying vec2 "
      "uv;void main(){gl_Position=vec4(position,0,1);uv=coord;}";
  const char *fs =
      tex ? "#version 120\nvarying vec2 uv;uniform sampler2D image;void "
            "main(){gl_FragColor=texture2D(image,uv);}"
          : "#version 120\nuniform vec4 color;void main(){gl_FragColor=color;}";
  GLuint p = glCreateProgram();
  glAttachShader(p, shader(GL_VERTEX_SHADER, vs));
  glAttachShader(p, shader(GL_FRAGMENT_SHADER, fs));
  glBindAttribLocation(p, 0, "position");
  glBindAttribLocation(p, 1, "coord");
  glLinkProgram(p);
  GLint ok;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok)
    exit(6);
  return p;
}
static void check(const char *s) {
  GLenum e = glGetError();
  if (e) {
    fprintf(stderr, "GL error %x at %s\n", e, s);
    exit(7);
  }
}
int main(int argc, char **argv) {
  Display *d = XOpenDisplay(0);
  if (!d)
    return 2;
  int attrs[] = {GLX_RGBA,
                 GLX_DOUBLEBUFFER,
                 GLX_RED_SIZE,
                 8,
                 GLX_GREEN_SIZE,
                 8,
                 GLX_BLUE_SIZE,
                 8,
                 GLX_ALPHA_SIZE,
                 8,
                 None};
  XVisualInfo *v = glXChooseVisual(d, DefaultScreen(d), attrs);
  if (!v) {
    fputs("No RGBA8 visual\n", stderr);
    return 3;
  }
  XSetWindowAttributes a = {
      .colormap =
          XCreateColormap(d, RootWindow(d, v->screen), v->visual, AllocNone)};
  Window w = XCreateWindow(d, RootWindow(d, v->screen), 0, 0, 128, 128, 0,
                           v->depth, InputOutput, v->visual, CWColormap, &a);
  XStoreName(d, w, "GRATE source-over validation");
  XMapWindow(d, w);
  XSync(d, False);
  GLXContext ctx = glXCreateContext(d, v, 0, True);
  if (!ctx || !glXMakeCurrent(d, w, ctx))
    return 4;
  const char *r = (const char *)glGetString(GL_RENDERER);
  printf("MIP renderer=%s direct=%d\n", r, glXIsDirect(d, ctx));
  fflush(stdout);
  if (argc != 2 || !r || strcmp(r, argv[1]))
    return 8;
  GLint ab = 0;
  glGetIntegerv(GL_ALPHA_BITS, &ab);
  if (ab < 8) {
    fprintf(stderr, "alpha bits=%d\n", ab);
    return 9;
  }
  GLfloat verts[] = {-1, -1, 0, 0, 1, -1, 1, 0, 1,  1, 1, 1,
                     -1, -1, 0, 0, 1, 1,  1, 1, -1, 1, 0, 1};
  glViewport(0, 0, 128, 128);
  glDisable(GL_DITHER);
  glDisable(GL_DEPTH_TEST);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), verts + 2);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  unsigned char colors[6][4] = {{240, 16, 32, 255},  {24, 232, 48, 255},
                                {40, 56, 224, 255},  {232, 216, 32, 255},
                                {208, 32, 192, 255}, {32, 200, 216, 128}};
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5);
  for (unsigned level = 0; level < 6; level++) {
    unsigned tw = 32 >> level, th = 8 >> level;
    if (!th)
      th = 1;
    unsigned char pixels[32 * 8 * 4];
    for (unsigned j = 0; j < tw * th; j++)
      memcpy(pixels + j * 4, colors[level], 4);
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, tw, th, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
  }
  GLuint mip_program = program(1);
  glUseProgram(mip_program);
  glUniform1i(glGetUniformLocation(mip_program, "image"), 0);
  glDisable(GL_BLEND);
  unsigned checks = 0;
  for (unsigned linear = 0; linear < 2; linear++) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    linear ? GL_LINEAR_MIPMAP_LINEAR
                           : GL_NEAREST_MIPMAP_LINEAR);
    for (unsigned level = 0; level < 5; level++)
      for (unsigned fraction = 0; fraction <= 4; fraction++) {
        float t = fraction * .25f, lod = level + t, scale = exp2f(lod);
        for (unsigned j = 0; j < 6; j++) {
          verts[j * 4 + 2] = verts[j * 4] > 0 ? 128.f / 32 * scale : 0;
          verts[j * 4 + 3] = verts[j * 4 + 1] > 0 ? 128.f / 8 * scale : 0;
        }
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glFinish();
        unsigned char actual[49 * 4];
        glReadPixels(57, 57, 7, 7, GL_RGBA, GL_UNSIGNED_BYTE, actual);
        check("trilinear readback");
        for (unsigned pix = 0; pix < 49; pix++)
          for (unsigned c = 0; c < 4; c++) {
            int expected = lroundf(colors[level][c] * (1 - t) +
                                   colors[level + 1][c] * t),
                got = actual[pix * 4 + c];
            if (abs(expected - got) > 8) {
              fprintf(stderr,
                      "MIP FAIL within_linear=%u lod=%g pixel=%u channel=%u "
                      "expected=%d got=%d\n",
                      linear, lod, pix, c, expected, got);
              return 10;
            }
            checks++;
          }
        printf("MIP PASS within_linear=%u lod=%g rgba=%u/%u/%u/%u\n", linear,
               lod, actual[0], actual[1], actual[2], actual[3]);
        fflush(stdout);
      }
  }
  printf("MIP ALL PASS components=%u alpha_bits=%d tolerance=8\n", checks, ab);
  glXMakeCurrent(d, None, 0);
  glXDestroyContext(d, ctx);
  XDestroyWindow(d, w);
  XFree(v);
  XCloseDisplay(d);
  return 0;
}
