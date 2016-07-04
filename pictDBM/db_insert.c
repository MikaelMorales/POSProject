/**
 * @file db_insert.c
 *
 * @brief
 * @date 29 April 2016
 */

#include "pictDB.h"
#include "image_content.h"
#include "dedup.h"
#include <string.h>

/**
* @brief Function that get the image resolution and write it in memory.
*
* @param img "table" of character (used as bytes).
* @param size The image size.
* @param db_file Data base in which we add the image.
* @param index Position of the image.
*
* @return 0 or an error code if an error occurs.
*/
int write_resolutions(const char* img, const size_t size, struct pictdb_file* db_file, size_t index)
{
    uint32_t width;
    uint32_t height;
    int return_value = get_resolution(&height, &width, img, size);
    if(return_value != 0) {
        return return_value;
    }
    db_file->metadata[index].res_orig[0] = width;
    db_file->metadata[index].res_orig[1] = height;

    return 0;
}

/**
* @brief Function that update the memory when we insert an image.
*
* @param img "table" of character (used as bytes).
* @param size The image size.
* @param db_file Data base in which we add the image.
* @param index Position of the image.
*
* @return 0 or an error code if an error occurs.
*/
int update_memory_and_content(const char* img, const size_t size, struct pictdb_file* db_file, size_t index)
{
    int check = 0;

    //We write the image at the end of the file iff it was not already there.
    if(db_file->metadata[index].offset[RES_ORIG] == 0) {
        //Cursor at the end of the file
        if(fseek(db_file->fpdb, 0, SEEK_END) != 0) {
            return ERR_IO;
        }
        //Get the cursor position before writing
        long int cursorPosition = ftell(db_file->fpdb);
        //Write the image at the end of the file.
        if(fwrite(img, size, 1, db_file->fpdb) != 1) {
            return ERR_IO;
        }
        //Update the metadata
        db_file->metadata[index].offset[RES_ORIG] = cursorPosition;
    }

    //Write the resolutions in the new metadata
    check = write_resolutions(img, size, db_file, index);
    if (check != 0) {
        return check;
    }

    //Update the header
    db_file->header.num_files += 1;
    db_file->header.db_version += 1;

    //Write the uptaded header on the disk
    if(fseek(db_file->fpdb, 0, SEEK_SET) != 0) {
        return ERR_IO;
    }
    if(fwrite(&db_file->header, sizeof(struct pictdb_header), 1 , db_file->fpdb) != 1) {
        return ERR_IO;
    }

    //Write the updated metadata on disk
    if(fseek(db_file->fpdb, index * sizeof(struct pict_metadata), SEEK_CUR) != 0) {
        return ERR_IO;
    }
    if(fwrite(&db_file->metadata[index], sizeof(struct pict_metadata), 1 , db_file->fpdb) != 1) {
        return ERR_IO;
    }

    return 0;
}

/**
 * @brief Function that inserts an image in a data base.
 *
 * @param img "table" of character (used as bytes).
 * @param size The image size.
 * @param pict_id String of char identifying the image.
 * @param db_file Data base in which we add the image.
 *
 * @return 0 or an error code if an error occurs.
 */
int do_insert(const char* img, const size_t size, const char* pict_id, struct pictdb_file* db_file)
{
    if(img == NULL || pict_id == NULL || db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if(db_file->header.num_files == db_file->header.max_files)  {
        return ERR_FULL_DATABASE;
    }

    //check that this pict_id does not already exist
    for(size_t i = 0; i < db_file->header.max_files; i++) {
        if(db_file->metadata[i].is_valid == NON_EMPTY) {
            if(!strncmp(db_file->metadata[i].pict_id, pict_id, MAX_PIC_ID)) {
                return ERR_DUPLICATE_ID;
            }
        }
    }

    size_t index = 0;

    while(index < db_file->header.max_files && db_file->metadata[index].is_valid == NON_EMPTY) {
        index += 1;
    }
    (void)SHA256((unsigned char *)img, size, db_file->metadata[index].SHA);
    if(strncpy(db_file->metadata[index].pict_id, pict_id, MAX_PIC_ID) == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    db_file->metadata[index].size[RES_ORIG] = size;
    db_file->metadata[index].size[RES_THUMB] = 0;
    db_file->metadata[index].size[RES_SMALL] = 0;
    db_file->metadata[index].offset[RES_THUMB] = 0;
    db_file->metadata[index].offset[RES_SMALL] = 0;
    db_file->metadata[index].is_valid = NON_EMPTY;

    int check = 0;
    check = do_name_and_content_dedup(db_file, index);
    if(check != 0) {
        return check;
    }

    check = update_memory_and_content(img, size, db_file, index);
    if(check != 0) {
        return check;
    }

    return 0;
}

