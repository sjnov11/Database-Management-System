#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "buffer.h"



int curr;
int64_t prev_lsn = 0;
int64_t log_tail = 0;

void init_log_buffer_pool() {
    int i;
    for (i = 0; i < LOG_BUF_POOL_SIZE; i++) {
        LogRecord* log_rec = &log_buffer_pool[i];
        log_rec->lsn = -1;
        log_rec->prev_lsn = 0;
        log_rec->trx_id = 0;
        log_rec->type = 0;
        log_rec->table_id = 0;
        log_rec->page_num = 0;
        log_rec->offset = 0;
        log_rec->data_length = 0;

        log_rec->old_image = NULL;
        log_rec->new_image = NULL;
    }
    // initialize log tail & prev_lsn
    FILE* logfile;
    if( (logfile = fopen("DATA.LOG", "rb")) != NULL ) {
        LogRecord* last_log_rec = read_last_log_record(logfile);
        if (last_log_rec != NULL) {
            log_tail = last_log_rec->lsn + sizeof(LogRecord) + last_log_rec->data_length * 2;
            prev_lsn = last_log_rec->prev_lsn;
        }

        fclose(logfile);
    }
       

    curr = 0;

}

void clear_log_buffer_pool() {
    int i;
    for (i = 0; i < LOG_BUF_POOL_SIZE; i++) {
        LogRecord* log_rec = &log_buffer_pool[i];
        log_rec->lsn = -1;
        log_rec->prev_lsn = 0;
        log_rec->trx_id = 0;
        log_rec->type = 0;
        log_rec->table_id = 0;
        log_rec->page_num = 0;
        log_rec->offset = 0;
        log_rec->data_length = 0;

        log_rec->old_image = NULL;
        log_rec->new_image = NULL;
    }
    curr = 0;
}
// write log to log buffer 
int64_t write_log(int trx_id, int type, int table_id, 
        int page_num, int offset, int data_length, void* old, void* new) {

    LogRecord* log_buffer = &log_buffer_pool[curr % LOG_BUF_POOL_SIZE];

    if (log_buffer->lsn != -1) {
        //printf("there is already log buf\n");
        flush_log_buffer_pool();
        //flush_log(log_buffer);
        //clear_log_buffer_pool();
    }

    log_buffer->trx_id = trx_id;
    log_buffer->type = type;
    log_buffer->table_id = table_id;
    log_buffer->page_num = page_num;
    log_buffer->offset = offset;
    log_buffer->data_length = data_length;
    log_buffer->old_image = old;
    log_buffer->new_image = new;

    log_buffer->prev_lsn = prev_lsn;
    log_buffer->lsn = log_tail;

    prev_lsn = log_buffer->lsn;
    log_tail = log_tail + sizeof(LogRecord) + data_length * 2;

    //printf("[write_log] lsn : %lld, prev_lsn : %lld\n", log_buffer->lsn, log_buffer->prev_lsn);
    //printf("[write_log] curr: %d\n", curr);
    
    curr++;
    

    return log_buffer->lsn;
}

LogRecord* read_log_record(FILE *logfile) {
    int data_len;
    LogRecord* log = (LogRecord*)malloc(sizeof(LogRecord));
    void *old, *new;


    //printf("fseek : %lld logrec size : %d\n", ftell(logfile), sizeof(LogRecord));
    fread(log, sizeof(LogRecord), 1, logfile);     
    if (feof(logfile)) {
        return NULL;
    }

    data_len = log->data_length;
    if (data_len > 0) {
        old = (void*)malloc(data_len);
        new = (void*)malloc(data_len);
    
        fread(old, data_len, 1, logfile);
        fread(new, data_len, 1, logfile);
    
        log->old_image = old;
        log->new_image = new;
    }
    
    //free 
    return log;
}

LogRecord* read_last_log_record(FILE *logfile) {

    LogRecord* current = read_log_record(logfile);
    LogRecord* next = read_log_record(logfile);

    while (next != NULL) {
        //printf("loop curr lsn : %lld\n", current->lsn);
        free(current->old_image);
        free(current->new_image);
        free(current);
        current = next;
        next = read_log_record(logfile);
    }

    //printf("final curr lsn : %lld\n", current->lsn);

    return current;
}

LogRecord* read_log_record_by_lsn(FILE *logfile, int64_t lsn) {
    LogRecord* log;

    fseek(logfile, lsn, SEEK_SET);
    //printf("fseek : %lld\n", ftell(logfile));

    log = read_log_record(logfile);

    return log;   

}


void flush_log_buffer_pool() {
    int i;
    LogRecord* log_buffer;
    for (i = 0; i < LOG_BUF_POOL_SIZE; i++) {
        log_buffer = &log_buffer_pool[i];
        //printf("[%d] lsn : %d\n", i , log_buffer->lsn);
        if (log_buffer->lsn == -1) break;
        flush_log(log_buffer);
    }
    curr = 0;
   // init_log_buffer_pool();
    clear_log_buffer_pool();
}


void flush_log(LogRecord* log_buffer) {
    FILE* logfile = fopen("DATA.LOG", "a+b");
    //printf("[flush_log] ftell : %lld\n", ftell(logfile));
    fseek(logfile, 0L, SEEK_END);
    //printf("[flush_log] end : %lld\n", ftell(logfile));
    
    fwrite(log_buffer, sizeof(LogRecord), 1, logfile);
    //printf("[flush_log] ftell : %lld\n", ftell(logfile));
    if (log_buffer->data_length > 0) {
        fwrite(log_buffer->old_image, log_buffer->data_length, 1, logfile);
        fwrite(log_buffer->new_image, log_buffer->data_length, 1, logfile);
        // deallocate image pointer
        free(log_buffer->old_image);
        free(log_buffer->new_image);
    }
    
    fclose(logfile);
}


void redo_pass () {
    LogRecord* log_record;
    int tablefile;
    Buffer* buf;
    FILE* logfile;

    //printf("Start redo pass\n");

    if ((logfile = fopen("DATA.LOG", "rb")) == NULL) return;

    while (1) {
        log_record = read_log_record(logfile);
        if (log_record == NULL) {
            //printf("no more log\n");
            break;
        }

        //printf("lsn : %lld, data : %d\n", log_record->lsn, log_record->data_length);
        //printf("table_id : %d, page_num : %d\n", log_record->table_id, log_record->page_num);
        //printf("offset : %lld, old : %s, new : %s\n", log_record->offset, (char*)log_record->old_image, (char*)log_record->new_image);
        if (log_record->data_length == 0) continue;
        buf = get_page_from_buffer(log_record->table_id, log_record->page_num * PAGE_SIZE);
        //printf("1) %lld\n", ((LeafPage*)(buf->frame))->page_lsn);
        //printf("2) %lld\n", get_page_lsn(buf->frame));
        if (get_page_lsn(buf->frame) >= log_record->lsn) {
            //printf("continue\n");
            free(log_record->old_image);
            free(log_record->new_image);
            buf->pin_count--;
            continue; 
        }
        
        
        //printf("!!111: %s\n", LEAF_VALUE((LeafPage*)buf->frame,(log_record->offset-128)/128 ));
        //printf("!!! : %s\n", (char*)buf->frame + log_record->offset);
        memcpy(((char*)buf->frame) + log_record->offset, log_record->new_image, log_record->data_length);
        set_page_lsn(buf->frame, &(log_record->lsn));

        //printf("!!! page_lsn : %lld\n", get_page_lsn(buf->frame));
        buf->is_dirty = 1;
        buf->pin_count--;
        free(log_record->old_image);
        free(log_record->new_image);

    }
    fclose(logfile);

}


void undo_pass() {

    LogRecord* log_record;
    int tablefile;
    Buffer* buf;
    FILE* logfile;
    int64_t undo_log_lsn;

    //printf("Start undoo pass\n");

    if ((logfile = fopen("DATA.LOG", "rb")) == NULL) return;

    
    log_record = read_last_log_record(logfile);
    
    if (log_record == NULL) return;

    //printf("log_rec type : %d \n", log_record->type);
    if (log_record->type == 2 || log_record->type == 3) return;

    undo_log_lsn = log_record->lsn;

    //printf("undo_log_lsq : %lld\n", undo_log_lsn);
    
    
    while (1) {
        log_record = read_log_record_by_lsn(logfile, undo_log_lsn);
        if (log_record == NULL) {
           // printf("no more log\n");
            break;
        }

       // printf("lsn : %lld, data : %d\n", log_record->lsn, log_record->data_length);
       // printf("table_id : %d, page_num : %d\n", log_record->table_id, log_record->page_num);
        //printf("offset : %lld, old : %s, new : %s\n", log_record->offset, (char*)log_record->old_image, (char*)log_record->new_image);
        if (log_record->type != 1) break;  // last record is begin or commit or abort 
        buf = get_page_from_buffer(log_record->table_id, log_record->page_num * PAGE_SIZE);
        //printf("1) %lld\n", ((LeafPage*)(buf->frame))->page_lsn);
        //printf("2) %lld\n", get_page_lsn(buf->frame));
        if (get_page_lsn(buf->frame) < log_record->lsn) {
            //printf("???%lld\n", get_page_lsn(buf->frame));
            //printf("continue\n");
            buf->pin_count--;
            continue; 
        }
        
        
        memcpy(((char*)buf->frame) + log_record->offset, log_record->old_image, log_record->data_length);
        set_page_lsn(buf->frame, &(log_record->lsn));
        undo_log_lsn = log_record->prev_lsn;
        buf->is_dirty = 1;
        buf->pin_count--;
        free(log_record->old_image);
        free(log_record->new_image);

    }
    //printf("End Undo pass\n");
    fclose(logfile);

}
