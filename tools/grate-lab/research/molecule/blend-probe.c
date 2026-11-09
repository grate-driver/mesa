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
  printf("BLEND renderer=%s direct=%d\n", r, glXIsDirect(d, ctx));
  fflush(stdout);
  if (argc != 2 || !r || strcmp(r, argv[1]))
    return 8;
  GLint ab = 0;
  glGetIntegerv(GL_ALPHA_BITS, &ab);
  if (ab < 8) {
    fprintf(stderr, "alpha bits=%d\n", ab);
    return 9;
  }
  const GLfloat verts[] = {-1, -1, 0, 0, 1, -1, 1, 0, 1,  1, 1, 1,
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
  const float dest[] = {.125f, .5f, .75f, .375f};
  unsigned checked = 0;
  for (unsigned tex = 0; tex < 2; tex++) {
    GLuint p = program(tex);
    glUseProgram(p);
    if (tex)
      glUniform1i(glGetUniformLocation(p, "image"), 0);
    for (unsigned mode = 1; mode <= 3; mode++)
      for (unsigned k = 0; k <= 4; k++) {
        float alpha = k * .25f;
        unsigned char bytes[4] = {204, 77, 153,
                                  (unsigned char)lroundf(alpha * 255)};
        if (mode == 1)
          for (unsigned c = 0; c < 3; c++)
            bytes[c] = (unsigned char)lroundf(bytes[c] * alpha);
        float src[4];
        for (unsigned c = 0; c < 4; c++)
          src[c] = bytes[c] / 255.f;
        alpha = src[3];
        if (tex) {
          unsigned char data[8 * 8 * 4];
          for (unsigned i = 0; i < 64; i++)
            memcpy(data + i * 4, bytes, 4);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA,
                       GL_UNSIGNED_BYTE, data);
        } else
          glUniform4fv(glGetUniformLocation(p, "color"), 1, src);
        glDisable(GL_BLEND);
        glClearColor(dest[0], dest[1], dest[2], dest[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(
            mode == 1 ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
            mode == 2 ? GL_SRC_ALPHA : GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        for (unsigned layer = 1; layer <= 2; layer++) {
          glDrawArrays(GL_TRIANGLES, 0, 6);
          glFinish();
          unsigned char actual[7 * 7 * 4];
          glReadPixels(57, 57, 7, 7, GL_RGBA, GL_UNSIGNED_BYTE, actual);
          check("blend/readback");
          float expected[4];
          for (unsigned c = 0; c < 4; c++) {
            float factor =
                c == 3 ? (mode == 2 ? alpha : 1) : (mode == 1 ? 1 : alpha);
            float prior = dest[c];
            for (unsigned j = 0; j < layer; j++)
              prior = src[c] * factor + prior * (1 - alpha);
            expected[c] = prior;
          }
          for (unsigned pix = 0; pix < 49; pix++)
            for (unsigned c = 0; c < 4; c++) {
              int e = (int)lroundf(expected[c] * 255),
                  got = actual[pix * 4 + c];
              if (abs(got - e) > 4) {
                fprintf(stderr,
                        "BLEND FAIL tex=%u mode=%u alpha=%.5f layer=%u "
                        "pixel=%u component=%u expected=%d got=%d\n",
                        tex, mode, alpha, layer, pix, c, e, got);
                for (unsigned yy = 0; yy < 7; yy++) {
                  fprintf(stderr, "GRID y=%u", yy);
                  for (unsigned xx = 0; xx < 7; xx++) {
                    unsigned q = (yy * 7 + xx) * 4;
                    fprintf(stderr, " %02x%02x%02x%02x", actual[q],
                            actual[q + 1], actual[q + 2], actual[q + 3]);
                  }
                  fputc('\n', stderr);
                }
                return 10;
              }
              checked++;
            }
        }
        printf("BLEND PASS tex=%u mode=%u alpha=%.5f\n", tex, mode, alpha);
        fflush(stdout);
      }
    glDeleteProgram(p);
  }
  printf("BLEND ALL PASS components=%u alpha_bits=%d\n", checked, ab);
  glXMakeCurrent(d, None, 0);
  glXDestroyContext(d, ctx);
  XDestroyWindow(d, w);
  XFree(v);
  XCloseDisplay(d);
  return 0;
}
