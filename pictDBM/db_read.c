/**
 * @file db_read.c
 *
 * @brief
 * @date 29 April 2016
 */
#include "pictDB.h"
#include "image_content.h" //for lazily_resize
#include <string.h>
#include <stdlib.h>

/**
* @brief Function that read an image and copies it in a "table" of bytes.
*
* @param pict_id String of char identifying the image.
* @param res Code of an image resolution.
* @param data Address of a "table" of char (used as bytes).
* @param pict_size Image size.
* @param db_file Data base.
*
* @return 0 or an error code if an error occurs.
*/
int do_read(const char* pict_id, const int res, char** data, uint32_t* pict_size, struct pictdb_file* db_file)
{
    if(pict_id == NULL || db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    int check = 0;
    size_t index = 0;   //Position of the image to delete
    while(strcmp(pict_id, db_file->metadata[index].pict_id) != 0 && index < db_file->header.max_files) {
        index += 1;
        while(db_file->metadata[index].is_valid == EMPTY && index < db_file->header.max_files) { //Skip the non valid metadatas
            index += 1;
        }
    }

    //If index == db_file->header.max_files, the image is not in the metadatas
    if(index >= db_file->header.max_files) {
        return ERR_FILE_NOT_FOUND;
    }

    if(db_file->metadata[index].offset[res] == 0 || db_file->metadata[index].size[res] == 0) {
        check = lazily_resize(res, db_file, index);
        if(check != 0) {
            return check;
        }
    }

    if(fseek(db_file->fpdb, db_file->metadata[index].offset[res], SEEK_SET) != 0) {
        return ERR_IO;
    }

    //Taille de l'image connue avec lazily_resize
    *pict_size = db_file->metadata[index].size[res];

    char* p = malloc(*pict_size);
    if(p == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    if(fread(p, *pict_size, 1, db_file->fpdb) != 1) {
        return ERR_IO;
    }
    *data = p;

    return 0;
}