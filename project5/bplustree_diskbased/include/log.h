#ifndef __LOG_H__
#define __LOG_H__
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>


#define LOG_BUF_POOL_SIZE           5

typedef struct _LogRecord {
    int64_t lsn;
    int64_t prev_lsn;
    int trx_id;
    int type;
    int table_id;
    int page_num;
    int offset;
    int data_length;

    void* old_image;
    void* new_image;
} LogRecord;

void init_log_buffer_pool();
int64_t write_log(int trx_id, int type, int table_id, int page_num, 
               int offset, int data_length, void* old, void* new);
LogRecord* read_log_record(FILE* logfile);
LogRecord* read_last_log_record(FILE* logfile);
LogRecord* read_log_record_by_lsn(FILE* logfile, int64_t lsn);

void flush_log_buffer_pool();
void flush_log(LogRecord* log_buffer);

void redo_pass();
void undo_pass();

LogRecord log_buffer_pool[LOG_BUF_POOL_SIZE];

#endif
