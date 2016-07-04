CFLAGS += -g -std=c99 -I/usr/local/opt/openssl/include
CFLAGS += $$(pkg-config vips --cflags)
LDLIBS += $$(pkg-config vips --libs) -lm
LDLIBS += -lssl -lcrypto -ljson-c

all: pictDBM
pictDBM: db_utils.o db_list.o error.o db_create.o db_delete.o image_content.o pictDBM_tools.o dedup.o db_insert.o db_read.o db_gbcollect.o

pictDB_server: pictDB_server.o db_utils.o db_list.o error.o db_create.o db_delete.o image_content.o pictDBM_tools.o dedup.o db_insert.o db_read.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDLIBS) -Llibmongoose -lmongoose

clean: 
	rm *.o
mongoose:
	export LD_LIBRARY_PATH=libmongoose

