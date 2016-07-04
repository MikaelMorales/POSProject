/**
 * @file image_content.c
 * @brief Resize an existing image.
 *
 * @date 29 April 2016
 */

#include "pictDB.h"
#include <vips/vips.h>
#include <stdlib.h>

/**
* @brief Compute the ratio between the old resolution and the new one.
* @param image Pointer to a VipsImage.
* @param max_newres_width New width resolution.
* @param max_newres_height New height resolution.
* @return
*/
double shrink_value(VipsImage *image,
                    int max_newres_width,
                    int max_newres_height)
{
    const double h_shrink = (double) max_newres_width / (double) image->Xsize ;
    const double v_shrink = (double) max_newres_height / (double) image->Ysize ;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

/**
* @brief create a new image from an existing one but in a different resolution.
* @param res Index of the new resolution.
* @param db_file Pointer to a pictdb_file structure.
* @param index Postion of the existing image in the metadata.
* @return O if no error occurs, otherwise an error.
*/
int create_image(const int res, struct pictdb_file* db_file, size_t index)
{
    int return_value_fseek = fseek(db_file->fpdb, db_file->metadata[index].offset[RES_ORIG], SEEK_SET);
    if(return_value_fseek != 0) {
        return ERR_IO;
    }

    size_t len = db_file->metadata[index].size[RES_ORIG];
    char* content = malloc(len);
    if(content == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    int check = fread(content, len, 1, db_file->fpdb);
    if(check != 1) {
        return ERR_IO;
    }

    //Used to load
    VipsImage* original;

    //some place to do the job
    VipsObject* process = VIPS_OBJECT( vips_image_new() );

    //we want 1 new images
    VipsImage** thumbs = (VipsImage**) vips_object_local_array(process, 1 );

    //Load the image
    if(vips_jpegload_buffer(content, len, &original, NULL) ) {
        return ERR_VIPS;
    }

    //Compute the resizing ratio.
    double ratio = shrink_value(original, db_file->header.res_resized[2*res], db_file->header.res_resized[2*res + 1]);

    //Check VIPS_Version
#if VIPS_MAJOR_VERSION > 7 || (VIPS_MAJOR_VERSION == 7 && MINOR_VERSION > 40)
    check = vips_resize(original, &thumbs[0], ratio, NULL);
    if(check != 0) {
        return ERR_VIPS;
    }
#else
    if (ratio < 1.0) {
        ratio = (int) (1./ratio) + 1.0;
        check = vips_shrink(original, &thumbs[0], ratio, ratio, NULL);
        if(check != 0) {
            return ERR_VIPS;
        }
    }
#endif

    char* newContent;

    //Save the resized image.
    if(vips_jpegsave_buffer(thumbs[0], (void**)&newContent, &len, NULL)) {
        return ERR_VIPS;
    }

    //write to the end of the file.
    return_value_fseek = fseek(db_file->fpdb, 0, SEEK_END);
    if(return_value_fseek != 0) {
        return ERR_IO;
    }
    long int cursorPosition = ftell(db_file->fpdb);

    check = fwrite(newContent, len, 1, db_file->fpdb);
    if(check != 1) {
        return ERR_IO;
    }

    free(content);
    content = NULL;
    g_free(newContent);
    g_object_unref( process );
    //Update image in memory
    db_file->metadata[index].offset[res] = cursorPosition;
    db_file->metadata[index].size[res] = len;

    //Update the metadata in file
    return_value_fseek = fseek(db_file->fpdb, sizeof(struct pictdb_header) + index * sizeof(struct pict_metadata), SEEK_SET);
    if(return_value_fseek != 0) {
        return ERR_IO;
    }
    check = fwrite(&db_file->metadata[index], sizeof(struct pict_metadata), 1, db_file->fpdb);
    if(check != 1) {
        return ERR_IO;
    }

    return 0;
}

/**
* @brief Resize an existing image.
* @param res Index of the new resolution.
* @param db_file Pointer to a pictdb_file structure.
* @param index Postion of the existing image in the metadata.
* @return 0 if successfull otherwise an error.
*/
int lazily_resize(const int res, struct pictdb_file* db_file, size_t index)
{

    if(res == RES_ORIG) {
        return 0;
    }

    if(res != RES_SMALL && res != RES_THUMB) {
        return ERR_INVALID_ARGUMENT;
    }

    if(db_file == NULL) {
        return ERR_FILE_NOT_FOUND;
    }

    if(index >= db_file->header.max_files) {
        return ERR_INVALID_ARGUMENT;
    }

    //if the memory size of the image is not 0 or offset is not 0
    if(db_file->metadata[index].size[res] != 0 || db_file->metadata[index].offset[res] != 0) {
        return 0;
    }

    //0 if no error occured !
    return create_image(res, db_file, index);
}

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
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size)
{
    if(image_buffer == NULL || height == NULL || width == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    //Used to load
    VipsImage* original;

    //Load the image
    if(vips_jpegload_buffer((void*)image_buffer, image_size, &original, NULL) ) {
        return ERR_VIPS;
    }

    int w = vips_image_get_width(original);
    int h = vips_image_get_height(original);

    *height = h;
    *width = w;

    return 0;
}



