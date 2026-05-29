#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiff.h>
#include <tiffio.h>

#ifndef thread_local
#ifdef _WIN32
#define thread_local __declspec(thread)
#else
#define thread_local _Thread_local
#endif
#endif

typedef struct {
  const uint8_t *data;
  size_t len;
  size_t offset;
} bare_tiff_reader_t;

typedef struct {
  uint8_t *data;
  size_t len;
  size_t capacity;
} bare_tiff_writer_t;

static thread_local char bare_tiff__last_error[512];

static void
bare_tiff__on_error(const char *module, const char *fmt, va_list ap) {
  vsnprintf(bare_tiff__last_error, sizeof(bare_tiff__last_error), fmt, ap);
}

static void
bare_tiff__on_finalize(js_env_t *env, void *data, void *finalize_hint) {
  free(data);
}

static tsize_t
bare_tiff__on_read(thandle_t handle, tdata_t data, tsize_t len) {
  bare_tiff_reader_t *reader = (bare_tiff_reader_t *) handle;

  if (reader->offset + len > reader->len) {
    len = reader->len - reader->offset;
  }

  memcpy(data, reader->data + reader->offset, len);

  reader->offset += len;

  return len;
}

static tsize_t
bare_tiff__on_write(thandle_t handle, tdata_t data, tsize_t len) {
  bare_tiff_writer_t *writer = (bare_tiff_writer_t *) handle;

  if (writer->len + len > writer->capacity) {
    size_t capacity = (writer->len + len) * 2;

    uint8_t *resized = realloc(writer->data, capacity);

    if (resized == NULL) return 0;

    writer->data = resized;
    writer->capacity = capacity;
  }

  memcpy(writer->data + writer->len, data, len);

  writer->len += len;

  return len;
}

static toff_t
bare_tiff__on_seek(thandle_t handle, toff_t offset, int whence) {
  bare_tiff_reader_t *reader = (bare_tiff_reader_t *) handle;

  switch (whence) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    offset = reader->offset + offset;
    break;
  case SEEK_END:
    offset = reader->len + offset;
    break;
  default:
    return (toff_t) -1;
  }

  if (offset > reader->len) return (toff_t) -1;

  reader->offset = offset;

  return reader->offset;
}

static int
bare_tiff__on_close(thandle_t handle) {
  return 0;
}

static toff_t
bare_tiff__on_size(thandle_t handle) {
  bare_tiff_reader_t *reader = (bare_tiff_reader_t *) handle;

  return reader->len;
}

static js_value_t *
bare_tiff_decode(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  uint8_t *tiff;
  size_t len;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &tiff, &len, NULL, NULL);
  assert(err == 0);

  bare_tiff_reader_t reader = {tiff, len, 0};

  TIFFSetErrorHandler(bare_tiff__on_error);

  TIFF *decoder = TIFFClientOpen("mem", "r", &reader, bare_tiff__on_read, bare_tiff__on_write, bare_tiff__on_seek, bare_tiff__on_close, bare_tiff__on_size, NULL, NULL);

  if (decoder == NULL) {
    err = js_throw_error(env, NULL, bare_tiff__last_error);
    assert(err == 0);

    return NULL;
  }

  uint32_t width, height;

  if (!TIFFGetField(decoder, TIFFTAG_IMAGEWIDTH, &width) || !TIFFGetField(decoder, TIFFTAG_IMAGELENGTH, &height)) {
    err = js_throw_error(env, NULL, "Missing image dimensions");
    assert(err == 0);

    TIFFClose(decoder);

    return NULL;
  }

  if (width == 0 || height == 0 || width > SIZE_MAX / 4 / height) {
    err = js_throw_error(env, NULL, "Invalid image dimensions");
    assert(err == 0);

    TIFFClose(decoder);

    return NULL;
  }

  size_t size = (size_t) width * height * 4;

  uint32_t *data = malloc(size);

  if (data == NULL) {
    err = js_throw_error(env, NULL, "Out of memory");
    assert(err == 0);

    TIFFClose(decoder);

    return NULL;
  }

  if (!TIFFReadRGBAImageOriented(decoder, width, height, data, ORIENTATION_TOPLEFT, 0)) {
    err = js_throw_error(env, NULL, bare_tiff__last_error);
    assert(err == 0);

    TIFFClose(decoder);

    free(data);

    return NULL;
  }

  TIFFClose(decoder);

  js_value_t *result;
  err = js_create_object(env, &result);
  assert(err == 0);

#define V(n) \
  { \
    js_value_t *val; \
    err = js_create_uint32(env, n, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, result, #n, val); \
    assert(err == 0); \
  }

  V(width);
  V(height);
#undef V

  js_value_t *buffer;
  err = js_create_external_arraybuffer(env, data, size, bare_tiff__on_finalize, NULL, &buffer);
  assert(err == 0);

  err = js_set_named_property(env, result, "data", buffer);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_tiff_encode(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 3);

  uint8_t *data;
  size_t data_len;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &data, &data_len, NULL, NULL);
  assert(err == 0);

  int64_t width;
  err = js_get_value_int64(env, argv[1], &width);
  assert(err == 0);

  int64_t height;
  err = js_get_value_int64(env, argv[2], &height);
  assert(err == 0);

  if (width <= 0 || height <= 0 || width > UINT32_MAX || height > UINT32_MAX) {
    err = js_throw_error(env, NULL, "invalid image dimensions");
    assert(err == 0);

    return NULL;
  }

  if ((uint64_t) width > SIZE_MAX / 4 / (uint64_t) height) {
    err = js_throw_error(env, NULL, "invalid image dimensions");
    assert(err == 0);

    return NULL;
  }

  size_t expected_len = (size_t) width * (size_t) height * 4;

  if (data_len < expected_len) {
    err = js_throw_error(env, NULL, "buffer too small for dimensions");
    assert(err == 0);

    return NULL;
  }

  bare_tiff_writer_t writer = {NULL, 0, 0};

  TIFFSetErrorHandler(bare_tiff__on_error);

  TIFF *encoder = TIFFClientOpen("mem", "w", &writer, bare_tiff__on_read, bare_tiff__on_write, bare_tiff__on_seek, bare_tiff__on_close, bare_tiff__on_size, NULL, NULL);

  if (encoder == NULL) {
    err = js_throw_error(env, NULL, bare_tiff__last_error);
    assert(err == 0);

    return NULL;
  }

  TIFFSetField(encoder, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(encoder, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(encoder, TIFFTAG_SAMPLESPERPIXEL, 4);
  TIFFSetField(encoder, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(encoder, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(encoder, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(encoder, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

  for (uint32_t row = 0; row < (uint32_t) height; row++) {
    err = TIFFWriteScanline(encoder, data + ((size_t) row * (size_t) width * 4), row, 0);

    if (err < 0) {
      err = js_throw_error(env, NULL, bare_tiff__last_error);
      assert(err == 0);

      TIFFClose(encoder);

      free(writer.data);

      return NULL;
    }
  }

  TIFFClose(encoder);

  js_value_t *result;
  err = js_create_external_arraybuffer(env, writer.data, writer.len, bare_tiff__on_finalize, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_tiff_exports(js_env_t *env, js_value_t *exports) {
  int err;

#define V(name, fn) \
  { \
    js_value_t *val; \
    err = js_create_function(env, name, -1, fn, NULL, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("decode", bare_tiff_decode)
  V("encode", bare_tiff_encode)
#undef V

  return exports;
}

BARE_MODULE(bare_tiff, bare_tiff_exports)
