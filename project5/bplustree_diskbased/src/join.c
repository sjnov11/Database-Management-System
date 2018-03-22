#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "join.h"


int join_table(int table_id_1, int table_id_2, char* pathname) {
	Buffer *buf1, *buf2, *result_buf;
	int flag = 1;
	int index_1 = 0, index_2 = 0;
	int64_t key1, key2;
	int result_table_id;
	off_t result_offset;
	int i;

	FILE* fp;
	if((fp = fopen(pathname, "w+")) == NULL) {
		printf("create join result file failed\n");
		return -1;
	}
	/*
	//result_table_id = open(pathname, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	if (result_table_id < 0) {
		assert("failed to create new result join table file");
		return -1;
	}

	//result_buf = get_page_from_buffer(result_table_id, 0);
	*/
	

	result_buf = get_join_result_buffer();
	buf1 = get_left_most_leaf_page(table_id_1);
	buf2 = get_left_most_leaf_page(table_id_2);

	JoinFrame* join_frame = ((JoinFrame*)result_buf->join_frame);
	LeafPage* leaf_node_1 = ((LeafPage*)buf1->frame);
	LeafPage* leaf_node_2 = ((LeafPage*)buf2->frame);	

	while (flag == 1) 
	{		
		key1 = LEAF_KEY(leaf_node_1, index_1);
		key2 = LEAF_KEY(leaf_node_2, index_2);	

		while (key1 < key2) {
			flag = move_advance(&buf1, &leaf_node_1, &index_1);
			if (flag == 1) 
				key1 = LEAF_KEY(leaf_node_1, index_1);
			
			else
				break;
		}			
		while (key1 > key2) {
			flag = move_advance(&buf2, &leaf_node_2, &index_2);
			if (flag == 1) 
				key2 = LEAF_KEY(leaf_node_2, index_2);
			else
				break;
		}

		if (key1 == key2) {
			// Join
			JOIN_KEY1(join_frame, result_buf->num_join) = key1;
			memcpy(JOIN_VALUE1(join_frame, result_buf->num_join), 
					LEAF_VALUE(leaf_node_1, index_1), SIZE_VALUE);
			JOIN_KEY2(join_frame, result_buf->num_join) = key2;			
			memcpy(JOIN_VALUE2(join_frame, result_buf->num_join),
					LEAF_VALUE(leaf_node_2, index_2), SIZE_VALUE);
			result_buf->num_join++;

			if (result_buf->num_join == 16) {
								
				for (i = 0; i < 16; i++) {
					fprintf(fp, "%" PRIu64",%s,%" PRIu64",%s\n",
							JOIN_KEY1(join_frame, i), JOIN_VALUE1(join_frame, i),
							JOIN_KEY2(join_frame, i), JOIN_VALUE2(join_frame, i));

					/*
					printf("%" PRIu64",%s,%" PRIu64",%s\n", 
							JOIN_KEY1(join_frame, i), JOIN_VALUE1(join_frame, i), 
							JOIN_KEY2(join_frame, i), JOIN_VALUE2(join_frame, i));	
					*/
		
				}
				
				result_buf->num_join = 0;
			}

			flag = move_advance(&buf1, &leaf_node_1, &index_1);
		}		

	}
	if (result_buf->num_join != 0) {
		
		for (i = 0; i < result_buf->num_join; i++) {
			fprintf(fp, "%" PRIu64",%s,%" PRIu64",%s\n",
							JOIN_KEY1(join_frame, i), JOIN_VALUE1(join_frame, i),
							JOIN_KEY2(join_frame, i), JOIN_VALUE2(join_frame, i));

			/*
			printf("%" PRIu64",%s,%" PRIu64",%s\n", 
					JOIN_KEY1(join_frame, i), JOIN_VALUE1(join_frame, i), 
					JOIN_KEY2(join_frame, i), JOIN_VALUE2(join_frame, i));
			*/
		}
		
		result_buf->num_join = 0;
	}

	buf1->pin_count--;
	buf2->pin_count--;
	result_buf->pin_count--;
	result_buf->page_offset = 0;
	fclose(fp);
	return 0;
}

Buffer* get_left_most_leaf_page(int table_id) {
	Buffer* buf;
	off_t offset;

	// Get header page
	buf = get_page_from_buffer(table_id, 0);
	offset = ((HeaderPage*)buf->frame)->root_offset;
	buf->pin_count--;

	// Get root page
	buf = get_page_from_buffer(table_id, offset);
	while ( ((NodePage*)buf->frame)->is_leaf != 1 ) {
		InternalPage* internal_node = (InternalPage*)buf->frame;
		buf->pin_count--;
		
		// Get internal page 
		offset = INTERNAL_OFFSET(internal_node, 0);
		buf = get_page_from_buffer(table_id, offset);
	}

	// Get leaf left most key value
	return buf;		// Warning : pin_count is stil 1.
}

int move_advance(Buffer** buf, LeafPage** leaf_node, int* index) {
	//LeafPage* leaf_node = (LeafPage*(*buf->frame)); 
	off_t sibling;
	/*
	// number of records per page : 31
	if (*index == BPTREE_LEAF_ORDER - 2) { /// wrong!
		sibling = (*leaf_node)->sibling;
		if (sibling != 0) {
			// There is right sibling, 
			// Change buf to right sibling page
			// and leaf_node to right sibling leaf node
			int buf_table_id = (*buf)->table_id;
			(*buf)->pin_count--;
			*buf = get_page_from_buffer(buf_table_id, sibling);
			*leaf_node = ((LeafPage*)(*buf)->frame);
			*index = 0;
			return 1;
		}
		else {
			return 0;
		}
	}
	*/

	if ((*leaf_node)->num_keys - 1 == *index) {
		sibling = (*leaf_node)->sibling;
		if (sibling != 0) {
			// There is right sibling, 
			// Change buf to right sibling page
			// and leaf_node to right sibling leaf node
			int buf_table_id = (*buf)->table_id;
			(*buf)->pin_count--;
			*buf = get_page_from_buffer(buf_table_id, sibling);
			*leaf_node = ((LeafPage*)(*buf)->frame);
			*index = 0;
			return 1;
		}
		else 
			return 0;
	}

	(*index)++;
	return 1;
}
