#include "common.h"
#include <stdlib.h>

typedef struct {
  uint8 r;
  uint8 g;
  uint8 b;
} color24;

typedef struct {
  uint32 length;
  color24* data;
} color_array_struct;

typedef color_array_struct* color_array;

#define color(r, g, b) ((color24) {r, g, b})
#define BLACK ((color24) {0, 0, 0})

color24 color_array_get(color_array arr, uint32 idx) {
  if (!arr) {
    return BLACK;
  }
  if (idx < 0 || idx >= arr->length) {
    return BLACK;
  }

  return arr->data[idx];
}

void color_array_set(color_array arr, uint32 idx, color24 ncolor) {
  if (!arr) {
    return;
  }
  if (idx < 0 || idx >= arr->length) {
    return;
  }

  arr->data[idx] = ncolor;
}

void setc(color24* out, uint8 v) {
  if (!out) {
    return;
  }

  out->r = v;
  out->g = v;
  out->b = v;
}

void pickcolor(color24* out, color_array arr, float delta) {
  if (delta < 0.0f || delta >= 1.0f || !arr || !out) {
    return;
  }

  uint32 idx = delta * arr->length;
  color24 c = color_array_get(arr, idx);

  *out = c;
}

void lerpcolor(color24* out, color_array arr, float delta) {
  if (delta < 0.0f || delta > 1.0f || !arr || !out) {
    return;
  }

  uint32 maxi = arr->length - 1;
  uint32 fidx = delta * maxi;

  float firststep = (float) fidx / maxi;
  float localstep = (delta - firststep) * maxi;

  color24 c1 = color_array_get(arr, fidx);
  color24 c2 = color_array_get(arr, fidx + 1);

  out->r = c1.r + (localstep * (c2.r - c1.r));
  out->g = c1.g + (localstep * (c2.g - c1.g));
  out->b = c1.b + (localstep * (c2.b - c1.b));
}

color_array colors_malloc(uint32 len) {
  uint64 memlen = len;
  memlen *= sizeof(color24);

  color24* ptr = (color24*) malloc(memlen);
  if (!ptr) {
    return NULL;
  }

  color_array arr = (color_array) malloc(sizeof(color_array_struct));
  if (!arr) {
    return NULL;
  }

  arr->length = len;
  arr->data = ptr;

  return arr;
}