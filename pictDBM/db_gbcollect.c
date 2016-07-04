/**
 * @file do_gbcollect.c
 * @brief A garbage collector, removing invalid images on the disk.
 *
 * @date 23 May 2016
 */
#include "pictDB.h"
#include "image_content.h"
#include <stdlib.h>

/**
* @brief Free the content of a pointer
*
* @param char** pointer to the image
*/
void free_picture(char** data)
{
    if(data != NULL) {
        if(*data != NULL) {
            free(*data);
            *data = NULL;
        }
    }
}

/**
* @brief Initialize the db_temp structure.
*
* @param db_file A pointer to a pictdb_file
* @param db_temp A pointer to a pictdb_file
*/
void initialize_pictdb(struct pictdb_file* db_file, struct pictdb_file* db_temp)
{
    db_temp->header.res_resized[RES_THUMB] = db_file->header.res_resized[RES_THUMB];
    db_temp->header.res_resized[RES_THUMB+1] = db_file->header.res_resized[RES_THUMB+1] ;
    db_temp->header.res_resized[2*RES_SMALL] = db_file->header.res_resized[2*RES_SMALL];
    db_temp->header.res_resized[2*RES_SMALL+1] = db_file->header.res_resized[2*RES_SMALL+1];
    db_temp->header.max_files = db_file->header.max_files;
}

/**
* @brief Remove the file and rename the other one with the name of
* the removed file.
*
* @param filename Name of the file to remove
* @param temp_filename Name of the file to rename
*
* @return 0 or an error code if an error occured
*/
int remove_and_rename(const char* filename, const char* temp_filename)
{
    int check = remove(filename);
    if(check != 0) {
        return ERR_IO;
    }
    check = rename(temp_filename, filename);
    if(check != 0) {
        return ERR_IO;
    }
    return 0;
}
/**
* @brief A garbage collection of the pictdb_file given as parameter, by removing
* every invalid image.
*
* @param db_file A pointer to a pictdb_file
* @param filename Name of the file to remove
* @param temp_filename Name of the file to rename
*
* @return 0 or an error code if an error occured
*/
int do_gbcollect(struct pictdb_file* db_file, const char* filename, const char* temp_filename)
{
    if(db_file == NULL || filename == NULL || temp_filename == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    struct pictdb_file db_temp;
    //Initalize and create db_temp
    initialize_pictdb(db_file, &db_temp);
    int check = do_create(temp_filename, &db_temp);
    if(check != 0) {
        do_close(&db_temp);
        return check;
    }
    for(int i=0; i < db_file->header.max_files; i++) {
        if(db_file->metadata[i].is_valid == NON_EMPTY) {
            //If the metadata is valid we insert it in db_temp.
            char* picture;
            uint32_t pict_size = 0;
            //Read the image from db_file.
            check = do_read(db_file->metadata[i].pict_id, RES_ORIG, &picture, &pict_size, db_file);
            if(check != 0) {
                do_close(&db_temp);
                free_picture(&picture);
                remove(temp_filename); //In case of an error we remove db_temp
                return check;
            }
            //Insert the image in db_temp.
            check = do_insert(picture, pict_size, db_file->metadata[i].pict_id, &db_temp);
            if(check != 0) {
                do_close(&db_temp);
                free_picture(&picture);
                remove(temp_filename);
                return check;
            }
            free_picture(&picture);
            //If the metadata has the small image, we add it too in db_temp.
            if(db_file->metadata[i].offset[RES_SMALL] != 0) {
                check = do_read(db_file->metadata[i].pict_id, RES_SMALL, &picture, &pict_size, &db_temp);
                if(check != 0) {
                    do_close(&db_temp);
                    free_picture(&picture);
                    remove(temp_filename);
                    return check;
                }
                free_picture(&picture);
            }
            //If the metadata has the thumbnail, we add it too in db_temp.
            if(db_file->metadata[i].offset[RES_THUMB] != 0) {
                check = do_read(db_file->metadata[i].pict_id, RES_THUMB, &picture, &pict_size, &db_temp);
                if(check != 0) {
                    do_close(&db_temp);
                    free_picture(&picture);
                    remove(temp_filename);
                    return check;
                }
                free_picture(&picture);
            }
        }
    }
    //Remove db_file and rename db_temp
    check = remove_and_rename(filename, temp_filename);
    do_close(&db_temp);
    if(check != 0) {
        return check;
    }
    return 0;
}