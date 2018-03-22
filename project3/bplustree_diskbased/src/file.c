#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
//#include "file.h"
#include "buffer.h"

HeaderPage dbheader;
int dbfile;
int tablefile;

// Get free page to use.
// If no more free page exist, expand file
off_t get_free_page(int table_id) {
    off_t freepage_offset;
    Buffer* buf;
    
    freepage_offset = dbheader.freelist;
    if (freepage_offset == 0) {
        // No more free page, expand file as twice
        expand_file(table_id, dbheader.num_pages);
        freepage_offset = dbheader.freelist;
    }
   
    FreePage freepage;
    //load_page(freepage_offset, (Page*)&freepage);
    buf = get_page_from_buffer(table_id, freepage_offset);
    freepage = *((FreePage*)buf->frame);
    buf->pin_count--;
    dbheader.freelist = freepage.next;
    
    //flush_page((Page*)&dbheader);
    buf = get_page_from_buffer(table_id, dbheader.file_offset);
    memcpy(buf->frame, &dbheader, sizeof(Page));
    buf->is_dirty = 1;
    buf->pin_count--;
    
    return freepage_offset;
}

// Put free page to the free list
void put_free_page(int table_id, off_t page_offset) {
    FreePage freepage;
    Buffer* buf;
    memset(&freepage, 0, PAGE_SIZE);

    freepage.next = dbheader.freelist;
    freepage.file_offset = page_offset;
    //flush_page((Page*)&freepage);
    buf = get_page_from_buffer(table_id, freepage.file_offset);
    memcpy(buf->frame, &freepage, sizeof(Page));
    buf->is_dirty = 1;
    buf->pin_count--;
    
    dbheader.freelist = page_offset;

    //flush_page((Page*)&dbheader);
    buf = get_page_from_buffer(table_id, dbheader.file_offset);
    memcpy(buf->frame, &dbheader, sizeof(Page));
    buf->is_dirty = 1;
    buf->pin_count--;
}

// Expand file pages and prepend them to the free list
void expand_file(int table_id, size_t cnt_page_to_expand) {
    off_t offset = dbheader.num_pages * PAGE_SIZE;
    Buffer* buf;

    if (dbheader.num_pages > 1024 * 1024) {
        // Test code: do not expand over than 4GB
        assert("Test: you are already having a DB file over than 4GB");
    }
    
    int i;
    for (i = 0; i < cnt_page_to_expand; i++) {
        put_free_page(table_id, offset);
        dbheader.num_pages++;
        offset += PAGE_SIZE;
    }

    //flush_page((Page*)&dbheader);
    buf = get_page_from_buffer(table_id, dbheader.file_offset);
    memcpy(buf->frame, &dbheader, sizeof(Page));
    buf->is_dirty = 1;
    buf->pin_count--;
}

void load_page(int table_id, off_t offset, Page* page) {
    //lseek(dbfile, offset, SEEK_SET);
    //read(dbfile, page, PAGE_SIZE);
	lseek(table_id, offset, SEEK_SET);
    read(table_id, page, PAGE_SIZE);

    page->file_offset = offset;
}

void flush_page(int table_id, Page* page) {
    //lseek(dbfile, page->file_offset, SEEK_SET);
    //write(dbfile, page, PAGE_SIZE);
	lseek(table_id, page->file_offset, SEEK_SET);
    write(table_id, page, PAGE_SIZE);

}

