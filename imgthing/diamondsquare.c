
#include "common.h"
#include <stdlib.h>
#include <string.h>

#define TO_HMAP_PTR(x, y, hmap) (hmap->heightData + ((x + (y * hmap->width)) * sizeof(float)))

typedef struct {
  uint32 width;
  uint32 height;
  float greatestValue;
  float smallestValue;
  float* heightData;
} heightmap_t;

typedef heightmap_t* heightmap;

heightmap hmap_alloc(uint32 w, uint32 h) {
  uint64 memlen = w * h * sizeof(float);
  float* data = (float*) malloc(memlen);
  heightmap hmap = (heightmap) malloc(sizeof(heightmap_t));

  if (!data || !hmap) {
    return NULL;
  }

  for (uint32 idx = 0; idx < w * h; idx++) {
    data[idx] = 0.333f;
  }

  hmap->width = w;
  hmap->height = h;
  hmap->greatestValue = 0.0f;
  hmap->smallestValue = 0.0f;
  hmap->heightData = data;

  return hmap;
}

void hmap_free(heightmap hmap)  {
  if (!hmap) {
    return;
  }

  free(hmap->heightData);
  free(hmap);
}

float hmap_getsample(heightmap hmap, uint32 x, uint32 y) {
  if (!hmap || x < 0 || y < 0 || x > hmap->width || y >= hmap->height) {
    return 0.0f;
  }

  return hmap->heightData[x + (y * hmap->width)];
}

void hmap_setsample(heightmap hmap, uint32 x, uint32 y, float val) {
  if (!hmap || x < 0 || y < 0 || x > hmap->width || y >= hmap->height) {
    return;
  }

  hmap->heightData[x + (y * hmap->width)] = val;

  //printf("set sample at %i %i to %f\n", x, y, val);

  if (val > hmap->greatestValue) {
    hmap->greatestValue = val;
  }
  if (val < hmap->smallestValue) {
    hmap->smallestValue = val;
  }
}

static float random(int32 reach) {
  float r = (float)rand() / RAND_MAX;
  return r * 2 * reach - reach;
}

static void squareStep(heightmap hmap, uint32 x, uint32 y, int32 reach) {
  uint32 count = 0;
  float avg = 0.0f;

  if (x - reach >= 0 && y - reach >= 0) {
    avg += hmap_getsample(hmap, x - reach, y - reach);
    count++;
  }
  if (x - reach >= 0 && y + reach < hmap->height) {
    avg += hmap_getsample(hmap, x - reach, y + reach);
    count++;
  }
  if (x + reach < hmap->width && y - reach >= 0) {
    avg += hmap_getsample(hmap, x + reach, y - reach);
    count++;
  }
  if (x + reach < hmap->width && y + reach < hmap->height) {
    avg += hmap_getsample(hmap, x + reach, y + reach);
    count++;
  }

  avg += random(reach);
  avg /= count;

  hmap_setsample(hmap, x, y, avg);
}

static void diamondStep(heightmap hmap, uint32 x, uint32 y, int32 reach) {
  uint32 count = 0;
  float avg = 0.0f;

  if (x - reach >= 0) {
    avg += hmap_getsample(hmap, x - reach, y);
    count++;
  }
  if (x + reach < hmap->width) {
    avg += hmap_getsample(hmap, x + reach, y);
    count++;
  }
  if (y - reach >= 0) {
    avg += hmap_getsample(hmap, x, y - reach);
    count++;
  }
  if (y + reach < hmap->height) {
    avg += hmap_getsample(hmap, x, y + reach);
    count++;
  }

  avg += random(reach);
  avg /= count;

  hmap_setsample(hmap, x, y, avg);
}

static void hmap_generate_step(heightmap hmap, uint32 size) {
  uint32 half = size / 2;
  if (half < 1) {
    return;
  }

  uint32 w = hmap->width;
  uint32 h = hmap->height;

  for (uint32 y = half; y < h; y += size) {
    for (uint32 x = half; x < w; x += size) {
      squareStep(hmap, x % w, y % h, half);
    }
  }

  uint32 col = 0;
  for (uint32 x = 0; x < w; x += half) {
    col++;

    uint32 ystart = 0;

    if (col % 2 == 1) {
      ystart = half;
    }

    for (uint32 y = ystart; y < h; y += size) {
      diamondStep(hmap, x % w, y % h, half);
    }
  }

  hmap_generate_step(hmap, half);
}

static void hmap_relativeize(heightmap hmap) {
  float sample = 0.0f;

  float greatest = hmap->greatestValue;
  float smallest = hmap->smallestValue;

  float range = greatest - smallest;

  for (uint32 x = 0; x < hmap->width; x++) {
    for (uint32 y = 0; y < hmap->height; y++) {
      sample = hmap_getsample(hmap, x, y);
      sample -= smallest;
      sample /= range;

      hmap_setsample(hmap, x, y, sample);
    }
  }
}

void hmap_generate(heightmap hmap) {
  if (!hmap) {
    return;
  }
  
  uint32 size = hmap->width / 2;
  hmap_generate_step(hmap, size);

  printf("greatestValue=%f\n", hmap->greatestValue);

  // hmap_round(hmap);
  hmap_relativeize(hmap);
}
