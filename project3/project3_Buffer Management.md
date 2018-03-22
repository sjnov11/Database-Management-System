---
typora-root-url: ./
---

# Buffer Management

#### Database System Management Project#3



## Object

Current disk-based b+tree doesn't support buffer management.
Our goal is to implement **in-memory buffer manager** to cache on-disk pages.



## Buffer structure

The buffer structure consists of following fields.

- **frame** : containing data contents of target page.
- **table_id** : the unique id of tavble (per file)
- **page_offset** : the position of target page within a file.
- **is_dirty** : if the buffer is modified, set **is_dirty** to 1. write this page during eviction.
- **pin_count** : if the buffer is pinned (which means that using this page), do not evict this page.
- **ref_bit** : check whether *clock LRU* has recently referenced this buffer.  


```c
typedef struct _Buffer {
  Page *frame;
  int table_id;
  off_t page_offset;
  int is_dirty;
  int pin_count;
  int ref_bit;
} Buffer;
```



## Buffer replacement policy 

#### Clock replacement

The **clock replacement** is simillar to LRU, but less overhead. The idea is to choose a page for replacement using a ***current*** variable that takes on values 1 through *N*, in circular order, where *N* is the number of buffer frames. *Current* frame is considered for replacement.  

The execution order of clock replacement is as follows.

> -  If the *pin_count* = 0 and *ref_bit* = 0, then *current* frame is chosen for replacement. If the *dirty_bit* = 1, flush buffer page to the disk.
>
>
> - If the *pin_count* = 0 and *ref_bit* = 1, set *ref_bit* to 0. (Meaning that this frame has referenced.)
> - Change *current* frame to the next buffer frame.

The following code shows a clock replacement implementation.

```c
Buffer* write_page_to_buffer(int table_id, off_t offset) {
	Page* ret = NULL;
	Buffer* current;
	while (ret == NULL) {
		current = &(buffer_pool[clk_hand]);
		// Find victim
		if (current->pin_count == 0 && current->ref_bit == 0) {
			if (current->is_dirty == 1)				
				flush_page(current->frame);
			load_page(offset, current->frame);
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
```



## Buffer manager 

Current b+plus reads/writes data directly from disk using ***load_page()*** , ***flush_page()*** functions. The buffer manager caches this data to in-memory.  

The buffer manager has the following functions. 

- **init_buffer_pool**  : create and initialize buffer pool of a given size.

  ```c
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
  ```

- **read_page_from_buffer** : Read page from the buffer that matches the given table_id and offset.

  ```c
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
  ```

- **get_page_from_buffer** : Get page from the buffer that matches the given table_id and offset. If buffer has the requested page, read it from buffer. If the buffer doesn't have the requested page, find victim buffer and replace it to the new one containing requested page.

  ```c
  Buffer* get_page_from_buffer(int table_id, off_t offset) {
  	Buffer* buf;
  	buf = read_page_from_buffer(table_id, offset);
  	if (buf == NULL) {
  		// Add page to buffer
  		buf = write_page_to_buffer(table_id, offset);
  	}
  	return buf;	
  }
  ```

- **flush_buffer** : Flush all pages of table_id from buffer to disk.

  ```c
  void flush_buffer(int table_id) {
  	int i;
  	for (i = 0; i < buffer_pool_size; i++) {
  		if (buffer_pool[i].table_id == table_id && buffer_pool[i].is_dirty == 1) {
  			flush_page(buffer_pool[i].frame);			
  		}
  		buffer_pool[i].table_id = 0;
  		buffer_pool[i].page_offset = 0;
  		buffer_pool[i].is_dirty = 0;
  		buffer_pool[i].pin_count = 0;
  		buffer_pool[i].ref_bit = 0;
  	}	
  }
  ```

- **destroy_buffer** : Flush all data from buffer and destroy allocated buffer.

  ```c
  void destroy_buffer(void) {
  	int i;
  	for (i = 0; i < buffer_pool_size; i++) {
  		if (buffer_pool[i].is_dirty == 1) {
  			flush_page(buffer_pool[i].frame);			
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
  ```

  ​

## Implement buffer manager to diskbased b+tree

Make all read / write through the buffer manager rather than file I/O like ***load_page()***, ***flush_page()***.

- **Read page using buffer manager**

  ``` c
  NodePage page;
  //Load_Page(offset, (Page*)&page);
  Buffer *buf = get_page_from_buffer(table_id, offset);
  page = *((NodePage*)buf->frame);
  buf->pin_count--;
  ```

- **Write page using buffer manager**

  ``` c
  NodePage page;
  //Flush_Page(offset, (Page*)&page)
  Buffer* buf = get_page_from_buffer(table_id, offset);
  page = *((NodePage*)buf->frame);
  buf->is_dirty = 1;
  buf->pin_count--;
  ```




## Performance result

The performance comparison between disked based b+tree and buffer management b+tree(buffer size = 16) was as follows.

![](/graph.PNG)



In the results, sequential insertion and deletion, which uses buffer frequently, was improved noticeably. But in random insertion / deletion, there was no significant differences between them.