/**
 * @file db_create.c
 * @brief pictDB library: do_create implementation.
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 */

#include "pictDB.h"
#include <string.h> // for strncpy
#include <stdlib.h>


/********************************************************************//**
 * Creates the database called db_filename. Writes the header and the
 * preallocated empty metadata array to database file.
 */
int do_create(const char* file_name, struct pictdb_file* db_file)
{
    size_t number_header = 0;
    size_t number_metadata = 0;
    // Sets the DB header name
    strncpy(db_file->header.db_name, CAT_TXT,  MAX_DB_NAME);
    db_file->header.db_name[MAX_DB_NAME] = '\0';
    //Initialisation
    db_file->header.db_version = 0;
    db_file->header.num_files = 0;

    //Memory allocation
    db_file->metadata = calloc(db_file->header.max_files, sizeof(struct pict_metadata));
    if(db_file->metadata == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    for(int i = 0; i < db_file->header.max_files; i++) {
        db_file->metadata[i].is_valid = 0;
    }

    db_file->fpdb = NULL;
    db_file->fpdb = fopen(file_name, "w+b");

    if(db_file->fpdb != NULL) {
        number_header = fwrite(&db_file->header, sizeof(struct pictdb_header), 1, db_file->fpdb);
        number_metadata = fwrite(db_file->metadata, sizeof(struct pict_metadata), db_file->header.max_files, db_file->fpdb);
    }

    if(number_header != 1 || number_metadata != db_file->header.max_files || db_file->fpdb == NULL) {
        free(db_file->metadata);
        db_file->metadata = NULL;
        return ERR_IO;
    }

    printf("%zu item(s) written\n", number_header+number_metadata);
    return 0;
}
