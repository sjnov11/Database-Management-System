#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "file.h"

typedef struct _Buffer {
	union {
		Page *frame;
		JoinFrame *join_frame;
	};
	union {
		int table_id;
		int num_join;
	};
	off_t page_offset;
	int is_dirty;
	int pin_count;
	int ref_bit;
	//struct _Buffer *next_buffer;
} Buffer;

Buffer *buffer_pool;
int buffer_pool_size;

int init_buffer_pool(int num_buf);
Buffer* write_page_to_buffer(int table_id, off_t offset);
Buffer* write_new_page_to_buffer(int table_id, Page* new_page);
Buffer* read_page_from_buffer(int table_id, off_t offset);
Buffer* get_page_from_buffer(int table_id, off_t offset);
Buffer* get_join_result_buffer(void);
void flush_buffer(int table_id);
void destroy_buffer(void);
int unpinning_buffer_page(int table_id, off_t offset);
#endif
