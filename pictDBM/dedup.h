/**
 * @file dedup.c
 * @brief Deduplicate two image with the same content.
 *
 * @date 29 April 2016
 */
#ifndef DEDUP_H
#define DEDUP_H 

#include "pictDB.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Compare two values of SHA-hash.
* @param a the first SHA-hash.
* @param b the second SHA-hash.
* @return 0 if they are identical, 1 otherwise.
*/
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index);

#ifdef __cplusplus
}
#endif
#endif
