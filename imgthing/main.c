#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stbi_image.h"
#include "stbi_image_write.h"

#define WIDTH    1024
#define HEIGHT   1024
#define CHANNELS    3

typedef unsigned char uint8;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

typedef struct {
  uint8 r;
  uint8 g;
  uint8 b;
} color24;

typedef struct {
  uint32 w;
  uint32 h;
  uint8* buf;
} img;

img allocImage(uint32 w, uint32 h) {
  uint64 memsize = CHANNELS * w * h;
  uint8* buf = (uint8*) malloc(memsize);
  img i = {
    .w = w, 
    .h = h, 
    .buf = buf
  };

  if (buf) {
    memset(buf, 255, memsize);
  }

  return i;
}

void freeimg(img i) {
  if (!i.buf) {
    return;
  }

  free(i.buf);
}

uint8* offsetBy(img image, int32 x, int32 y) {
  uint64 offset = 0;
  offset += x;
  offset += y * image.w;
  offset *= CHANNELS;
  return image.buf + offset;
}

void setcolor(img image, int32 x, int32 y, color24 color) {
  uint8* ptr = offsetBy(image, x, y);
  ptr[0] = color.r;
  ptr[1] = color.g;
  ptr[2] = color.b;
}

color24 getcolor(img image, int32 x, int32 y) {
  uint8* ptr = offsetBy(image, x, y);
  color24 res = {
    .r = ptr[0],
    .g = ptr[1],
    .b = ptr[2]
  };

  return res;
}

void drawline(img image, color24 c, int32 xfrom, int32 yfrom, int32 xto, int32 yto) {
  float xdif = xto - xfrom;
  float ydif = yto - yfrom;
  float dist = sqrt((xdif * xdif) + (ydif * ydif));

  float xstep = xdif / dist;
  float ystep = ydif / dist;

  float x = xfrom;
  float y = yfrom;

  for (int32 i = 0; i < dist; i++) {
    int32 px = floorf(x);
    int32 py = floorf(y);

    setcolor(image, px, py, c);

    x += xstep;
    y += ystep;
  }
}

typedef void (*shader)(img image, int32 x, int32 y, color24* out);

void applyshader(img image, shader shaderFunc) {
  uint32 imgsize = image.w * image.h;
  uint32 pixelsDone = 0;

  color24 c = {
    .r = 0,
    .g = 0,
    .b = 0
  };

  uint8* ptr = image.buf;

  for (int32 y = 0; y < image.h; y++) {
    for (int32 x = 0; x < image.w; x++) {
      shaderFunc(image, x, y, &c);
      ptr[0] = c.r;
      ptr[1] = c.g;
      ptr[2] = c.b;
      ptr += CHANNELS;
    }
  }
}

float f(float x) {
  return (x + 1.0f) / 2.0f;
}

void setc(color24* out, uint8 v) {
  out->r = v;
  out->g = v;
  out->b = v;
}

void testshader(img image, int32 x, int32 y, color24* out) {
  int32 cdistx = (image.w / 2) - x;
  int32 cdisty = (image.h / 2) - y;

  int32 cidst = sqrt((cdistx * cdistx) + (cdisty * cdisty));

  if (cidst % 40 == 0) {
    setc(out, 255);
    return;
  }

  setc(out, 0);
}

int32 main() {
  img image = allocImage(WIDTH, HEIGHT);

  color24 c = {.r = 255, .g = 0, .b = 0};

  applyshader(image, testshader);

  drawline(image, c, 0, 0, WIDTH, HEIGHT);
  drawline(image, c, WIDTH, 0, 0, HEIGHT);

  int32 result = stbi_write_png("testfile.png", WIDTH, HEIGHT, CHANNELS, image.buf, WIDTH * CHANNELS);
  printf("Wrote image! result=%i", result);
  return 0;
}
