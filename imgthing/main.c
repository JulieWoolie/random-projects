#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stbi_image_write.h"
#include "perlin.c"

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

#define color(re,gr,bl) {.r = re, .g = gr, .b = bl}
#define lerp(v,min,max) (min + v * (max - min))
#define lerpf(v,min,max) (((float) min) + v * (((float) max) - ((float) min)))
#define relto(v, min, max) ((v - min) / (max - min))

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

void colorMult(color24* dest, float mod) {
  dest->r = (uint8) dest->r * mod;
  dest->g = (uint8) dest->g * mod;
  dest->b = (uint8) dest->b * mod;
}

#define MAX_COLOR_COMPONENT 255
#define TERRAIN_COLOR ((MAX_COLOR_COMPONENT / 3) * 2)
#define SEALEVEL 0.33
#define MTNLEVEL 0.7

#define TERRAIN_COLORS 3
static const color24 terrainColors[TERRAIN_COLORS] = {
  color(0, TERRAIN_COLOR, 0),
  color(TERRAIN_COLOR, TERRAIN_COLOR, 0),
  color(TERRAIN_COLOR, TERRAIN_COLOR, TERRAIN_COLOR)
};

#define USESMARTLERP 0

void lerpcolors(color24* out, color24* arr, uint32 count, float t) {
  if (!USESMARTLERP) {
    uint32 idx = count * t;
    *out = arr[idx];
    return;
  }

  uint32 maxi = count - 1;
  uint32 fidx = t * maxi;
  float firststep = (float) fidx / maxi;
  float localstep = (t - firststep) * maxi;

  color24 c1 = arr[fidx];
  color24 c2 = arr[fidx + 1];

  out->r = lerpf(localstep, c1.r, c2.r);
  out->g = lerpf(localstep, c1.g, c2.g);
  out->b = lerpf(localstep, c1.b, c2.b);
}

float minNoise = 100.0f;
float maxNoise = -100.0f;

void testshader(img image, int32 x, int32 y, color24* out) {
  double noisex = x * 0.15;
  double noisey = y * 0.15;

  float noise = (perlin2d(noisex, noisey, 0.2, 4));

  if (noise < minNoise) {
    minNoise = noise;
  }
  if (noise > maxNoise) {
    maxNoise = noise;
  }
  
  noise = relto(noise, SEALEVEL, 1.0f);
  lerpcolors(out, terrainColors, TERRAIN_COLORS, noise);

  // if (noise < SEALEVEL) {
  //   out->r = 0;
  //   out->g = 0;
  //   out->b = TERRAIN_COLOR;
  //   noise = 1 - noise;
  // } else if (noise > MTNLEVEL) {
  //   setc(out, TERRAIN_COLOR);
  // } else {
  //   out->r = 0;
  //   out->g = TERRAIN_COLOR;
  //   out->b = 0;
  // }
}

int32 main() {
  img image = allocImage(WIDTH, HEIGHT);

  color24 c = {.r = 255, .g = 0, .b = 0};

  applyshader(image, testshader);

  // drawline(image, c, 0, 0, WIDTH, HEIGHT);
  // drawline(image, c, WIDTH, 0, 0, HEIGHT);

  int32 result = stbi_write_png("testfile.png", WIDTH, HEIGHT, CHANNELS, image.buf, WIDTH * CHANNELS);
  printf("Wrote image! result=%i\n", result);

  for (uint32 i = 0; i < TERRAIN_COLORS; i++) {
    color24 c = terrainColors[i];
    printf("r=%i g=%i b=%i\n", c.r, c.g, c.b);
  }

  printf("minNoise=%f maxNoise=%f", minNoise, maxNoise);

  return 0;
}
