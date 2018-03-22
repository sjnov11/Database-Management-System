# Recovery

#### Database System Management Project#5



## Object

Implement transaction API that can support 'Atomicity' and 'Durability' using log manager.
Our log manager satisfies below properties.

- No force (REDO) & Steal (UNDO) policy
- Write Ahead Logging (WAL)
- Recovery when initializing DB



## Log Record Structure 

Log record is consisted of 

- LSN : Start log file offset of a current log record.
- prev LSN : LSN of previous log record.
- Transaction ID : Indicates the transaction that triggers current log record
- Type : Type of current log record (BEGIN : 0, UPDATE : 1, COMMIT : 2, ABORT : 3)
- Table ID : Indicates the data file. (name : DATA[Table ID])
- Page Number : Page that contains the modified area.
- Offset : Start offset of modified area within a page.
- Data Length : The length of modified area.
- Old Image : Old contents of the modified area.
- New Image : New contents of the modified area.

```c
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
```

The *old_image* and *new_image* point real data structure in - memory with *data_length* size. 





## Log File Structure

Unlike project specification, Modified log file structure as shown below.

It contains in-memory *LogRecord* structure info, and its actual *old_image* , *new_image*.

We can parse data file by reading *LogRecord*. When we read *LogRecord*, we can know the size of *old/new image* contents.







## Write Ahead Logging

Our log manager has **No force & Steal** policy, with **Write Ahead Logging** properties.

**1) Steal**

When buffer replacement is occured and buffer should write on disk because it is dirty, first, flush log buffer and then flush buffer to disk.

```c
// Find victim
if (current->pin_count == 0 && current->ref_bit == 0) {
  if (current->is_dirty == 1 && current->page_offset != -1) {				
    flush_log_buffer_pool();
    flush_page(current->table_id, current->frame);
  }
...
```

**2) No force** 

When transaction commits/aborts, write down logs to the disk so DB can recover commit/abort data when crush occurs.

```c
int commit_transaction() {
    write_log(trx_seq_num, 2, 0, 0, 0, 0, NULL, NULL);  
    flush_log_buffer_pool(); 
    return 0;
}
```



## Compensation Logging

When transaction aborts its actions, we make compensation log to cancel actions.

```c
log_record = read_last_log_record(logfile);
    while (1) {
        if (log_record == NULL) {          
            return -1;
        }
        if (log_record->type == 0) {            
            break;
        }
        // write compensate log
        write_log(log_record->trx_id, 1, log_record->table_id, 
                  log_record->page_num, log_record->offset, 
                  log_record->data_length, log_record->new_image, 
                  log_record->old_image);   
       
        log_record = read_log_record_by_lsn(logfile, log_record->prev_lsn);
    }
// write abort log
write_log(trx_seq_num, 3, 0, 0, 0, 0, NULL, NULL);
flush_log_buffer_pool();
```



## Recovery (Redo History)

Recovery will be done by **Redo History** and **Undo losers.**

Loser is opened transaction (BEGIN and no commit/aborts) and there is no nested transaction on this project, loser is the opened transaction from the end of log file. We just undo this 1 opened transaction.

```c
void redo_pass () {
    LogRecord* log_record;
    int tablefile;
    Buffer* buf;
    FILE* logfile;
  
    if ((logfile = fopen("DATA.LOG", "rb")) == NULL) return;
    while (1) {
        log_record = read_log_record(logfile);
        if (log_record == NULL) {          
            break;
        }
        
        if (log_record->data_length == 0) continue;
        buf = get_page_from_buffer(log_record->table_id, 
                                   log_record->page_num * PAGE_SIZE);
       
        if (get_page_lsn(buf->frame) >= log_record->lsn) {            
            free(log_record->old_image);
            free(log_record->new_image);
            buf->pin_count--;
            continue; 
        }   
        
        memcpy(((char*)buf->frame) + log_record->offset, 
               log_record->new_image, log_record->data_length);
        set_page_lsn(buf->frame, &(log_record->lsn));       
        buf->is_dirty = 1;
        buf->pin_count--;
        free(log_record->old_image);
        free(log_record->new_image);
    }
    fclose(logfile);
}
```



```c
void undo_pass() {
    LogRecord* log_record;
    int tablefile;
    Buffer* buf;
    FILE* logfile;
    int64_t undo_log_lsn;
   
    if ((logfile = fopen("DATA.LOG", "rb")) == NULL) return;
    
    log_record = read_last_log_record(logfile);
  
    if (log_record == NULL) return;    
    if (log_record->type == 2 || log_record->type == 3) return;
  
    undo_log_lsn = log_record->lsn;   
    
    while (1) {
        log_record = read_log_record_by_lsn(logfile, undo_log_lsn);
        if (log_record == NULL) {           
            break;
        }
      
        if (log_record->type != 1) break;  
        buf = get_page_from_buffer(log_record->table_id, 
                                   log_record->page_num * PAGE_SIZE);
        
        if (get_page_lsn(buf->frame) < log_record->lsn) {            
            buf->pin_count--;
            continue; 
        }
        
        
        memcpy(((char*)buf->frame) + log_record->offset, 
               log_record->old_image, log_record->data_length);
        set_page_lsn(buf->frame, &(log_record->lsn));
        undo_log_lsn = log_record->prev_lsn;
        buf->is_dirty = 1;
        buf->pin_count--;
        free(log_record->old_image);
        free(log_record->new_image);
    }   
    fclose(logfile);
}
```

