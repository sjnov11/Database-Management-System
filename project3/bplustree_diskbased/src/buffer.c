#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "buffer.h"

int clk_hand = 0;

int init_buffer_pool(int num_buf) {
	int i;
	buffer_pool = malloc(sizeof(Buffer) * num_buf);
	buffer_pool_size = num_buf;

	for (i = 0; i < buffer_pool_size; i++) {
		buffer_pool[i].frame = malloc(sizeof(Page));
		buffer_pool[i].table_id = 0;
		buffer_pool[i].page_offset = 0;
		buffer_pool[i].is_dirty = 0;
		buffer_pool[i].pin_count = 0;
		buffer_pool[i].ref_bit = 0;
	}

	return 0;
}


// Write page to the empty buffer or
// replace buffer page to new page
// following clock LRU algorithm.
// Return allocated buffer structure.
Buffer* write_page_to_buffer(int table_id, off_t offset) {
	Page* ret = NULL;
	Buffer* current;

	while (ret == NULL) {
		current = &(buffer_pool[clk_hand]);

		// Find victim
		if (current->pin_count == 0 && current->ref_bit == 0) {
			if (current->is_dirty == 1)				
				flush_page(current->table_id, current->frame);

			load_page(table_id, offset, current->frame);
			ret = current->frame;
			current->table_id = table_id;
			current->page_offset = offset;
			current->is_dirty = 0;
			current->pin_count = 1;
			current->ref_bit = 1;
			//current.next_buffer
		}

		else if (current->pin_count == 0 && current->ref_bit == 1) {
			current->ref_bit = 0;
		}

		clk_hand = (clk_hand + 1) % buffer_pool_size; 

	}
	return current;
}

Buffer* write_new_page_to_buffer(int table_id, Page* new_page) {
	Page* ret = NULL;
	Buffer* current;

	while (ret == NULL) {
		current = &(buffer_pool[clk_hand]);

		// Find victim
		if (current->pin_count == 0 && current->ref_bit == 0) {
			if (current->is_dirty == 1)				
				flush_page(current->table_id, current->frame);
			memcpy(current->frame, new_page, sizeof(Page));
			ret = current->frame;
			current->table_id = table_id;
			current->page_offset = new_page->file_offset;
			current->is_dirty = 0;
			current->pin_count = 1;
			current->ref_bit = 1;
			//current.next_buffer
		}

		else if (current->pin_count == 0 && current->ref_bit == 1) {
			current->ref_bit = 0;
		}

		clk_hand = (clk_hand + 1) % buffer_pool_size; 

	}
	return current;

}

// Read page from buffer
// Return buffer structure if exist,
// else NULL.
Buffer* read_page_from_buffer(int table_id, off_t offset) {
	int i;

	for (i = 0; i < buffer_pool_size; i++) {
		if (buffer_pool[i].table_id == table_id &&
				buffer_pool[i].page_offset == offset) {
			buffer_pool[i].pin_count++;
			return &buffer_pool[i];
		}
	}
	return NULL;
}

Buffer* get_page_from_buffer(int table_id, off_t offset) {
	Buffer* buf;
	buf = read_page_from_buffer(table_id, offset);
	if (buf == NULL) {
		// Add page to buffer
		buf = write_page_to_buffer(table_id, offset);
	}

	return buf;	
}

void flush_buffer(int table_id) {
	int i;
	for (i = 0; i < buffer_pool_size; i++) {
		if (buffer_pool[i].table_id == table_id && buffer_pool[i].is_dirty == 1) {
			flush_page(table_id, buffer_pool[i].frame);			
		}

		buffer_pool[i].table_id = 0;
		buffer_pool[i].page_offset = 0;
		buffer_pool[i].is_dirty = 0;
		buffer_pool[i].pin_count = 0;
		buffer_pool[i].ref_bit = 0;
	}	
}

void destroy_buffer(void) {
	int i;
	for (i = 0; i < buffer_pool_size; i++) {
		if (buffer_pool[i].is_dirty == 1) {
			flush_page(buffer_pool[i].table_id, buffer_pool[i].frame);			
		}
		free(buffer_pool[i].frame);
		buffer_pool[i].table_id = 0;
		buffer_pool[i].page_offset = 0;
		buffer_pool[i].is_dirty = 0;
		buffer_pool[i].pin_count = 0;
		buffer_pool[i].ref_bit = 0;		
	}	
	free(buffer_pool);
}

int unpinning_buffer_page(int table_id, off_t offset) {
	int i;

	for (i = 0; i < buffer_pool_size; i++) {
		if (buffer_pool[i].table_id == table_id &&
				buffer_pool[i].page_offset == offset) {
			buffer_pool[i].pin_count--;
			return 0;
		}
	}
	return -1;
}
