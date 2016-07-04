/**
 * @file image_content.c
 * @brief Resize an existing image.
 *
 * @date 29 April 2016
 */

#ifndef IMAGE_CONTENT_H
#define IMAGE_CONTENT_H

#include "pictDB.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Resize an existing image.
* @param res Index of the new resolution.
* @param db_file Pointer to a pictdb_file structure.
* @param index Postion of the existing image in the metadata.
* @return 0 if successfull otherwise an error.
*/
int lazily_resize(const int res, struct pictdb_file* db_file, size_t index);



/**
* @brief Function that retrieve the resolution of a JPEG image.
*
* @param height Pointer which store the address of the height.
* @param width Pointer which store the address of the width.
* @param image_buffer Pointer to the memory region where the JPEG image is stored.
* @param image_size size of the memory region where the JPEG image is stored.
*
* @return 0 or ERR_VIPS if an error occurs.
*/
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size);

#ifdef __cplusplus
}
#endif
#endif
