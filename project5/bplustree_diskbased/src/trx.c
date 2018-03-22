#include "trx.h"
#include "log.h"

int begin_transaction() {

    trx_seq_num++;
    write_log(trx_seq_num, 0, 0, 0, 0, 0, NULL, NULL);
    flush_log_buffer_pool();
    return 0;
}

int commit_transaction() {
    write_log(trx_seq_num, 2, 0, 0, 0, 0, NULL, NULL);
    //printf("commit flush\n");
    flush_log_buffer_pool();
    //read_log();
    return 0;
}

int abort_transaction() {
    LogRecord* log_record;

    //printf("abort!\n");
    flush_log_buffer_pool(); //일단 다 내리고

    //compensate_log()
    // because no nested trx 
    
    FILE* logfile;

    if ((logfile = fopen("DATA.LOG", "rb")) == NULL)  {
        //printf("No log file\n");
        return -1;
    }

    log_record = read_last_log_record(logfile);

    //printf("[abort_trx] last_log->lsn : %lld, prev_lsn : %lld\n", log_record->lsn, log_record->prev_lsn);


    while (1) {
        if (log_record == NULL) {
            //printf("Trx begin not found\n");
            return -1;
        }
        if (log_record->type == 0) {
            //printf("begin?\n");
            break;
        }
        
        //printf("trx_id : %d, table_id : %d, page_num : %d\n", log_record->trx_id, log_record->table_id, log_record->page_num);
        //printf("offset : %d, datalen : %d\n", log_record->offset, log_record->data_length);
        // make compensate log
        write_log(log_record->trx_id, 1, log_record->table_id, log_record->page_num, 
                log_record->offset, log_record->data_length, log_record->new_image,
                log_record->old_image);
    
        //printf("read prev_lsn record : %lld\n", log_record->prev_lsn);
        log_record = read_log_record_by_lsn(logfile, log_record->prev_lsn);
    }
    // write abort log
    write_log(trx_seq_num, 3, 0, 0, 0, 0, NULL, NULL);
    flush_log_buffer_pool();
    return 0;
}
