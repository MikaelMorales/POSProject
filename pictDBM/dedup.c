/**
 * @file dedup.c
 * @brief Deduplicate two image with the same content.
 *
 * @date 29 April 2016
 */

#include "pictDB.h"
#include <string.h>


/**
* @brief Compare two values of SHA-hash.
* @param a the first SHA-hash.
* @param b the second SHA-hash.
* @return 0 if they are identical, 1 otherwise.
*/
int compare_SHA(unsigned char a[SHA256_DIGEST_LENGTH], unsigned char b[SHA256_DIGEST_LENGTH])
{
    for(size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        if(a[i] != b[i]) {
            return 1;
        }
    }
    return 0;
}


/**
* @brief De-deplicate two images with the same content.
* @param db_file Pointer to a pictdb_file structure.
* @param index Index of the given image.
* @return 0 if we made a de-deplication or if no duplicata, else, an error code.
*/
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index)
{
    if(db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    if(index >= db_file->header.max_files && db_file->metadata[index].is_valid == EMPTY) {
        return ERR_INVALID_ARGUMENT;
    }

    for(size_t i = 0; i < db_file->header.max_files; i++) {
        if(i != index && db_file->metadata[i].is_valid == NON_EMPTY) {
            //If the two pict_ID are equals.
            if(!strncmp(db_file->metadata[i].pict_id, db_file->metadata[index].pict_id, MAX_PIC_ID)) {
                return ERR_DUPLICATE_ID;
            }

            //if the two SHA are equals.
            if(compare_SHA(db_file->metadata[i].SHA, db_file->metadata[index].SHA) == 0) {
                for(size_t j = 0; j < NB_RES; j++) {
                    db_file->metadata[index].size[j] = db_file->metadata[i].size[j];
                    db_file->metadata[index].offset[j] = db_file->metadata[i].offset[j];
                }
                return 0;
            }

        }
    }
    //if no duplicata.
    db_file->metadata[index].offset[RES_ORIG] = 0;
    return 0;

}
