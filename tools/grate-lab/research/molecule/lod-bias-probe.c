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
static GLuint program(int project) {
  const char *vs =
      "#version 120\nattribute vec2 position;attribute vec4 coord;varying vec4 "
      "uv;void main(){gl_Position=vec4(position,0,1);uv=coord;}";
  const char *fs =
      project == 2
          ? "#version 120\nvarying vec4 uv;uniform sampler2D image;uniform "
            "vec4 tint;void main(){gl_FragColor=texture2DProj(image,uv)*tint;}"
      : project ? "#version 120\nvarying vec4 uv;uniform sampler2D image;void "
                  "main(){gl_FragColor=texture2DProj(image,uv);}"
                : "#version 120\nvarying vec4 uv;uniform sampler2D image;void "
                  "main(){gl_FragColor=texture2D(image,uv.xy);}";
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
  XStoreName(d, w, "GRATE LOD bias validation");
  XMapWindow(d, w);
  XSync(d, False);
  GLXContext ctx = glXCreateContext(d, v, 0, True);
  if (!ctx || !glXMakeCurrent(d, w, ctx))
    return 4;
  const char *r = (const char *)glGetString(GL_RENDERER);
  printf("BIAS renderer=%s direct=%d\n", r, glXIsDirect(d, ctx));
  fflush(stdout);
  if (argc != 2 || !r || strcmp(r, argv[1]))
    return 8;
  GLint ab = 0;
  glGetIntegerv(GL_ALPHA_BITS, &ab);
  if (ab < 8) {
    fprintf(stderr, "alpha bits=%d\n", ab);
    return 9;
  }
  GLfloat verts[] = {-1, -1, 0, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1,  1, 1, 1, 0, 1,
                     -1, -1, 0, 0, 0, 1, 1, 1,  1, 1, 0, 1, -1, 1, 0, 1, 0, 1};
  glViewport(0, 0, 128, 128);
  glDisable(GL_DITHER);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), verts);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), verts + 2);
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

  const unsigned char colors[6][4] = {{240, 16, 32, 255},  {24, 232, 48, 255},
                                      {40, 56, 224, 255},  {232, 216, 32, 255},
                                      {208, 32, 192, 255}, {32, 200, 216, 128}};
  const float biases[] = {-16, -2, -1, -.25f, 0, .25f, 1, 2, 16},
              base_lods[] = {-1, 0, .5f, 2.5f, 4.5f, 6};
  unsigned checks = 0;
  for (unsigned project = 0; project < 3; project++) {
    GLuint p = program(project);
    glUseProgram(p);
    glUniform1i(glGetUniformLocation(p, "image"), 0);
    const float tint[4] = {.75f, .5f, .25f, 1};
    if (project == 2) {
      glUniform4fv(glGetUniformLocation(p, "tint"), 1, tint);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    } else
      glDisable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
    for (unsigned linear = 0; linear < 2; linear++) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      linear ? GL_LINEAR_MIPMAP_LINEAR
                             : GL_NEAREST_MIPMAP_LINEAR);
      for (unsigned bi = 0; bi < 9; bi++)
        for (unsigned li = 0; li < 6; li++) {
          float bias = biases[bi], base = base_lods[li], scale = exp2f(base),
                q = project ? 2 : 1;
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, bias);
          for (unsigned j = 0; j < 6; j++) {
            verts[j * 6 + 2] = verts[j * 6] > 0 ? 128.f / 32 * scale * q : 0;
            verts[j * 6 + 3] = verts[j * 6 + 1] > 0 ? 128.f / 8 * scale * q : 0;
            verts[j * 6 + 4] = 0;
            verts[j * 6 + 5] = q;
          }
          glClearColor(0, 0, 0, 0);
          glClear(GL_COLOR_BUFFER_BIT);
          glDrawArrays(GL_TRIANGLES, 0, 6);
          if (project == 2)
            glDrawArrays(GL_TRIANGLES, 0, 6);
          glFinish();
          unsigned char actual[49 * 4];
          glReadPixels(57, 57, 7, 7, GL_RGBA, GL_UNSIGNED_BYTE, actual);
          check("LOD bias readback");
          float lod = fminf(5, fmaxf(0, base + bias)), t = lod - floorf(lod);
          unsigned l = (unsigned)floorf(lod), h = l < 5 ? l + 1 : l;
          for (unsigned pix = 0; pix < 49; pix++)
            for (unsigned c = 0; c < 4; c++) {
              float source = colors[l][c] * (1 - t) + colors[h][c] * t;
              if (project == 2) {
                float a = (colors[l][3] * (1 - t) + colors[h][3] * t) / 255.f;
                source *= tint[c] * (2 - a);
              }
              int expected = lroundf(source), got = actual[pix * 4 + c];
              if (abs(expected - got) > 8) {
                fprintf(stderr,
                        "BIAS FAIL project=%u within_linear=%u base=%g bias=%g "
                        "lod=%g pixel=%u channel=%u expected=%d got=%d\n",
                        project, linear, base, bias, lod, pix, c, expected,
                        got);
                return 10;
              }
              checks++;
            }
          printf("BIAS PASS project=%u within_linear=%u base=%g bias=%g "
                 "rgba=%u/%u/%u/%u\n",
                 project, linear, base, bias, actual[0], actual[1], actual[2],
                 actual[3]);
          fflush(stdout);
        }
    }
    /* Same quadrant pattern at all three mip levels detects UV scaling tricks.
     */
    const unsigned char pattern[4][4] = {{255, 32, 16, 255},
                                         {16, 240, 48, 255},
                                         {24, 48, 232, 255},
                                         {224, 216, 32, 255}};
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST_MIPMAP_LINEAR);
    for (unsigned level = 0; level < 4; level++) {
      unsigned n = 8 >> level;
      unsigned char pixels[8 * 8 * 4];
      for (unsigned y = 0; y < n; y++)
        for (unsigned x = 0; x < n; x++)
          memcpy(pixels + (y * n + x) * 4,
                 pattern[(y >= n / 2) * 2 + (x >= n / 2)], 4);
      if (level == 3) {
        const unsigned char average[4] = {128, 128, 128, 255};
        memcpy(pixels, average, 4);
      }
      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, n, n, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, pixels);
    }
    const float uv_biases[] = {-16,  -2, -1,   -.25f, 0,
                               .25f, 1,  1.5f, 2}; /* LOD=-4: remain magnified
                                                      for coordinate checks. */
    float q = project ? 2 : 1;
    for (unsigned j = 0; j < 6; j++) {
      verts[j * 6 + 2] = verts[j * 6] > 0 ? q : 0;
      verts[j * 6 + 3] = verts[j * 6 + 1] > 0 ? q : 0;
      verts[j * 6 + 4] = 0;
      verts[j * 6 + 5] = q;
    }
    for (unsigned bi = 0; bi < 9; bi++) {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, uv_biases[bi]);
      glClear(GL_COLOR_BUFFER_BIT);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      if (project == 2)
        glDrawArrays(GL_TRIANGLES, 0, 6);
      glFinish();
      for (unsigned quadrant = 0; quadrant < 4; quadrant++) {
        unsigned char pixel[4];
        glReadPixels(quadrant & 1 ? 96 : 32, quadrant & 2 ? 96 : 32, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        check("bias preserves UV");
        for (unsigned c = 0; c < 4; c++) {
          if (abs((int)pixel[c] - (int)lroundf(pattern[quadrant][c] *
                                               (project == 2 ? tint[c] : 1))) >
              4) {
            fprintf(stderr,
                    "BIAS UV FAIL project=%u bias=%g quadrant=%u channel=%u "
                    "expected=%u got=%u\n",
                    project, uv_biases[bi], quadrant, c, pattern[quadrant][c],
                    pixel[c]);
            return 11;
          }
          checks++;
        }
      }
    }
    glDeleteProgram(p);
  }
  printf(
      "BIAS ALL PASS components=%u alpha_bits=%d tolerance=8 uv_tolerance=4\n",
      checks, ab);
  glXMakeCurrent(d, None, 0);
  glXDestroyContext(d, ctx);
  XDestroyWindow(d, w);
  XFree(v);
  XCloseDisplay(d);
  return 0;
}
