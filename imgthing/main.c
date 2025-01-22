#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stbi_image_write.h"
#include "common.h"
#include "perlin.c"
#include "color.c"
#include "diamondsquare.c"

#define WIDTH    2050
#define HEIGHT   1025
#define CHANNELS    3

typedef struct {
  uint32 w;
  uint32 h;
  uint8* buf;
} img;

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

#define MAX_COLOR_COMPONENT 255
#define TERRAIN_COLOR ((MAX_COLOR_COMPONENT / 3) * 2)
#define SEALEVEL 0.33
#define MTNLEVEL 0.7

static color_array terrainColors = NULL;
static color_array seaColors = NULL;

static heightmap terrainHeightMap = NULL;

void colorheightmap(img image, int32 x, int32 y, color24* out) {
  double noisex = x * 0.15;
  double noisey = y * 0.15;

  float noise = hmap_getsample(terrainHeightMap, x, y);

  if (noise < 0 || noise > 1.0f) {
    printf("Noise out of bounds 0..1: (%i %i)=%f\n", x, y, noise);
    return;
  }

  uint8 component = noise * 255;

  if (noise < SEALEVEL) {
    float rnoise = 1.0f - noise / SEALEVEL;
    pickcolor(out, seaColors, rnoise);
    return;
  }

  pickcolor(out, terrainColors, noise);
}

void shaderTest(img image, int32 x, int32 y, color24* out) {
  setc(out, 255);
}

void outlineLand(img image, int32 x, int32 y, color24* out) {
  float sample = hmap_getsample(terrainHeightMap, x, y);
  uint8 issea = sample < SEALEVEL;

  for (int32 xdif = -1; xdif < 2; xdif++) {
    for (int32 ydif = -1; ydif < 2; ydif++) {
      int32 px = x + xdif;
      int32 py = y + ydif;

      if (px < 0 || py < 0) {
        continue;
      }
      if (px >= image.w || py >= image.h) {
        continue;
      }

      float pxsample = hmap_getsample(terrainHeightMap, px, py);
      uint8 pxIsSea = pxsample < SEALEVEL;

      if (pxIsSea == issea) {
        continue;
      }

      setc(out, 0);
      return;
    }
  }

  color24 c = getcolor(image, x, y);
  // printf("Pre existing color: %x %x %x\n", c.r, c.g, c.b);

  out->r = c.r;
  out->g = c.g;
  out->b = c.b;
}


uint8 createpalettes() {
  terrainColors = colors_malloc(7);
  seaColors = colors_malloc(6);

  if (!terrainColors || !seaColors) {
    return 0;
  }

  color_array_set(terrainColors, 0, color(0x05, 0x78, 0x64));
  color_array_set(terrainColors, 1, color(0x0A, 0x9b, 0x50));
  color_array_set(terrainColors, 2, color(0x46, 0xaf, 0x64));
  color_array_set(terrainColors, 3, color(0x96, 0xc8, 0x69));
  color_array_set(terrainColors, 4, color(0xe1, 0xd7, 0x91));
  color_array_set(terrainColors, 5, color(0xcd, 0xb9, 0x91));
  color_array_set(terrainColors, 6, color(0x05, 0x78, 0x64));

  color_array_set(seaColors, 0, color(0xff, 0xff, 0xff));
  color_array_set(seaColors, 1, color(0xd7, 0xeb, 0xeb));
  color_array_set(seaColors, 2, color(0x96, 0xcd, 0xd2));
  color_array_set(seaColors, 3, color(0x5a, 0xaf, 0xb9));
  color_array_set(seaColors, 4, color(0x41, 0xa5, 0xb4));
  color_array_set(seaColors, 5, color(0x3c, 0x96, 0xaa));

  return 1;
}

uint8 generateheightmap() {
  heightmap hmap = hmap_alloc(WIDTH, HEIGHT);
  if (!hmap) {
    return 0;
  }

  terrainHeightMap = hmap;

  srand(time(NULL));
  hmap_generate(hmap);

  return 1;
}

int32 randomInt(int32 max) {
  uint32 r = rand();
  return r % max;
}

void paintRiverTile(img image, int32 centerx, int32 centery, float size) {
  int32 isize = (int32) size;
  int32 h = isize / 2;

  color24 c = color(0, 0, 255);

  for (int32 rx = -h; rx <= h; rx++) {
    for (int32 ry = -h; ry <= h; ry++) {
      setcolor(image, rx + centerx, ry + centery, c);
    }
  }
}

void placeRivers(img image) {
  uint32 rivers = 10;
  uint32 attempts = 0;

  while (rivers > 0 && attempts < 500) {
    attempts++;

    int32 rx = randomInt(image.w);
    int32 ry = randomInt(image.h);

    float sample = hmap_getsample(terrainHeightMap, rx, ry);

    if (sample < SEALEVEL) {
      continue;
    }

    rivers--;

    float riverGrowth = 0.1f;
    float riverWidth = 2.0f;

    while (1) {
      paintRiverTile(image, rx, ry, riverWidth);
      uint8 foundmove = 0;
      int32 searchRange = (int32) riverWidth;

      for (int32 dx = -searchRange; dx <= searchRange; dx++) {
        for (int32 dy = -searchRange; dy <= searchRange; dy++) {
          float dsample = hmap_getsample(terrainHeightMap, rx + dx, ry + dy);

          if (dsample < SEALEVEL) {
            goto afterloop;
          }

          if (dsample < sample) {
            sample = dsample;
            rx = rx + dx;
            ry = ry + dy;
            foundmove = 1;
            riverWidth += riverGrowth;
            goto afterloop;
          }
        }
      }

afterloop:
      if (!foundmove) {
        break;
      }
    }
  }
}

int32 main() {
  img image = allocImage(WIDTH, HEIGHT);

  if (!createpalettes()) {
    printf("Failed to allocate palettes.\n");
    return EXIT_FAILURE;
  }
  printf("Generated palettes...");

  if (!generateheightmap()) {
    printf("Failed to generate height map\n");
    return EXIT_FAILURE;
  }
  printf("Generated heightmap...\n");

  // applyshader(image, shaderTest);
  applyshader(image, colorheightmap);

  // placeRivers(image);

  applyshader(image, outlineLand);

  printf("Applied shader...\n");

  int32 result = stbi_write_png("testfile.png", WIDTH, HEIGHT, CHANNELS, image.buf, WIDTH * CHANNELS);
  printf("Wrote image! result=%i\n", result);

  return 0;
}
