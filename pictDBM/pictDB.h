/**
 * @file pictDB.h
 * @brief Main header file for pictDB core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The picture database starts with exactly one header structure
 * followed by exactly pictdb_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * database file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 */

#ifndef PICTDBPRJ_PICTDB_H
#define PICTDBPRJ_PICTDB_H

#include "error.h" /* not needed here, but provides it as required by
                    * all functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define CAT_TXT "EPFL PictDB binary"

/* constraints */
#define MAX_DB_NAME 31  // max. size of a PictDB name
#define MAX_PIC_ID 127  // max. size of a picture id
#define MAX_MAX_FILES 100000  // will be increased later in the project

/* For is_valid in pictdb_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// pictDB library internal codes for different picture resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief The header of an image, contains the configuration settings.
*/
struct pictdb_header {
    char db_name[MAX_DB_NAME+1];
    uint32_t db_version;
    uint32_t num_files;
    uint32_t max_files;
    uint16_t res_resized[2*(NB_RES-1)];
    uint32_t unused_32;
    uint64_t unused_64;
};

/**
* @brief Metadata of an image.
*/
struct pict_metadata {
    char pict_id[MAX_PIC_ID+1];
    unsigned char SHA[SHA256_DIGEST_LENGTH];
    uint32_t res_orig[2];
    uint32_t size[NB_RES];
    uint64_t offset[NB_RES];
    uint16_t is_valid;
    uint16_t unused_16;
};
/**
* @brief Describe a picture with the file, metadata and the header.
*/
struct pictdb_file {
    FILE* fpdb;
    struct pictdb_header header;
    struct pict_metadata* metadata;
};

/**
* @brief Enum determining the format to list
*/
enum do_list_mode{
    STDOUT, JSON
};

/**
 * @brief Prints database header informations.
 *
 * @param header The header to be displayed.
 */
void print_header (const struct pictdb_header* header);

/**
 * @brief Prints picture metadata informations.
 *
 * @param metadata The metadata of one picture.
 */
void print_metadata (const struct pict_metadata* metadata);

/**
 * @brief Displays pictDB metadata.
 * @brief format in which we return the output.
 *
 * @param db_file In memory structure with header and metadata.
 *
 * @return char* content of the pictdb_file
 */
const char* do_list (const struct pictdb_file* file, enum do_list_mode format);

/**
 * @brief Creates the database called db_filename. Writes the header and the
 *        preallocated empty metadata array to database file.
 *
 * @param db_file In memory structure with header and metadata.
 */
int do_create(const char* file_name, struct pictdb_file* db_file);

/**
 * @brief Open the file, read the header and metadata and write them
 *       in the picdb_file struct.
 *
 * @param  const char* : the file name
 * @param  const char* : opening mode
 * @param  const struct picdb_file* db_file : struct where we store the read data
 *
 * @return 0 if opened correctly, an error otherwise
 */
int do_open(const char* file_name, const char* opening_mode, struct pictdb_file* db_file);


/**
 * @brief close the file contained in the struct picdb_file.
 *
 * @param const struct picdb_file* db_file
 */
void do_close(struct pictdb_file* db_file);

/**
 *
 * @brief delete a picture from a pictdb_file.
 *
 * @param const char* picture_name
 * @param struct pictdb_file* db_file
 *
 * @return 0 if deleted correctly, error otherwise.
 */
int do_delete(const char* picture_name, struct pictdb_file* db_file);

/**
 * @brief Function that get the resolution code associated with the resolution name.
 *
 * @param resolution The resolution name.
 *
 * @return The resolution code.
 */
int resolution_atoi(const char* resolution);

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
int do_read(const char* pict_id, const int res, char** data, uint32_t* pict_size, struct pictdb_file* db_file);

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
int do_insert(const char* img, const size_t size, const char* pict_id, struct pictdb_file* db_file);

/**
*
*
*
*
*/
int do_gbcollect(struct pictdb_file* db_file, const char* filename, const char* temp_filename);



#ifdef __cplusplus
}
#endif
#endif
