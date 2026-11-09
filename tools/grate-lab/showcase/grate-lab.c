#define _POSIX_C_SOURCE 200809L
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

typedef struct {
   float x, y, z, u, v;
} Vertex;
typedef struct {
   float v[16];
} Matrix;
static Display *display;
static Window window;
static GLXContext context;
static Atom close_atom;
static GLuint program, checker, mips, font, palette, metal[3];
static GLint matrix_location;
static int width = 760, height = 1000;
static const char *renderer;
static unsigned checked_pixels;
static unsigned char mip_colors[8][4] = {
    {255, 40, 40, 255},  {40, 255, 40, 255},  {40, 80, 255, 255},  {255, 230, 40, 255},
    {255, 40, 230, 255}, {40, 240, 230, 255}, {180, 120, 60, 255}, {220, 220, 220, 255}};
static unsigned char palette_colors[8][4] = {
    {10, 17, 30, 255},   {26, 42, 65, 255},  {25, 210, 235, 255},  {60, 235, 150, 255},
    {250, 185, 55, 255}, {235, 65, 90, 255}, {255, 255, 255, 255}, {50, 80, 130, 255}};
static const unsigned char alphabet[36][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {14, 17, 16, 16, 16, 17, 14},
    {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 14}, {17, 17, 17, 31, 17, 17, 17}, {14, 4, 4, 4, 4, 4, 14},
    {7, 2, 2, 2, 18, 18, 12},     {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17}, {14, 17, 17, 17, 17, 17, 14},
    {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       {17, 17, 17, 17, 17, 17, 14},
    {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31},     {14, 17, 19, 21, 25, 17, 14},
    {4, 12, 4, 4, 4, 4, 14},      {14, 17, 1, 2, 4, 8, 31},     {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2},     {31, 16, 16, 30, 1, 1, 30},   {14, 16, 16, 30, 17, 17, 14},
    {31, 1, 2, 4, 8, 8, 8},       {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14}};
static double clock_seconds(void) {
   struct timespec t;
   clock_gettime(CLOCK_MONOTONIC, &t);
   return t.tv_sec + t.tv_nsec * 1e-9;
}
static void gl_check(const char *where) {
   GLenum e = glGetError();
   if (e) {
      fprintf(stderr, "GL FAIL %s 0x%x\n", where, e);
      exit(20);
   }
}
static Matrix identity(void) {
   Matrix m = {{0}};
   for (int i = 0; i < 4; i++)
      m.v[i * 5] = 1;
   return m;
}
static Matrix multiply(Matrix a, Matrix b) {
   Matrix r = {{0}};
   for (int c = 0; c < 4; c++)
      for (int row = 0; row < 4; row++)
         for (int k = 0; k < 4; k++)
            r.v[c * 4 + row] += a.v[k * 4 + row] * b.v[c * 4 + k];
   return r;
}
static Matrix translation(float x, float y, float z) {
   Matrix r = identity();
   r.v[12] = x;
   r.v[13] = y;
   r.v[14] = z;
   return r;
}
static Matrix rotation(float a, int axis) {
   Matrix r = identity();
   float c = cosf(a), s = sinf(a);
   if (axis == 2) {
      r.v[0] = c;
      r.v[1] = s;
      r.v[4] = -s;
      r.v[5] = c;
   } else {
      r.v[0] = c;
      r.v[2] = -s;
      r.v[8] = s;
      r.v[10] = c;
   }
   return r;
}
static Matrix ortho(float w, float h) {
   Matrix r = identity();
   r.v[0] = 2 / w;
   r.v[5] = -2 / h;
   r.v[12] = -1;
   r.v[13] = 1;
   return r;
}
static Matrix perspective(float aspect) {
   Matrix r = {{0}};
   float f = 2.3f, n = .1f, z = 30;
   r.v[0] = f / aspect;
   r.v[5] = f;
   r.v[10] = (z + n) / (n - z);
   r.v[11] = -1;
   r.v[14] = 2 * z * n / (n - z);
   return r;
}
static void matrix(Matrix m) { glUniformMatrix4fv(matrix_location, 1, GL_FALSE, m.v); }
static GLuint shader(GLenum type, const char *source) {
   GLuint s = glCreateShader(type);
   glShaderSource(s, 1, &source, NULL);
   glCompileShader(s);
   GLint ok;
   glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
   if (!ok) {
      char b[4096];
      glGetShaderInfoLog(s, sizeof(b), NULL, b);
      fprintf(stderr, "SHADER FAIL %s\n", b);
      exit(21);
   }
   return s;
}
static void setup(int small) {
   setvbuf(stdout, NULL, _IONBF, 0);
   setvbuf(stderr, NULL, _IONBF, 0);
   if (small)
      width = height = 256;
   display = XOpenDisplay(NULL);
   if (!display) {
      fprintf(stderr, "DISPLAY unavailable\n");
      exit(2);
   }
   int attrs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, None};
   XVisualInfo *visual = glXChooseVisual(display, DefaultScreen(display), attrs);
   if (!visual)
      exit(3);
   context = glXCreateContext(display, visual, NULL, True);
   if (!context)
      exit(4);
   XSetWindowAttributes wa = {.colormap =
                                  XCreateColormap(display, RootWindow(display, visual->screen),
                                                  visual->visual, AllocNone),
                              .event_mask = ExposureMask | KeyPressMask | StructureNotifyMask};
   window = XCreateWindow(display, RootWindow(display, visual->screen), 10, 20, width, height,
                          0, visual->depth, InputOutput, visual->visual,
                          CWColormap | CWEventMask, &wa);
   XStoreName(display, window, "GRATE LAB | Tegra 3 GPU showcase");
   close_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(display, window, &close_atom, 1);
   XMapWindow(display, window);
   XSync(display, False);
   if (!glXMakeCurrent(display, window, context))
      exit(5);
   renderer = (const char *)glGetString(GL_RENDERER);
   if (!renderer)
      exit(6);
   printf("RENDERER %s direct=%d\n", renderer, glXIsDirect(display, context));
   const char *vs =
       "#version 120\nattribute vec3 position;attribute vec2 texcoord;uniform mat4 "
       "mvp;varying vec2 uv;void main(){gl_Position=mvp*vec4(position,1.0);uv=texcoord;}";
   const char *fs = "#version 120\nuniform sampler2D image;varying vec2 uv;void "
                    "main(){gl_FragColor=texture2D(image,uv);}";
   program = glCreateProgram();
   glAttachShader(program, shader(GL_VERTEX_SHADER, vs));
   glAttachShader(program, shader(GL_FRAGMENT_SHADER, fs));
   glBindAttribLocation(program, 0, "position");
   glBindAttribLocation(program, 1, "texcoord");
   glLinkProgram(program);
   GLint ok;
   glGetProgramiv(program, GL_LINK_STATUS, &ok);
   if (!ok) {
      char b[4096];
      glGetProgramInfoLog(program, sizeof(b), NULL, b);
      fprintf(stderr, "LINK FAIL %s\n", b);
      exit(7);
   }
   glUseProgram(program);
   glUniform1i(glGetUniformLocation(program, "image"), 0);
   matrix_location = glGetUniformLocation(program, "mvp");
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glDisable(GL_DITHER);
   glDisable(GL_BLEND);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   gl_check("setup");
   XFree(visual);
}
static void draw(GLuint texture, const Vertex *v, unsigned count) {
   glBindTexture(GL_TEXTURE_2D, texture);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(*v), &v[0].x);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(*v), &v[0].u);
   glDrawArrays(GL_TRIANGLES, 0, count);
}
static void quad(GLuint t, float x, float y, float w, float h, float z, float u, float v,
                 float du, float dv) {
   Vertex p[6] = {
       {x, y, z, u, v}, {x + w, y, z, u + du, v},          {x + w, y + h, z, u + du, v + dv},
       {x, y, z, u, v}, {x + w, y + h, z, u + du, v + dv}, {x, y + h, z, u, v + dv}};
   draw(t, p, 6);
}
static GLuint texture(unsigned w, unsigned h, const unsigned char *data) {
   GLuint t;
   glGenTextures(1, &t);
   glBindTexture(GL_TEXTURE_2D, t);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
   return t;
}
static unsigned glyph_row(unsigned char c, unsigned y) {
   c = toupper(c);
   if (c >= 'A' && c <= 'Z')
      return alphabet[c - 'A'][y];
   if (c >= '0' && c <= '9')
      return alphabet[26 + c - '0'][y];
   if (c == '-')
      return y == 3 ? 31 : 0;
   if (c == ':')
      return (y == 2 || y == 5) ? 4 : 0;
   if (c == '.')
      return y == 6 ? 4 : 0;
   if (c == '/')
      return 1u << (y * 4 / 6);
   if (c == '+')
      return y == 3 ? 31 : (y >= 1 && y <= 5 ? 4 : 0);
   if (c == '(')
      return y == 0 || y == 6 ? 2 : 4;
   if (c == ')')
      return y == 0 || y == 6 ? 8 : 4;
   return 0;
}
static void textures(int with_mips) {
   unsigned char *p = malloc(256 * 256 * 4);
   if (!p)
      exit(8);
   for (unsigned y = 0; y < 64; y++)
      for (unsigned x = 0; x < 64; x++) {
         unsigned k = (y * 64 + x) * 4;
         p[k] = x < 32 ? 255 : 0;
         p[k + 1] = y < 32 ? 255 : 0;
         p[k + 2] = x < 32 ? 0 : 255;
         p[k + 3] = 255;
      }
   checker = texture(64, 64, p);
   for (unsigned y = 0; y < 256; y++)
      for (unsigned x = 0; x < 128; x++) {
         unsigned layer = y / 64, ch = ((y % 64) / 8) * 16 + x / 8, k = (y * 128 + x) * 4;
         unsigned on =
             (x % 8 < 5 && y % 8 < 7) ? (glyph_row(ch, y % 8) >> (4 - x % 8)) & 1 : 0;
         static const unsigned char ink[4][3] = {
             {220, 232, 246}, {60, 235, 150}, {250, 185, 55}, {235, 65, 90}};
         for (unsigned c = 0; c < 3; c++)
            p[k + c] = on ? ink[layer][c] : palette_colors[0][c];
         p[k + 3] = 255;
      }
   font = texture(128, 256, p);
   for (unsigned y = 0; y < 8; y++)
      for (unsigned x = 0; x < 64; x++) {
         unsigned k = (y * 64 + x) * 4;
         memcpy(p + k, palette_colors[x / 8], 4);
      }
   palette = texture(64, 8, p);
   for (unsigned gear = 0; gear < 3; gear++) {
      for (unsigned y = 0; y < 128; y++)
         for (unsigned x = 0; x < 128; x++) {
            unsigned k = (y * 128 + x) * 4;
            int grid = ((x / 8) ^ (y / 8)) & 1;
            int grain = (x * 13 + y * 7) % 17;
            for (unsigned c = 0; c < 3; c++) {
               unsigned char base[3][3] = {{38, 175, 220}, {225, 137, 48}, {114, 196, 134}};
               p[k + c] = base[gear][c] * (grid ? 1 : .65f) + grain;
            }
            p[k + 3] = 255;
         }
      metal[gear] = texture(128, 128, p);
      if (with_mips) {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
         unsigned previous = 128;
         for (unsigned level = 1; level < 8; level++) {
            unsigned n = previous / 2;
            for (unsigned y = 0; y < n; y++)
               for (unsigned x = 0; x < n; x++)
                  for (unsigned c = 0; c < 4; c++) {
                     unsigned a = ((y * 2) * previous + x * 2) * 4 + c;
                     p[(y * n + x) * 4 + c] =
                         (p[a] + p[a + 4] + p[a + previous * 4] + p[a + previous * 4 + 4]) / 4;
                  }
            glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
            previous = n;
         }
      }
   }
   if (with_mips) {
      glGenTextures(1, &mips);
      glBindTexture(GL_TEXTURE_2D, mips);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      for (unsigned l = 0; l < 8; l++) {
         unsigned n = 128 >> l;
         for (unsigned k = 0; k < n * n; k++)
            memcpy(p + k * 4, mip_colors[l], 4);
         glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
      }
   }
   free(p);
   gl_check("textures");
}
static void solid(float x, float y, float w, float h, unsigned color, float z) {
   float u = (color * 8 + 4) / 64.f;
   quad(palette, x, y, w, h, z, u, .5f, 0, 0);
}
static unsigned char readback[128 * 128 * 4];
static void read128(void) {
   glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, readback);
   gl_check("readback");
}
static int sample(unsigned x, unsigned y, const unsigned char expected[4], const char *name) {
   const unsigned char *p = readback + (y * 128 + x) * 4;
   for (unsigned c = 0; c < 4; c++)
      if (abs((int)p[c] - expected[c]) > 1) {
         printf("CHECK FAIL %s at=%u,%u got=%u,%u,%u,%u expected=%u,%u,%u,%u\n", name, x, y,
                p[0], p[1], p[2], p[3], expected[0], expected[1], expected[2], expected[3]);
         return 0;
      }
   checked_pixels++;
   return 1;
}
static void test_begin(void) {
   glViewport(0, 0, 128, 128);
   glDisable(GL_DEPTH_TEST);
   glClearColor(0, 0, 0, 1);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   matrix(ortho(128, 128));
}
static int test_texture(void) {
   test_begin();
   quad(checker, 0, 0, 128, 128, 0, 0, 0, 1, 1);
   glFinish();
   read128();
   for (unsigned y = 8; y < 120; y++)
      for (unsigned x = 8; x < 120; x++) {
         if ((x >= 60 && x < 68) || (y >= 60 && y < 68))
            continue;
         unsigned char expected[4] = {x < 64 ? 255 : 0, y >= 64 ? 255 : 0, x < 64 ? 0 : 255,
                                      255};
         if (!sample(x, y, expected, "texture"))
            return 0;
      }
   gl_check("texture-test");
   puts("CHECK PASS texture pixels");
   return 1;
}
static int test_mip(void) {
   test_begin();
   glBindTexture(GL_TEXTURE_2D, mips);
   unsigned char pixels[128 * 128 * 4];
   for (unsigned l = 0; l < 8; l++) {
      unsigned n = 128 >> l;
      glGetTexImage(GL_TEXTURE_2D, l, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      for (unsigned k = 0; k < n * n; k++)
         if (memcmp(pixels + k * 4, mip_colors[l], 4)) {
            printf("CHECK FAIL mip storage level=%u\n", l);
            return 0;
         }
   }
   for (unsigned l = 0; l < 8; l++) {
      unsigned n = 128 >> l;
      glClear(GL_COLOR_BUFFER_BIT);
      quad(mips, 0, 0, n, n, 0, 0, 0, 1, 1);
      glFinish();
      read128();
      if (!sample(n / 2, 127 - n / 2, mip_colors[l], "mipmap")) {
         printf("MIP LOD FAIL expected_level=%u footprint=%u\n", l, n);
         return 0;
      }
      printf("MIP LOD PASS level=%u footprint=%u\n", l, n);
   }
   gl_check("mip-test");
   puts("CHECK PASS mip storage and implicit GPU LOD 8/8");
   return 1;
}
static Vertex letters[16384];
static unsigned letter_count;
static void text(float x, float y, float scale, const char *s, unsigned style) {
   for (; *s; s++, x += 6 * scale) {
      unsigned c = toupper((unsigned char)*s) & 127;
      float u = (c % 16) * 8 / 128.f, v = ((c / 16) * 8 + style * 64) / 256.f;
      float w = 8 * scale, h = 8 * scale;
      Vertex p[6] = {{x, y, 0, u, v},
                     {x + w, y, 0, u + 8 / 128.f, v},
                     {x + w, y + h, 0, u + 8 / 128.f, v + 8 / 256.f},
                     {x, y, 0, u, v},
                     {x + w, y + h, 0, u + 8 / 128.f, v + 8 / 256.f},
                     {x, y + h, 0, u, v + 8 / 256.f}};
      assert(letter_count + 6 <= 16384);
      memcpy(letters + letter_count, p, sizeof(p));
      letter_count += 6;
   }
}
static void text_flush(void) {
   if (letter_count)
      draw(font, letters, letter_count);
   letter_count = 0;
}
static int test_text(void) {
   test_begin();
   letter_count = 0;
   text(8, 8, 2, "GRATE", 0);
   text_flush();
   glFinish();
   read128();
   for (unsigned g = 0; g < 5; g++)
      for (unsigned y = 0; y < 7; y++)
         for (unsigned x = 0; x < 5; x++) {
            unsigned on = (glyph_row("GRATE"[g], y) >> (4 - x)) & 1;
            unsigned char expected[4];
            for (unsigned c = 0; c < 3; c++)
               expected[c] = on ? (unsigned char[]){220, 232, 246}[c] : palette_colors[0][c];
            expected[3] = 255;
            if (!sample(8 + g * 12 + x * 2, 127 - (8 + y * 2), expected, "text"))
               return 0;
         }
   gl_check("text-test");
   puts("CHECK PASS opaque OpenGL font atlas");
   return 1;
}
static int test_depth(void) {
   test_begin();
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   solid(0, 0, 128, 128, 5, .5f);
   solid(20, 20, 88, 88, 2, -.5f);
   solid(0, 0, 128, 128, 3, .7f);
   glFinish();
   read128();
   int pass = sample(64, 64, palette_colors[2], "depth-near") &&
              sample(4, 4, palette_colors[5], "depth-far");
   glDisable(GL_DEPTH_TEST);
   gl_check("depth-test");
   if (pass)
      puts("CHECK PASS depth occlusion");
   return pass;
}
static Vertex mesh[10000];
static unsigned mesh_count;
static void triangle(Vertex a, Vertex b, Vertex c) {
   assert(mesh_count + 3 <= 10000);
   mesh[mesh_count++] = a;
   mesh[mesh_count++] = b;
   mesh[mesh_count++] = c;
}
static Vertex point(float x, float y, float z) {
   return (Vertex){x, y, z, x * .4f + .5f, y * .4f + .5f};
}
static void gear_mesh(unsigned teeth) {
   mesh_count = 0;
   unsigned n = teeth * 4;
   const float tau = 6.28318530718f;
   for (unsigned i = 0; i < n; i++) {
      float a = tau * i / n, b = tau * (i + 1) / n,
            ra = (i % 4 == 1 || i % 4 == 2) ? 1.05f : .88f,
            rb = ((i + 1) % 4 == 1 || (i + 1) % 4 == 2) ? 1.05f : .88f;
      float x0 = cosf(a), y0 = sinf(a), x1 = cosf(b), y1 = sinf(b);
      Vertex f0 = point(ra * x0, ra * y0, .16), f1 = point(rb * x1, rb * y1, .16),
             f2 = point(.35 * x1, .35 * y1, .16), f3 = point(.35 * x0, .35 * y0, .16);
      Vertex b0 = f0, b1 = f1, b2 = f2, b3 = f3;
      b0.z = b1.z = b2.z = b3.z = -.16;
      triangle(f0, f1, f2);
      triangle(f0, f2, f3);
      triangle(b0, b2, b1);
      triangle(b0, b3, b2);
      triangle(f0, b0, b1);
      triangle(f0, b1, f1);
      triangle(f2, b2, b3);
      triangle(f2, b3, f3);
   }
}
static void panel(float x, float y, float w, float h) {
   solid(x - 1, y - 1, w + 2, h + 2, 1, 0);
   solid(x, y, w, h, 0, 0);
}
static void screenshot(const char *name);
static void frame(double t, double fps, int tests, const char *snapshot) {
   glViewport(0, 0, width, height);
   glDisable(GL_DEPTH_TEST);
   glClearColor(.025, .045, .08, 1);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   matrix(ortho(760, 1000));
   letter_count = 0;
   panel(16, 16, 728, 66);
   panel(16, 96, 728, 454);
   panel(16, 566, 228, 182);
   panel(260, 566, 228, 182);
   panel(504, 566, 240, 182);
   panel(16, 764, 728, 218);
   text(30, 29, 3, "GRATE LAB", 0);
   char b[160];
   snprintf(b, sizeof(b), "%s   %.1f FPS", renderer, fps);
   text(300, 40, 2, b, 1);
   text(30, 112, 2, "3D GEOMETRY + TEXTURES + DEPTH", 0);
   glViewport(30, height - 532, width - 60, 374);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   Matrix camera = multiply(perspective((width - 60) / 374.f),
                            translation(0, 0, -8.5f - 1.5f * sinf(t * .3f)));
   camera = multiply(camera, rotation(.3f * sinf(t * .4f), 1));
   const float xy[3][2] = {{-1.05, -.28}, {.93, -.28}, {0, 1.15}};
   for (unsigned g = 0; g < 3; g++) {
      Matrix m = multiply(camera, translation(xy[g][0], xy[g][1], 0));
      m = multiply(m, rotation(.25f, 1));
      m = multiply(m, rotation((g == 1 ? -1 : 1) * t * .65f + g * .18f, 2));
      matrix(m);
      gear_mesh(g == 2 ? 10 : 14);
      draw(metal[g], mesh, mesh_count);
   }
   glDisable(GL_DEPTH_TEST);
   glViewport(0, 0, width, height);
   matrix(ortho(760, 1000));
   text(30, 582, 2, "RGBA TEXTURE", 0);
   quad(checker, 48, 614, 164, 112, 0, 0, 0, 1, 1);
   text(274, 582, 2, "GL FONT ATLAS", 0);
   text(281, 624, 4, "GRATE", 1);
   text(281, 672, 2, "0123456789", 0);
   text(281, 703, 2, "TEXT GPU", 1);
   text(518, 582, 2, "GPU MIP LOD", 0);
   float x = 520;
   for (unsigned l = 1; l < 8; l++) {
      unsigned n = 128 >> l;
      quad(mips, x, 620, n, n, 0, 0, 0, 1, 1);
      char digit[2] = {'0' + l, 0};
      text(x, 699, 1.5, digit, 0);
      x += n + 7;
   }
   text(518, 726, 1.5, "8 LEVELS CHECKED", 1);
   text(32, 785, 2, "AUTOMATIC FRAMEBUFFER CHECKS", 0);
   text(32, 820, 2, tests ? "PASS TEXTURES  PASS TEXT  PASS DEPTH" : "FAIL - SEE TERMINAL",
        tests ? 1 : 3);
   text(32, 854, 2, "PASS MIP STORAGE + GPU LOD 8/8", 1);
   text(32, 888, 1.6, "EXPLICIT RGBA MIPS / OPAQUE TEXT ATLAS", 0);
   text(32, 920, 1.6, "NO CLAIM OF FULL OPENGL CONFORMANCE", 2);
   text(32, 950, 1.6, "ESC: CLOSE   AUTO STOP: BOUNDED RUN", 0);
   text_flush();
   gl_check("frame");
   if (snapshot) {
      glFinish();
      screenshot(snapshot);
   }
   glXSwapBuffers(display, window);
}
static void screenshot(const char *name) {
   unsigned char *p = malloc(width * height * 3);
   if (!p)
      exit(30);
   glReadBuffer(GL_BACK);
   glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, p);
   FILE *f = fopen(name, "wb");
   if (!f)
      exit(31);
   fprintf(f, "P6\n%d %d\n255\n", width, height);
   for (int y = height - 1; y >= 0; y--)
      fwrite(p + y * width * 3, 1, width * 3, f);
   fclose(f);
   free(p);
}
int main(int argc, char **argv) {
   const char *test = NULL, *capture = NULL;
   double seconds = 15;
   int self = 0;
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--test") && i + 1 < argc)
         test = argv[++i];
      else if (!strcmp(argv[i], "--self-test"))
         self = 1;
      else if (!strcmp(argv[i], "--seconds") && i + 1 < argc)
         seconds = atof(argv[++i]);
      else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
         capture = argv[++i];
      else {
         fprintf(stderr,
                 "usage: %s [--test texture|text|depth|mipmap] [--self-test] [--seconds "
                 "1..30] [--capture FILE.ppm]\n",
                 argv[0]);
         return 1;
      }
   }
   if (seconds < 1 || seconds > 30)
      return 1;
   if (test && strcmp(test, "texture") && strcmp(test, "text") && strcmp(test, "depth") &&
       strcmp(test, "mipmap"))
      return 1;
   setup(test || self);
   textures(!test || !strcmp(test, "mipmap"));
   int pass = 1;
   if (!test || !strcmp(test, "texture"))
      pass = pass && test_texture();
   if (!test || !strcmp(test, "text"))
      pass = pass && test_text();
   if (!test || !strcmp(test, "depth"))
      pass = pass && test_depth();
   if (!test || !strcmp(test, "mipmap"))
      pass = pass && test_mip();
   printf("SELFTEST %s pixels=%u\n", pass ? "PASS" : "FAIL", checked_pixels);
   if (!pass)
      return 40;
   if (!test && !self) {
      double start = clock_seconds(), last = start, fps = 0;
      unsigned frames = 0, interval = 0;
      int quit = 0, captured = 0;
      while (!quit && clock_seconds() - start < seconds) {
         while (XPending(display)) {
            XEvent e;
            XNextEvent(display, &e);
            if (e.type == ClientMessage && (Atom)e.xclient.data.l[0] == close_atom)
               quit = 1;
            if (e.type == KeyPress && XLookupKeysym(&e.xkey, 0) == XK_Escape)
               quit = 1;
         }
         double t = clock_seconds();
         const char *shot = capture && !captured && t - start > 2 ? capture : NULL;
         frame(t - start, fps, pass, shot);
         if (shot)
            captured = 1;
         frames++;
         interval++;
         if (t - last >= 1) {
            fps = interval / (t - last);
            interval = 0;
            last = t;
         }
      }
      double elapsed = clock_seconds() - start;
      printf("DEMO DONE frames=%u seconds=%.3f fps=%.3f\n", frames, elapsed, frames / elapsed);
   }
   glXMakeCurrent(display, None, NULL);
   glXDestroyContext(display, context);
   XDestroyWindow(display, window);
   XCloseDisplay(display);
   puts("APP DONE exit=0");
   return 0;
}
