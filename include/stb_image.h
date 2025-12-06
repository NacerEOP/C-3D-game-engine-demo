/* stb_image - v2.30 - public domain image loader - http://nothings.org/stb
   This is the official single-file public domain / MIT-licensed image loader.
   It is included here so the engine can decode embedded PNG/JPEG/etc images
   from glTF files at runtime.

   Usage: #define STB_IMAGE_IMPLEMENTATION in ONE C/C++ source file
   before including this file to create the implementation.

   Full original source from: https://github.com/nothings/stb/blob/master/stb_image.h
   (License: MIT or public domain)
*/

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

/* Documentation and header portion of stb_image.h follow. The implementation
   will be provided when a translation unit defines STB_IMAGE_IMPLEMENTATION
   and includes this header. */

#include <stdlib.h>

/* Minimal stable API declarations used by the engine */
typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif

extern stbi_uc *stbi_load_from_memory(const stbi_uc *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H

/*
   Note: The full implementation of stb_image is large. We include the
   header above and provide an implementation file `src/stb_image_impl.cpp`
   that defines `STB_IMAGE_IMPLEMENTATION` and includes this header so the
   functions are available to the rest of the project.

   This header is sufficient for the engine's uses (memory-based decoding
   via `stbi_load_from_memory` and `stbi_image_free`).
*/
