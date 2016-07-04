/**
* @file pictDB_server.c
*
* @brief Small webserver managing a picture database.
*/

#include "libmongoose/mongoose.h"
#include "pictDB.h"
#include <vips/vips.h>
#include <string.h>

#define MAX_QUERY_PARAM 5
#define MAX_FILE_NAME 1024

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

/**
* @struct db_file
*
* @brief Global database
*/
struct pictdb_file db_file;

/**
* @brief Free the pointer received as parameter
*
* @param void* pointer
*/
void do_free(void* ptr)
{
    if(ptr != NULL) {
        free(ptr);
        ptr = NULL;
    }
}

/**
* @brief Function freeing the memory associated to data.
*
* @param data Address of a "table" of char (used as bytes).
*/
void free_data(char** data)
{
    if(data != NULL) {
        if(*data != NULL) {
            free(*data);
            *data = NULL;
        }
    }
}

/**
* @brief Routine that return a HTTP error code.
*
* @param nc A pointer to a mongoose connection
*
* @param error An error code
*/
static void mg_error(struct mg_connection* nc, int error)
{
    if(error > 0 && error < 16) {
        mg_printf(nc, "HTTP/1.1 500\r\nContent-Length: 0\r\n\r\n%s", ERROR_MESSAGES[error]);
    }
}

/**
* @brief Split the query string in several chunks.
*
* @param result[] Pointer table of string
*
* @param tmp String containing the several part of the query string
*
* @param src The query string.
*
* @param delim String containing delimiters.
*
* @param len Query string length.
*/
static void split (char* result[], char* tmp, const char* src, const char* delim, size_t len)
{
    strncpy(tmp, src, len);
    result[0] = strtok(tmp, delim);
    for(size_t i = 1; i < MAX_QUERY_PARAM; i++) {
        result[i] = strtok(NULL, delim);
    }
}

/**
* @brief Funtion that handles list calls. Called by the event handler.
*
* @param nc A pointer to a mongoose connection
*/
static void handle_list_call(struct mg_connection *nc)
{
    const char* JSON_list = do_list(&db_file, JSON);
    mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(JSON_list), JSON_list);
    do_free((char*)JSON_list);
}

/**
* @brief Function that handles read call. Called by the event handler.
*
* @param nc A pointer to a mongoose connection
*
* @param mssg A pointer to a http_message
*/
static void handle_read_call(struct mg_connection *nc, struct http_message *mssg)
{
    char* tmp = calloc(MAX_QUERY_PARAM, MAX_PIC_ID+1);
    if(tmp == NULL) {
        mg_error(nc, ERR_OUT_OF_MEMORY);
    } else {
        const char delim[] = "&=";
        char* result[MAX_QUERY_PARAM];
        size_t len = mssg->query_string.len;
        size_t resolution = -1;
        char pict_id[MAX_PIC_ID + 1];
        //Split the query->string
        split(result, tmp, mssg->query_string.p, delim, len);
        //We get resolution and pict_id from the query.
        for(size_t i = 0; i < MAX_QUERY_PARAM; i++) {
            if(result[i] != NULL && i+1 < MAX_QUERY_PARAM) {
                if(!strncmp(result[i], "res", strlen("res"))) { //Peut-être factoriser le i+1 < MAX_QUERRY...?
                    resolution = resolution_atoi(result[i+1]);
                } else if(!strncmp(result[i], "pict_id", strlen("pict_id"))) {
                    if(result[i+1] != NULL) {
                        strncpy(pict_id, result[i+1], MAX_PIC_ID);
                    }
                }
            }
        }
        //We check that there were the 2 arguments resolution and pict_id in the querry.
        if(resolution == -1) {
            mg_error(nc, ERR_NOT_ENOUGH_ARGUMENTS);
        } else {
            char* data;
            uint32_t pict_size = 0;
            int check = do_read(pict_id, resolution, &data, &pict_size, &db_file);
            if(check != 0) {
                mg_error(nc, check);
            } else {
                mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", pict_size);
                mg_send(nc, data, (int)pict_size);
            }
            free_data(&data);
        }
        do_free(tmp);
    }
}

/**
* @brief Function that handles insertion. Called by the event handler.
*
* @param nc A pointer to a mongoose connection
*
* @param mssg A pointer to a http_message
*/
static void handle_insert_call(struct mg_connection *nc, struct http_message *mssg)
{
    char var_name[MAX_PIC_ID+1];
    char pict_id[MAX_PIC_ID+1];
    const char* img;
    size_t size = 0;

    mg_parse_multipart(mssg->body.p,
                       mssg->body.len,
                       var_name, MAX_PIC_ID,
                       pict_id, MAX_PIC_ID,
                       &img, &size);
    if(size == 0) {
        mg_error(nc, ERR_INVALID_ARGUMENT);
    } else {
        int check = do_insert(img, size, pict_id, &db_file);
        if(check != 0) {
            mg_error(nc, check);
        } else {
            mg_printf(nc, "HTTP/1.1 302 Found\r\nLocation: http://localhost:%s/index.html\r\n\r\n", s_http_port);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        }
    }
}

/**
* @brief Function that handles deletion. Called by the event handler.
*
* @param nc A pointer to a mongoose connection
*
* @param mssg A pointer to a http_message
*/
static void handle_delete_call(struct mg_connection *nc, struct http_message *mssg)
{
    char* tmp = calloc(MAX_QUERY_PARAM, MAX_PIC_ID+1);
    if(tmp == NULL) {
        mg_error(nc, ERR_OUT_OF_MEMORY);
    } else {
        const char delim[] = "&=";
        char* result[MAX_QUERY_PARAM];
        size_t len = mssg->query_string.len;
        char pict_id[MAX_PIC_ID + 1];
        //Split the query->string
        split(result, tmp, mssg->query_string.p, delim, len);
        //We get resolution and pict_id from the query.
        for(size_t i = 0; i < MAX_QUERY_PARAM; i++) {
            if(result[i] != NULL && !strncmp(result[i], "pict_id", strlen("pict_id")) && i+1 < MAX_QUERY_PARAM) {
                if(result[i+1] != NULL) {
                    strncpy(pict_id, result[i+1], MAX_PIC_ID);
                }
            }
        }
        int check = do_delete(pict_id, &db_file);
        if(check != 0) {
            mg_error(nc, check);
        } else {
            mg_printf(nc,"HTTP/1.1 302 Found\r\nLocation: http://localhost:%s/index.html\r\n\r\n", s_http_port);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        }
        do_free(tmp);
    }
}

/**
* @brief Function that handles and dispatches http requests
*
* @param nc A pointer to a mongoose connection
*
* @param ev_data A pointer to a http_message
*/
static void ev_handler(struct mg_connection *nc, int ev, void* ev_data)
{
    struct http_message *hm = (struct http_message*) ev_data;
    switch (ev) {
    case MG_EV_HTTP_REQUEST:
        if (mg_vcmp(&hm->uri, "/pictDB/list") == 0) {
            handle_list_call(nc);
        } else if(mg_vcmp(&hm->uri, "/pictDB/read") == 0) {
            handle_read_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/insert") == 0) {
            handle_insert_call(nc, hm);
        } else if(mg_vcmp(&hm->uri, "/pictDB/delete") == 0) {
            handle_delete_call(nc, hm);
        } else {
            mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
        }
        break;
    default:
        break;
    }
}

/************************************************************
* Main
*************************************************************/
int main(int argc, char* argv[])
{
    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    } else {

        if (VIPS_INIT(argv[0])) {
            return ERR_VIPS;
        }

        const char* dbfilename = argv[1];

        //Open the file in Read and write
        int check = do_open(dbfilename, "rb+", &db_file);

        if(check != 0) {
            do_close(&db_file);
            return check;
        }
        //Print the header
        print_header(&db_file.header);

        struct mg_mgr mgr;
        struct mg_connection *nc;

        mg_mgr_init(&mgr, NULL);
        nc = mg_bind(&mgr, s_http_port, ev_handler);
        mg_set_protocol_http_websocket(nc);
        s_http_server_opts.document_root = ".";  // Serve current directory
        s_http_server_opts.dav_document_root = ".";  // Allow access via WebDav
        s_http_server_opts.enable_directory_listing = "yes";

        for (;;) {
            mg_mgr_poll(&mgr, 1000);
        }

        //Shutdown
        do_close(&db_file);
        mg_mgr_free(&mgr);

        vips_shutdown();

        return 0;
    }
}