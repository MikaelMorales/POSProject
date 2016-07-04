/**
 * @file db_delete.c
 * @brief Delete an image in a file
 *
 * @date 17 April 2016
 */

#include <string.h> // for strcmp
#include <stdio.h> // for sprintf
#include "pictDB.h"

/**
 * @brief Delete an image in the file
 *
 * @param pictdb_file* A pointer to a structure in memory with header and metadata.
 * @param const char* picture_name the picture we want to delete
 *
 * @return 0 if no errors, otherwise an error.
 */
int do_delete(const char* picture_name, struct pictdb_file* db_file)
{
    if(db_file->header.num_files == 0) {
        return ERR_IO;
    }
    unsigned int index = 0;   //Position of the image to delete
    while(index < db_file->header.max_files && strcmp(picture_name, db_file->metadata[index].pict_id) != 0) {
        index += 1;
        while(index < db_file->header.max_files && db_file->metadata[index].is_valid == EMPTY) { //Skip the non valid metadatas
            index += 1;
        }
    }
    //If index == db_file->header.max_files, the image is not in the metadatas
    if(index >= db_file->header.max_files) {
        return ERR_FILE_NOT_FOUND;
    }

    //Position of the image in the file
    size_t pict_position = sizeof(struct pictdb_header) + index * sizeof(struct pict_metadata);
    db_file->metadata[index].is_valid = EMPTY;
    int return_value_fseek = fseek(db_file->fpdb, pict_position, SEEK_SET);
    size_t return_value_fwrite = 0;

    if(return_value_fseek == 0) {
        //Rewrite the whole metadata
        return_value_fwrite = fwrite(&db_file->metadata[index], sizeof(struct pict_metadata), 1, db_file->fpdb);
        if(return_value_fwrite == 1) {
            //Update the header
            db_file->header.db_version += 1;
            db_file->header.num_files -= 1;
            return_value_fseek = fseek(db_file->fpdb, 0, SEEK_SET);
            return_value_fwrite = fwrite(&db_file->header, sizeof(struct pictdb_header), 1, db_file->fpdb);
        }
    }
    if(return_value_fseek != 0 || return_value_fwrite != 1) {
        return ERR_IO;
    }
    return 0;
}