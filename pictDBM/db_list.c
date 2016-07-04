/**
 * @file db_list.c
 * @brief Print the content of the the file.
 * Print first the header and then the metadata of the image if possible.
 *
 * @date 13 March 2016
 */

#include "pictDB.h"
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>

/**
 * @brief Displays pictDB metadata.
 * @brief format in which we return the output.
 *
 * @param db_file In memory structure with header and metadata.
 *
 * @return char* content of the pictdb_file
 */
const char* do_list (const struct pictdb_file* file, enum do_list_mode format)
{
    if(format == STDOUT) {
        print_header(&file->header);
        if(file->header.num_files != 0) {
            for(size_t i = 0; i < file->header.max_files ; i++) {
                if(file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&file->metadata[i]);
                }
            }
        } else {
            printf("<< empty database >>\n");
        }
        return NULL;
    } else if(format == JSON) {
        struct json_object* object = json_object_new_object();
        struct json_object* array = json_object_new_array();

        for(size_t i = 0; i < file->header.max_files ; i++) {
            if(file->metadata[i].is_valid != EMPTY) {
                struct json_object* string =  json_object_new_string(file->metadata[i].pict_id);
                json_object_array_add(array, string);
            }
        }
        json_object_object_add(object, "Pictures", array);
        const char* string = json_object_to_json_string(object);
        char* result = calloc(strlen(string), sizeof(char));
        strncpy(result,string,strlen(string));
        //Free the json_object
        json_object_put(object);
        return result;
    } else {
        return "unimplemented do_list mode";
    }
}