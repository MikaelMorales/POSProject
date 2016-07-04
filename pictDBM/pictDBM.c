/**
 * @file pictDBM.c
 * @brief pictDB Manager: command line interpretor for pictDB core commands.
 *
 * Picture Database Management Tool
 *
 * @author Mia Primorac
 * @date 2 Nov 2015
 */

#include "pictDB.h"
#include "image_content.h"
#include "pictDBM_tools.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <vips/vips.h>

/* Some macros that define default values */
#define MAX_NUMBER_FILES 100000
#define MAX_THUMB_RES 128
#define MAX_SMALL_RES 512
#define DEFAULT_NUMBER_FILES 10
#define DEFAULT_THUMB_RES 64
#define DEFAULT_SMALL_RES 256
#define NUMBER_OF_COMMAND 7
#define MAX_FILE_NAME 1024

/**
* @typedef command
*
* @brief pointer on a function.
*/
typedef int (*command)(int args, char *argv[]);

/**
* @struct command_mapping
*
* @brief map the name of the command to the pointer on the function which execute the command.
*/
typedef struct {
    const char* name;
    command call;
} command_mapping;

/********************************************************************//**
 * Prepares and calls do_list command.
********************************************************************** */
int do_list_cmd(int args, char *argv[])
{
    if(args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    int return_value = 0;
    struct pictdb_file myfile;

    const char* filename = argv[1];

    return_value = do_open(filename, "rb", &myfile);

    if(return_value == 0) {
        do_list(&myfile, STDOUT);
    }

    do_close(&myfile);

    return return_value;
}

/**
* @brief assigns max_files with the value passed as parameter.

* @param max_file Pointer to a uint32_t.
* @param arg2 "String" which contains the value of max_file.
* @param max Maximum possible value of max_file.

* @return 1 if an error occurs, 0 otherwise.
*/
size_t initialise_max_files(uint32_t* max_file, char* arg2, uint32_t max)
{
    if(arg2 == NULL) {
        return 1;
    }
    *max_file = atouint32(arg2);
    return (*max_file <= 0 || *max_file > max) ? 1 : 0;
}

/**
* @brief assigns the resolution

* @param resX Pointer to a uint16_t which is the resolution with respect to the X-axis.
* @param resY Pointer to a uint16_t which is the resolution with respect to the Y-axis.
* @param arg1 Value of resX.
* @param arg2 Value of resY.
* @param max Maximum possible value of the resolution.

* @return 0 or 1 if an error occurs.
*/
size_t initialise_res(uint16_t* resX, uint16_t* resY,char* arg1, char* arg2, uint16_t max)
{
    if(arg1 == NULL || arg2 == NULL) {
        return 1;
    }
    *resX = atouint16(arg1);
    *resY = atouint16(arg2);
    return (*resX <= 0 || *resX > max || *resY <= 0 || *resY > max) ? 1 : 0;
}

/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int args, char *argv[])
{

    if(args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* filename = argv[1];

    uint32_t max_files = DEFAULT_NUMBER_FILES;
    uint16_t thumb_resX = DEFAULT_THUMB_RES;
    uint16_t thumb_resY = DEFAULT_THUMB_RES;
    uint16_t small_resX = DEFAULT_SMALL_RES;
    uint16_t small_resY = DEFAULT_SMALL_RES;

    //Local variable to check if the argument weren't false
    size_t check_max_file = 0;
    size_t check_res = 0;
    size_t index = 2;

    //Check the options given during the "create" command line
    while(index < args) {
        //Check if a -max_file option is given
        if(!strcmp(argv[index], "-max_files")) {
            if(index < args - 1) {
                //Give the following argument to initialize max_files
                check_max_file = initialise_max_files(&max_files, argv[index+1], MAX_NUMBER_FILES);
                if(check_max_file != 0) {
                    return ERR_MAX_FILES;
                }
            } else {
                return ERR_MAX_FILES;
            }
            index += 2;
        }
        //Check if a -thumb_res option is given
        else if(!strcmp(argv[index], "-thumb_res")) {
            if(index < args - 2) {
                //Give the two following arguments to initialize the thumbnail resolution
                check_res = initialise_res(&thumb_resX, &thumb_resY, argv[index+1], argv[index+2], MAX_THUMB_RES);
                if(check_res != 0) {
                    return ERR_RESOLUTIONS;
                }
            } else {
                return ERR_RESOLUTIONS;
            }
            index += 3;
        }
        //Check if a -small_res option is given
        else if(!strcmp(argv[index], "-small_res")) {
            if(index < args - 2) {
                //Give the two following arguments to initialize the small resolution
                check_res = initialise_res(&small_resX, &small_resY, argv[index+1], argv[index+2], MAX_SMALL_RES);
                if(check_res != 0) {
                    return ERR_RESOLUTIONS;
                }
            } else {
                return ERR_RESOLUTIONS;
            }
            index += 3;
            //If an invalid option is given
        } else {
            return ERR_INVALID_ARGUMENT;
        }
    }

    puts("Create");

    struct pictdb_file file;

    file.header.res_resized[RES_THUMB] = thumb_resX;
    file.header.res_resized[RES_THUMB+1] = thumb_resY;
    file.header.res_resized[2*RES_SMALL] = small_resX;
    file.header.res_resized[2*RES_SMALL+1] = small_resY;
    file.header.max_files = max_files;
    int return_value;
    return_value = do_create(filename, &file);
    do_close(&file);
    return return_value;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int help(int args, char* argv[])
{
    printf("pictDBM [COMMAND] [ARGUMENTS]\n");
    printf("  help: displays this help.\n");
    printf("  list <dbfilename>: list pictDB content.\n");
    printf("  create <dbfilename>: create a new pictDB.\n");
    printf("      options are:\n");
    printf("          -max_files <MAX_FILES>: maximum number of files.\n");
    printf("                                  default value is %d\n", DEFAULT_NUMBER_FILES);
    printf("                                  maximum value is %d\n", MAX_NUMBER_FILES);
    printf("          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n");
    printf("                                  default value is %dx%d\n", DEFAULT_THUMB_RES, DEFAULT_THUMB_RES);
    printf("                                  maximum value is %dx%d\n", MAX_THUMB_RES, MAX_THUMB_RES);
    printf("          -small_res <X_RES> <Y_RES>: resolution for small images.\n");
    printf("                                  default value is %dx%d\n", DEFAULT_SMALL_RES, DEFAULT_SMALL_RES);
    printf("                                  maximum value is %dx%d\n", MAX_SMALL_RES, MAX_SMALL_RES);
    printf("  read   <dbfilename> <pictID> [original|orig|thumbnail|thumb|small]:\n");
    printf("      read an image from the pictDB and save it to a file.\n");
    printf("      default resolution is \"original\".\n");
    printf("  insert <dbfilename> <pictID> <filename>: insert a new image in the pictDB.\n");
    printf("  delete <dbfilename> <pictID>: delete picture pictID from pictDB\n");
    printf("  gc <dbfilename> <tmp dbfilename>: performs garbage collecting on pictDB. Requires a temporary filename for copying the pictDB.\n");
    return 0;
}

/********************************************************************//**
 * Deletes a picture from the database.
 */
int do_delete_cmd(int args, char* argv[])
{
    if(args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* filename = argv[1];

    const char* pictID = argv[2];

    if(strlen(pictID) == 0 || strlen(pictID) > MAX_PIC_ID) {
        return ERR_INVALID_PICID;
    }

    int check_do_open = 0;
    int check_do_delete = 0;
    struct pictdb_file db_file;

    //Open the file in Read and write
    check_do_open = do_open(filename, "rb+", &db_file);
    if(check_do_open == 0) {
        check_do_delete = do_delete(pictID, &db_file);
    }

    do_close(&db_file);

    if(check_do_open != 0) {
        return check_do_open;
    }
    if(check_do_delete != 0) {
        return check_do_delete;
    }

    return 0;
}

/**
* @brief Reads an image from the disk and returns it.
*
* @param file The file from which we read the image.
* @param size Pointer which will contain the address of the image size.
*
* @return img "table" of char (used as bytes).
*/
char* read_disk_image(FILE* file, size_t* size)
{
    //Get the size of the image
    if(fseek(file, 0, SEEK_END) != 0) {
        return NULL;
    }
    *size = ftell(file);
    if(fseek(file, 0, SEEK_SET) != 0) {
        return NULL;
    }

    char* img = malloc(*size);

    if(img == NULL) {
        return NULL;
    }

    if(fread(img, *size, 1, file) != 1) {
        return NULL;
    }
    return img;
}

/********************************************************************//**
 * Inserts a picture in the database.
 */
int do_insert_cmd(int args, char* argv[])
{
    if(args < 4) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* dbfilename = argv[1];
    const char* pictID = argv[2];
    const char* filename = argv[3];

    struct pictdb_file db_file;
    int check = 0;
    check =  do_open(dbfilename, "rb+", &db_file);
    if(check != 0) {
        do_close(&db_file);
        return check;
    }

    size_t size;

    FILE* file = fopen(filename, "rb");
    if(file == NULL) {
        do_close(&db_file);
        return ERR_IO;
    }

    char* img = read_disk_image(file, &size);
    fclose(file);
    if(img == NULL) {
        do_close(&db_file);
        return ERR_IO;
    }

    check = do_insert(img, size, pictID, &db_file);
    if(check != 0) {
        free(img);
        img = NULL;
        do_close(&db_file);
        return check;
    }
    free(img);
    img = NULL;
    do_close(&db_file);

    return check;
}

/**
* @brief Function creating the name of the image.
*
* @param name String of char which will contain the image name.
* @param pictID String of char identifying the image.
* @param res Resolution of the image.
*
* @return 0 or an error code if an error occurs.
*/
int create_name(char* name, const char* pictID, const int res)
{
    char resolution_suffix[7];
    switch(res) {
    case RES_ORIG: {
        strcpy(resolution_suffix,"_orig");
        break;
    }
    case RES_SMALL: {
        strcpy(resolution_suffix,"_small");
        break;
    }
    case RES_THUMB: {
        strcpy(resolution_suffix,"_thumb");
        break;
    }
    }
    size_t name_size = strlen(pictID) + strlen(resolution_suffix) + strlen(".jpg");
    if(snprintf(name, MAX_FILE_NAME, "%s%s%s", pictID, resolution_suffix, ".jpg") != name_size) {
        return ERR_IO;
    }
    return 0;
}

/***
* @brief Function writting the image to the disk.
*
* @param filename The image name.
* @param pict_size The image size.
* @param data Address of a "table" of char (used as bytes).
*
* @return 0 or an error code if an error occurs.
*/
int write_disk_image(char* filename, uint32_t pict_size, char** data)
{
    if(data == NULL || pict_size == 0) {
        return ERR_INVALID_ARGUMENT;
    }
    int return_value = 0;
    FILE* file = fopen(filename, "wb");
    if(file == NULL) {
        return_value = ERR_IO;
    } else {
        int check = fwrite(*data, pict_size, 1, file);
        if(check != 1) {
            return_value = ERR_IO;
        }
        fclose(file);
    }
    return return_value;
}

/**
* @brief Function freeing the memory associated to data.
*
* @param data Address of a "table" of char (used as bytes).
*
* @return void.
*/
void free_data(char** data)
{
    if(data != NULL) {
        if(*data != NULL) {
            free(*data);
            *data = NULL;
        }
        free(data);
        data = NULL;
    }
}

/********************************************************************//**
 * Reads a picture from the database.
 */
int do_read_cmd(int args, char* argv[])
{
    if(args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* dbfilename = argv[1];
    const char* pictID = argv[2];
    int res = RES_ORIG;
    if(args >= 4) {
        res = resolution_atoi(argv[3]);
        if(res == -1) {
            return ERR_INVALID_ARGUMENT;
        }
    }

    struct pictdb_file db_file;

    //Open the file in Read and write
    int check = do_open(dbfilename, "rb+", &db_file);

    if(check != 0) {
        do_close(&db_file);
        return check;
    }

    //Variable
    char** data = malloc(sizeof(char*));
    if(data == NULL) {
        do_close(&db_file);
        return ERR_OUT_OF_MEMORY;
    }
    *data = NULL;
    uint32_t pict_size = 0;

    check = do_read(pictID, res, data, &pict_size, &db_file);
    if(check != 0) {
        free_data(data);
        do_close(&db_file);
        return check;
    }

    char name[MAX_FILE_NAME];
    check = create_name(name, pictID, res);
    if(check != 0) {
        free_data(data);
        do_close(&db_file);
        return check;
    }
    check = write_disk_image(name, pict_size, data);

    if(check != 0) {
        free_data(data);
        do_close(&db_file);
        return check;
    }

    free_data(data);
    do_close(&db_file);
    return 0;
}

/********************************************************************//**
 *
 */
int do_gc_cmd(int args, char* argv[])
{
    if(args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* dbfilename = argv[1];
    const char* temp_filename = argv[2];

    struct pictdb_file db_file;

    //Open the file in Read and write
    int check = do_open(dbfilename, "rb+", &db_file);
    if(check != 0) {
        do_close(&db_file);
        return check;
    }

    check = do_gbcollect(&db_file, dbfilename, temp_filename);
    if(check != 0) {
        do_close(&db_file);
        return check;
    }

    do_close(&db_file);
    return 0;
}

/********************************************************************//**
 * MAIN
 */
int main (int argc, char* argv[])
{
    int ret = 0;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {

        if (VIPS_INIT(argv[0])) {
            return ERR_VIPS;
        }

        argc--;
        argv++; // skips command call name

        //Current commands available
        command_mapping list_cmd = {"list", do_list_cmd};
        command_mapping create_cmd = {"create", do_create_cmd};
        command_mapping delete_cmd = {"delete", do_delete_cmd};
        command_mapping help_cmd = {"help", help};
        command_mapping read_cmd = {"read", do_read_cmd};
        command_mapping insert_cmd = {"insert", do_insert_cmd};
        command_mapping gc_cmd = {"gc", do_gc_cmd};

        command_mapping tab[NUMBER_OF_COMMAND] = {list_cmd, create_cmd, delete_cmd, help_cmd,
                                                  read_cmd, insert_cmd, gc_cmd
                                                 };

        //Check if we called an existing command
        size_t callMade = 0;

        //Loop that check which command we which to call if it exists.
        for(size_t i = 0; i < NUMBER_OF_COMMAND; i++) {
            if(!strcmp(tab[i].name, argv[0])) {
                callMade = 1;
                ret = tab[i].call(argc, argv);
            }
        }

        if(!callMade) { //callMade == 0
            ret = ERR_INVALID_COMMAND;
        }

        vips_shutdown();

    }

    if (ret) { //ret != 0
        fprintf(stderr, "ERROR: %s\n", ERROR_MESSAGES[ret]);
        (void)help(argc, argv);
    }

    return ret;
}
