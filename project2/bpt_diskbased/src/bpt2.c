/*
 *  bpt.c  
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

#include "bpt.h"

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
//int order = DEFAULT_ORDER;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
node * queue = NULL;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
//bool verbose_output = false;

FILE* db;
char* buf;

// FUNCTION DEFINITIONS.

// UTILITIES

void enqueue(node * new_node) {
	node * c;
	if (queue == NULL) {
		queue = new_node;
		queue->next = NULL;
	}
	else {
		c = queue;
		while(c->next != NULL) {
			c = c->next;
		}
		c->next = new_node;
		new_node->next = NULL;
	}
}

node dequeue(void) {
	node ret;
	node * n = queue;
	queue = queue->next;
	ret.offset = n->offset;
	ret.next = NULL;
	//n->next = NULL;
	free(n);
	return ret;
}

int path_to_root(int64_t page_offset) {
	int length = 0;
	int64_t c = page_offset;
	int64_t root = get_root_page_offset();

	while (c != root) {
		c = get_parent_page_offset(c);
		length ++;
	}
	return length;
}

void print_tree() {

	node *root;
	node *temp;
	node  n;
	int i = 0;
	int rank = 0;
	int new_rank = 0;
	int64_t root_page_offset = get_root_page_offset();
	int64_t parent_offset;

	struct Record r;
	int64_t entry_key, entry_offset;

	if (root_page_offset == (int64_t)0) {
		printf("Empty tree.\n");
		return;
	}

	queue = NULL;
	
	root = malloc(sizeof(node));
	root->offset = root_page_offset;
	root->next = NULL;
	enqueue(root);
	while (queue != NULL) {
		n = dequeue();
		parent_offset = get_parent_page_offset(n.offset);
		if (parent_offset != (int64_t)0 && 
						n.offset == get_one_more_page_offset(parent_offset)) {
			new_rank = path_to_root(n.offset);
			if (new_rank != rank) {
				rank = new_rank;
				printf("\n");
			}
		}
		for (i = 0; i < get_number_of_keys(n.offset); i++) {
			if (get_is_leaf(n.offset) == 0) {
				entry_key = get_entry_key(n.offset, i);
				printf("%lld ", entry_key);
			}
			else if (get_is_leaf(n.offset) == 1) {
				r = get_record(n.offset, i);
				printf("(%lld, %s) ", r.key, r.value);
			}
		}
		if (get_is_leaf(n.offset) == 0) {
			// Left most
			entry_offset = get_one_more_page_offset(n.offset);
			temp = malloc(sizeof(node));
			temp->offset = entry_offset;
			temp->next = NULL;
			enqueue(temp);

			for (i = 0; i < get_number_of_keys(n.offset); i++) {
				entry_offset = get_entry_offset(n.offset, i);
				temp = malloc(sizeof(node));
				temp->offset = entry_offset;
				temp->next = NULL;
				enqueue(temp);
			}
		}
		printf("| ");

	}
	printf("\n");
}

int cut ( int length ) {
	if (length % 2 == 0)
		return length/2;
	else
		return length/2 + 1;
}

int open_db( char* pathname ) {
	// unistd.h
	// inttypes.h
	int exist = access(pathname, F_OK);
	int64_t data;

	// File exists
	if (exist == 0) {
		db = fopen(pathname, "r+b");		// r+ : open for reading/writing start at beginning
		if (db == NULL) {
			printf("open database file failed\n");
			return -1;
		}	
	}
	// File doesn't exist
	else {
		db = fopen(pathname, "w+b");		// w+ : open for reading/writing (overwrite)
		if (db == NULL) {
			printf("open database file failed\n");
			return -1;
		}
		// initialize header page	
		data = 0;
		fseek(db, FREE_PAGE_OFFSET, SEEK_SET);
		fwrite(&data, sizeof(data), 1, db);
		fseek(db, ROOT_PAGE_OFFSET, SEEK_SET);
		fwrite(&data, sizeof(data), 1, db);
		fseek(db, NUMBER_OF_PAGES, SEEK_SET);
		fwrite(&data, sizeof(data), 1, db);
		fflush(db);
	}	
}

char* find (int64_t key) {
	int i;
	int64_t c;
	struct Record r;

	c = find_leaf(key);
	if (c == (int64_t)-1) return NULL;
	for (i = 0; i < get_number_of_keys(c); i++)
		if (get_record_key(c, i) == key) break;
	if (i == get_number_of_keys(c))
		return NULL;
	else {
		r = get_record(c, i);
		buf = malloc(sizeof(char) * VALUE_SIZE);
		strcpy(buf, r.value);
		return buf;
	}

}

int64_t find_leaf (int64_t key) {
	int64_t c;
	int i;

	c = get_root_page_offset();

	if (c == (int64_t)0) {
		//printf("Empty\n");
		return (int64_t)-1;
	}

	while (get_is_leaf(c) == 0) {		// is internal page?
		i = 0;
		while (i < get_number_of_keys(c)) {
			if (key >= get_entry_key(c, i)) i++;
			else break;
		}

		if (i == 0) {
			c = get_one_more_page_offset(c);
		}
		else {
			c = get_entry_offset(c, i - 1);
		}
	}

	return c;
}


/************************************************/
/*				Helper Functions				*/
/************************************************/

// 1. For header page.

void move_to_free_page_offset_layer() {
	fseek(db, FREE_PAGE_OFFSET, SEEK_SET);
}
void move_to_root_page_offset_layer() {
	fseek(db, ROOT_PAGE_OFFSET, SEEK_SET);
}
void move_to_number_of_pages_layer() {
	fseek(db, NUMBER_OF_PAGES, SEEK_SET);
}
int64_t get_free_page_offset() {
	int64_t offset;
	move_to_free_page_offset_layer();
	fread(&offset, sizeof(offset), 1, db);
	return offset;
}
int64_t get_root_page_offset() {
	int64_t offset;
	move_to_root_page_offset_layer();
	fread(&offset, sizeof(offset), 1, db);
	return offset;
}
int64_t get_number_of_pages() {
	int64_t number;
	move_to_number_of_pages_layer();
	fread(&number, sizeof(number), 1, db);
	return number;
}

// 2. For common pages (except header page).
void move_to_parent_page_offset_layer(int64_t page_offset) {
	fseek(db, page_offset + PARENT_PAGE_OFFSET, SEEK_SET);
}
void move_to_is_leaf_layer(int64_t page_offset) {
	fseek(db, page_offset + IS_LEAF, SEEK_SET);
}
void move_to_number_of_keys_layer(int64_t page_offset) {
	fseek(db, page_offset + NUMBER_OF_KEYS, SEEK_SET);
}
int64_t get_parent_page_offset(int64_t page_offset) {
	int64_t parent_page_offset;
	move_to_parent_page_offset_layer(page_offset);
	fread(&parent_page_offset, sizeof(parent_page_offset), 1, db);
	return parent_page_offset;
}
int get_number_of_keys(int64_t page_offset) {
	int number_of_keys;
	move_to_number_of_keys_layer(page_offset);
	fread(&number_of_keys, sizeof(number_of_keys), 1, db);
	return number_of_keys;
}
int get_is_leaf(int64_t page_offset) {
	int is_leaf;
	move_to_is_leaf_layer(page_offset);
	fread(&is_leaf, sizeof(is_leaf), 1, db);
	return is_leaf;
}

// 3. For leaf pages only.
void move_to_right_sibling_page_offset_layer(int64_t leaf_page_offset) {
	fseek(db, leaf_page_offset + RIGHT_SIBLING_PAGE_OFFSET, SEEK_SET);
}
void move_to_record_layer(int64_t leaf_page_offset, int order) {
	fseek(db, leaf_page_offset + (RECORD_SIZE * (order + 1)), SEEK_SET);
}
int64_t get_record_key(int64_t leaf_page_offset, int order) {		
	// order begins at 0
	int64_t key;
	move_to_record_layer(leaf_page_offset, order);
	fread(&key, sizeof(key), 1, db);

	return key;
}
/*
char* get_record_value(int64_t leaf_page_offset, int order) {
	//char value[VALUE_SIZE];		// 120 : SIZE OF VALUE
	char* value = malloc(sizeof(char) * VALUE_SIZE);
	move_to_record_layer(leaf_page_offset, order);
	fseek(db, KEY_SIZE, SEEK_CUR);
	fread(&value, VALUE_SIZE, 1, db);
	//strcpy(ret_val, value);
	return value;
}*/
struct Record get_record(int64_t leaf_page_offset, int order) {
	struct Record record;
	move_to_record_layer(leaf_page_offset, order);
	fread(&record, sizeof(record), 1, db);

	return record;
}
int64_t get_right_sibling_page_offset(int64_t leaf_page_offset) {
	int64_t right_sibling_page_offset;
	move_to_right_sibling_page_offset_layer(leaf_page_offset);
	fread(&right_sibling_page_offset, sizeof(right_sibling_page_offset), 1, db);

	return right_sibling_page_offset;
}


// 4. For internal pages only
void move_to_one_more_page_offset_layer(int64_t internal_page_offset) {
	fseek(db, internal_page_offset + ONE_MORE_PAGE_OFFSET, SEEK_SET);
}
void move_to_entry_layer(int64_t internal_page_offset, int order) {
	fseek(db, internal_page_offset + BEFORE_ENTRY_SIZE + (ENTRY_SIZE * order), SEEK_SET);
}
void move_to_entry_offset_layer(int64_t internal_page_offset, int order) {
    fseek(db, internal_page_offset + BEFORE_ENTRY_SIZE +
            (ENTRY_SIZE * order) + KEY_SIZE, SEEK_SET);
}
int64_t get_one_more_page_offset(int64_t internal_page_offset) {
	int64_t left_most_page_offset;
	move_to_one_more_page_offset_layer(internal_page_offset);
	fread(&left_most_page_offset, sizeof(left_most_page_offset), 1, db);
	return left_most_page_offset;
}
int64_t get_entry_key(int64_t internal_page_offset, int order) {
	int64_t key;
	move_to_entry_layer(internal_page_offset, order);
	fread(&key, sizeof(key), 1, db);
	return key;
}
int64_t get_entry_offset(int64_t internal_page_offset, int order) {
	int64_t page_offset;
	move_to_entry_layer(internal_page_offset, order);
	fseek(db, KEY_SIZE, SEEK_CUR);
	fread(&page_offset, sizeof(page_offset), 1, db);
	return page_offset;
}

// 5. For free pages only
void move_to_next_free_page_offset_layer(int64_t free_page_offset) {
    fseek(db, free_page_offset + NEXT_FREE_PAGE_OFFSET, SEEK_SET);
}
int64_t get_next_free_page_offset(int64_t free_page_offset) {
    int64_t page_offset;
    move_to_nex_free_page_offset_layer(free_page_offset);
    fread(&page_offset, sizeof(page_offset), 1, db);
    return page_offset;
}


// Insertion.

int64_t make_leaf_page() {
	int64_t free_page_offset;
	int64_t number_of_pages;
	int64_t leaf_page_offset;

	int data;
	int64_t data_64;

	free_page_offset = get_free_page_offset();

	if (free_page_offset != 0) {
		// Allocate free page
	}
	else {
		number_of_pages = get_number_of_pages();
		leaf_page_offset = PAGE_SIZE * (number_of_pages + 1);

		// Initialize page header
		data_64 = (int64_t)0;
		move_to_parent_page_offset_layer(leaf_page_offset);
		fwrite(&data_64, sizeof(data_64), 1, db);

		data = 1;
		move_to_is_leaf_layer(leaf_page_offset);
		fwrite(&data, sizeof(data), 1, db);

		data = 0;
		move_to_number_of_keys_layer(leaf_page_offset);
		fwrite(&data, sizeof(data), 1, db);

		data_64 = (int64_t)0;
		move_to_right_sibling_page_offset_layer(leaf_page_offset);
		fwrite(&data_64, sizeof(data_64), 1, db);

		// Add 1 to number_of_page
		number_of_pages++;
		move_to_number_of_pages_layer();
		fwrite(&number_of_pages, sizeof(number_of_pages), 1, db);

		fflush(db);

		return leaf_page_offset;				
	}
}

int64_t make_internal_page() {
	int64_t free_page_offset;
	int64_t number_of_pages;
	int64_t internal_page_offset;

	int data;
	int64_t data_64;

	free_page_offset = get_free_page_offset();

	if (free_page_offset != 0) {

	}
	else {
		number_of_pages = get_number_of_pages();
		internal_page_offset = PAGE_SIZE * (number_of_pages + 1);
		
		// Initialize page header
		data_64 = (int64_t)0;
		move_to_parent_page_offset_layer(internal_page_offset);
		fwrite(&data_64, sizeof(data_64), 1, db);

		data = 0;
		move_to_is_leaf_layer(internal_page_offset);
		fwrite(&data, sizeof(data), 1, db);

		data = 0;
		move_to_number_of_keys_layer(internal_page_offset);
		fwrite(&data, sizeof(data), 1, db);
		
		data_64 = 0;
		move_to_one_more_page_offset_layer(internal_page_offset);
		fwrite(&data_64, sizeof(data_64), 1, db);

		// Add 1 to number_of_page
		number_of_pages++;
		move_to_number_of_pages_layer();
		fwrite(&number_of_pages, sizeof(number_of_pages), 1, db);

		fflush(db);

		return internal_page_offset;
	}
}

/* Inserts a new key and value
 * Leaf has enough space to insert.
 * No splitting.
 */
int insert_into_leaf(int64_t leaf_page_offset, int64_t key, char* value) {
	int number_of_keys, i, insertion_point;
	struct Record r;

	number_of_keys = get_number_of_keys(leaf_page_offset);

	insertion_point = 0;
	while (insertion_point < number_of_keys && 
					get_record_key(leaf_page_offset, insertion_point) < key) {
		insertion_point++;
	}

	for (i = number_of_keys; i > insertion_point; i--) {
		// Copy i-1 th record
		move_to_record_layer(leaf_page_offset, i - 1);
		fread(&r, sizeof(r), 1, db);
		// Paste to i th
		move_to_record_layer(leaf_page_offset, i);
		fwrite(&r, sizeof(r), 1, db);

	}

	// Insert new record value.
	move_to_record_layer(leaf_page_offset, insertion_point);
	r.key = key;
	strcpy(r.value, value);
	fwrite(&r, sizeof(r), 1, db);

	// number_of_keys
	number_of_keys++;
	move_to_number_of_keys_layer(leaf_page_offset);
	fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

	fflush(db);

	return 0;
}

/* Inserts a new key and value
 * to a new record into a leaf so as to exceed
 * the tree's order(factor), causing the leaf to be split
 * in half.
 */
int insert_into_leaf_after_splitting(int64_t leaf_page_offset, int64_t key, char* value) {
	int64_t new_leaf_page_offset, right_sibling_page_offset, parent_page_offset;
	struct Record* temp_record;
	struct Record insertion_record, garbage_record;
	int insertion_index, split, i, j, number_of_keys, data;
	int64_t new_key;

	new_leaf_page_offset = make_leaf_page();

	temp_record = malloc ( LEAF_PAGE_FACTOR * sizeof(struct Record) );
	if (temp_record == NULL) {
		printf("Temporary record error\n");
		return -1;
	}

	insertion_index = 0;
	while (insertion_index < LEAF_PAGE_FACTOR - 1 && 
					get_record_key(leaf_page_offset, insertion_index) < key) {
		insertion_index++;
	}

	// Save all record to temp_record 
	number_of_keys = get_number_of_keys(leaf_page_offset);
	for (i = 0, j = 0; i < number_of_keys; i++, j++) {
		if (j == insertion_index) j++;
		temp_record[j] = get_record(leaf_page_offset, i);
		//temp_record[j].key = get_record_key(leaf_page_offset, i);
		//strcpy(temp_record[j].value, get_record_value(leaf_page_offset, i));			
	}

	insertion_record.key = key;
	strcpy(insertion_record.value, value);
	temp_record[insertion_index] = insertion_record;
	
	//temp_record[insertion_index].key = key;
	//strcpy(temp_record[insertion_index].value, value);

	
	split = cut(LEAF_PAGE_FACTOR - 1);

	// Save first half of record to previous leaf page
	for (i = 0; i < split; i++) {
		move_to_record_layer(leaf_page_offset, i);
		fwrite(&temp_record[i], sizeof(temp_record[i]), 1, db);
	}

	data = split;
	move_to_number_of_keys_layer(leaf_page_offset);
	fwrite(&data, sizeof(data), 1, db);

	// Save last of record to new leaf page
	for (i = split, j = 0; i < LEAF_PAGE_FACTOR; i++, j++) {
		move_to_record_layer(new_leaf_page_offset, j);
		fwrite(&temp_record[i], sizeof(temp_record[i]), 1, db);
	}

	data = LEAF_PAGE_FACTOR - split;
	move_to_number_of_keys_layer(new_leaf_page_offset);
	fwrite(&data, sizeof(data), 1, db);


	// Set right siblings
	move_to_right_sibling_page_offset_layer(leaf_page_offset);
	fread(&right_sibling_page_offset, sizeof(right_sibling_page_offset), 1, db);
	
	move_to_right_sibling_page_offset_layer(leaf_page_offset);
	fwrite(&new_leaf_page_offset, sizeof(new_leaf_page_offset), 1 ,db);

	move_to_right_sibling_page_offset_layer(new_leaf_page_offset);
	fwrite(&right_sibling_page_offset, sizeof(right_sibling_page_offset), 1, db);

	fflush(db);

	// Put trash record to none used part
	garbage_record.key = (int64_t)NULL;
	strcpy(garbage_record.value, "\0");

    // LEAF_PAGE_FACTOR-1 ?
	for (i = get_number_of_keys(leaf_page_offset); i < LEAF_PAGE_FACTOR i++) {
		move_to_record_layer(leaf_page_offset, i);
		fwrite(&garbage_record, sizeof(garbage_record), 1, db);
	}
    // ?
	for (i = get_number_of_keys(new_leaf_page_offset); i < LEAF_PAGE_FACTOR; i++) {
		move_to_record_layer(new_leaf_page_offset, i);
		fwrite(&garbage_record, sizeof(garbage_record), 1, db);
	}

	move_to_parent_page_offset_layer(leaf_page_offset);
	fread(&parent_page_offset, sizeof(parent_page_offset), 1, db);

	move_to_parent_page_offset_layer(new_leaf_page_offset);
	fwrite(&parent_page_offset, sizeof(parent_page_offset), 1, db);

	fflush(db);
	free(temp_record);

	move_to_record_layer(new_leaf_page_offset, 0);
	fread(&new_key, sizeof(new_key), 1, db);

	return insert_into_parent(leaf_page_offset, new_key, new_leaf_page_offset);

}


/* Inserts a new key and page offset to a internal page
 *
 */
int insert_into_internal(int64_t internal_page_offset, int left_index, int64_t key,
					int64_t right_page_offset) {
	int i;
	int64_t temp_key, temp_offset;
	int number_of_keys;
	for (i = get_number_of_keys(internal_page_offset); i > left_index; i--) {
		// Read order i - 1 th entry
		move_to_entry_layer(internal_page_offset, i - 1);
		fread(&temp_key, sizeof(temp_key), 1, db);
		fread(&temp_offset, sizeof(temp_offset), 1, db);

		// Write to i th entry
		move_to_entry_layer(internal_page_offset, i);
		fwrite(&temp_key, sizeof(temp_key), 1, db);
		fwrite(&temp_offset, sizeof(temp_offset), 1, db);
	}

	// Insert new key and page offset to next to left index.
	// left_index is greater 1 than order.
	move_to_entry_layer(internal_page_offset, left_index);
	fwrite(&key, sizeof(key), 1, db);
	fwrite(&right_page_offset, sizeof(right_page_offset), 1, db);

	// Add 1 to number of keys.
	number_of_keys = get_number_of_keys(internal_page_offset);
	number_of_keys++;
	move_to_number_of_keys_layer(internal_page_offset);
	fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

	fflush(db);

	return 0;
}

/*	Insert a new key and page offset to a internal page
 *	Internal page size exceed the factor order,
 *	splitting into two.
 */
int insert_into_internal_after_splitting(int64_t internal_page_offset, int left_index,
				int64_t key, int64_t right_page_offset) {


	int64_t new_internal;
	int64_t * temp_keys;
	int64_t * temp_offsets;
	int i, j, split, number_of_keys;
	int64_t k_prime;
	int64_t temp_parent_offset, temp_child_offset;



	temp_keys = malloc( (INTERNAL_PAGE_FACTOR + 1) * sizeof(int64_t) );
	if (temp_keys == NULL) {
		printf("Temporary keys array for splitting internal pages\n");
		return -1;
	}

	temp_offsets = malloc( (INTERNAL_PAGE_FACTOR + 1) * sizeof(int64_t) );
	if (temp_offsets == NULL) {
		printf("Temporary offsets array for splitting internal pages\n");
		return -1;
	}

	for (i = 0, j = 0; i < get_number_of_keys(internal_page_offset); i++, j++) {
		if (j == left_index) j++;

		temp_keys[j + 1] = get_entry_key(internal_page_offset, i);
		temp_offsets[j + 1] = get_entry_offset(internal_page_offset, i);
		//move_to_entry_layer(internal_page_offset, i);
		//fread(&temp_keys[j + 1], sizeof(temp_keys[j + 1]), 1, db);
		//fread(&temp_offsets[j + 1], sizeof(temp_offsets[j + 1]), 1, db);
		//temp_key[j + 1] = key[i]
		//temp_offsets[j + 1] = offset[i]
	}
	
	// One more page offset
	//move_to_one_more_page_offset_layer(parent_page_offset);
	//fread(&temp_offsets[0], sizeof(temp_offsets[0]), 1, db);
	temp_offsets[0] = get_one_more_page_offset(internal_page_offset);

	// New key and offset
	temp_keys[left_index + 1] = key;
	temp_offsets[left_index + 1] = right_page_offset;

	/*	Create the new page and copy
	 *	half the keys and pointers to
	 *	the old an half to the new.
	 */
	
	split = cut(INTERNAL_PAGE_FACTOR);
	new_internal = make_internal_page();
	
	split --;
	for (i = 0; i < split; i++) {
		move_to_entry_layer(internal_page_offset, i);		// for safe
		fwrite(&temp_keys[i + 1], sizeof(temp_keys[i + 1]), 1, db);
		fwrite(&temp_offsets[i + 1], sizeof(temp_offsets[i + 1]), 1, db);
	}
	move_to_one_more_page_offset_layer(internal_page_offset);
	fwrite(&temp_offsets[0], sizeof(temp_offsets[0]), 1, db);

	number_of_keys = split;
	move_to_number_of_keys_layer(internal_page_offset);
	fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);
	
	k_prime = temp_keys[i + 1];
	
	move_to_one_more_page_offset_layer(new_internal);
	fwrite(&temp_offsets[i + 1], sizeof(temp_offsets[i + 1]), 1, db);
		
	for (i++, j = 0; i < INTERNAL_PAGE_FACTOR; i++, j++) {
		move_to_entry_layer(new_internal, j);
		fwrite(&temp_keys[i + 1], sizeof(temp_keys[i]), 1, db);
		fwrite(&temp_offsets[i + 1], sizeof(temp_offsets[i]), 1, db);
	}

	number_of_keys = INTERNAL_PAGE_FACTOR - split - 1;
	move_to_number_of_keys_layer(new_internal);
	fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

	fflush(db);
	free(temp_keys);
	free(temp_offsets);

	// old parent = new parent
	move_to_parent_page_offset_layer(internal_page_offset);
	fread(&temp_parent_offset, sizeof(temp_parent_offset), 1, db);
	move_to_parent_page_offset_layer(new_internal);
	fwrite(&temp_parent_offset, sizeof(temp_parent_offset), 1, db);

	// new_child -> parent = new internal
	temp_child_offset = get_one_more_page_offset(new_internal);
	move_to_parent_page_offset_layer(temp_child_offset);
	fwrite(&new_internal, sizeof(new_internal), 1, db);
	for (i = 0; i < get_number_of_keys(new_internal); i++) {
		temp_child_offset = get_entry_offset(new_internal, i);
		move_to_parent_page_offset_layer(temp_child_offset);
		fwrite(&new_internal, sizeof(new_internal), 1, db);
	}

	fflush(db);
	
	return insert_into_parent(internal_page_offset, k_prime, new_internal);

}

int insert_into_new_root(int64_t left_page_offset, int64_t key, int64_t right_page_offset) {
	int64_t root = make_internal_page();
	int64_t number_of_keys;
	

	move_to_one_more_page_offset_layer(root);
	//fwrite(&key, sizeof(key), 1, db);
	fwrite(&left_page_offset, sizeof(left_page_offset), 1, db);

	move_to_entry_layer(root, 0);
	fwrite(&key, sizeof(key), 1, db);
	fwrite(&right_page_offset, sizeof(right_page_offset), 1, db);

	number_of_keys = (int64_t)1;
	move_to_number_of_keys_layer(root);
	fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

	move_to_parent_page_offset_layer(left_page_offset);
	fwrite(&root, sizeof(root), 1, db);
	
	move_to_parent_page_offset_layer(right_page_offset);
	fwrite(&root, sizeof(root), 1, db);

	// This will be root page
	move_to_root_page_offset_layer();
	fwrite(&root, sizeof(root), 1, db);

	fflush(db);

	return 0;	
}

// Left index is order + 1. 
// Because internal page has one more page offset.
int get_left_index(int64_t parent_page_offset, int64_t left_leaf_page_offset) {
	int left_index = 0;
	
	if (get_one_more_page_offset(parent_page_offset) == left_leaf_page_offset)
		return left_index;

	left_index++;
	while (left_index <= get_number_of_keys(parent_page_offset) &&
			get_entry_offset(parent_page_offset, left_index - 1) != left_leaf_page_offset)
		left_index++;
	return left_index;
}


/* Inserts a new page(node, internal) to internal parent page
 * 
 */
int insert_into_parent(int64_t left_page_offset, int64_t key, int64_t right_page_offset) {
	int64_t parent_page_offset;
	int left_index;

	move_to_parent_page_offset_layer(left_page_offset);
	fread(&parent_page_offset, sizeof(parent_page_offset), 1, db);

	/* Case: new root. */
	if (parent_page_offset == (int64_t)0) 
		return insert_into_new_root(left_page_offset, key, right_page_offset);
	
	/* Case: leaf or node.
	 * (Remainder of function body.)
	 */
	
	/* Find the parent's pointer to the left node. */
	left_index = get_left_index(parent_page_offset, left_page_offset);

	/* The new key fits into the node. */
	if (get_number_of_keys(parent_page_offset) < INTERNAL_PAGE_FACTOR - 1)
		return insert_into_internal(parent_page_offset, left_index, key, right_page_offset);

	return insert_into_internal_after_splitting(parent_page_offset, left_index, 
				key, right_page_offset);

}

int start_new_tree(int64_t key, char* value) {
	int64_t leaf = make_leaf_page();
	
	// Set leaf page with key, value.
	insert_into_leaf(leaf, key, value);

	// Set root page offset on HEADER PAGE.
	move_to_root_page_offset_layer();
	fwrite(&leaf, sizeof(leaf), 1, db);
	fflush(db);

	return 0;
}

int insert (int64_t key, char* value) {
	int64_t root = get_root_page_offset();
	int64_t leaf;	
	char *s;

	// ## Should Implement find.
	//printf("%s\n", find(key));
	s = find(key);
	if(s != NULL) {
		//printf("%s\n", s);
		free(s);
		return -1;
	}
	free(s);

	if (root == (int64_t)0)
		return start_new_tree(key, value);

	leaf = find_leaf(key);

	if (get_number_of_keys(leaf) < LEAF_PAGE_FACTOR - 1)  {
		return insert_into_leaf(leaf, key, value);
	}
	// leaf must be split.
	return insert_into_leaf_after_splitting(leaf, key, value);
}

int remove_entry_from_node(int64_t page_offset, int64_t key) {
	int i, number_of_keys, is_leaf;
    struct Record r, garbage_record;
    int64_t entry_key, entry_offset, garbage_entry;
    int64_t free_page_offset;
    // page_offset = leaf page
	// Remove 

    is_leaf = get_is_leaf(page_offset);

    // Leaf page
    if (is_leaf == 1) {
    	i = 0;
        while (get_record_key(page_offset, i) != key) 
            i++;
        for (++i; i < get_number_of_keys(page_offset); i++) {
            r = get_record(page_offset, i);
            move_to_record_layer(page_offset, i - 1);
            fwrite(&r, sizeof(r), 1, db);
        }
    
        // One key fewer.
        number_of_keys = get_number_of_keys(page_offset);
        number_of_keys--;
        move_to_get_number_of_keys_layer(page_offset);
        fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);
    
    	// Put trash record to none used part
    	garbage_record.key = (int64_t)NULL;
    	strcpy(garbage_record.value, "\0");
    
        for (i = number_of_keys; i < LEAF_PAGE_FACTOR; i++) {
            move_to_record_layer(page_offset, i);
            fwrite(&garbage_record, sizeof(garbage_record), 1, db);
        }
    }
    // Internal page
    else {
        i = 0;
        while (get_entry_key(page_offset, i) != key)
            i++;
        for (++i; i < get_number_of_keys(page_offset); i++) {
            entry_key = get_entry_key(page_offset, i);
            entry_offset = get_entry_offset(page_offset, i);
            move_to_entry_layer(page_offset, i - 1);
            fwrite(&entry_key, sizeof(entry_key), 1, db);
            fwrite(&entry_offset, sizeof(entry_offset), 1, db);
        }

        // One key fewer.
        number_of_keys = get_number_of_keys(page_offset);
        number_of_keys--;
        move_to_get_number_of_keys_layer(page_offset);
        fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

        // Put trash entry to none used part
        garbage_entry = (int64_t)0;

        for (i = number_of_keys; i < INTERNAL_PAGE_FACTOR; i++) {
            move_to_record_layer(page_offset, i);
            fwrite(&garbage_entry, sizeof(garbage_entry), 1, db);
        }
    }

   
    // Set Free Page offset
    if (number_of_keys == 0) {
        add_free_page_offset(page_offset);
    }

    fflush(db);

            
    return 0;

}

void add_free_page_offset(int64_t page_offset){
    // Set Free Page offset
    free_page_offset = get_free_page_offset();
    if (free_page_offset == 0) {
        move_to_free_page_offset();
        fwrite(&page_offset, sizeof(page_offset), 1, db);
    }
    else {
        while (1) {
            free_page_offset = get_next_free_page_offset(free_page_offset);
            if (free_page_offset == 0)
                break;                
        }
        fseek(db, NEXT_FREE_PAGE_OFFSET * (-1), SEEK_CUR);
        fwrite(&page_offset, sizeof(page_offset), 1, db);
    }

}

int adjust_root() {
    int64_t root = get_root_page_offset();
    int64_t new_root;
    int64_t temp;
    // Case: nonempty root.
    if (get_number_of_keys(root) > 0)
        return 0;
    
    // Case: empty root.

    if (get_is_leaf(root) == 0) {
        new_root = get_one_more_page_offset(page_offset);
        move_to_parent_page_offset_layer(new_root);
        temp = (int64_t)0;
        fwrite(&temp, sizeof(temp), 1, db);
        move_to_root_page_offset();
        fwrite(&new_root, sizeof(new_root), 1, db);
    }
    else {       
        new_root = (int64_t)0;
        move_to_root_page_offset();
        fwrite(&new_root, sizeof(new_root), 1, db);
        
    }

    fflush(db);

    return 0;
}

int get_neighbor_index(int64_t page_offset) {
    int i;
    int64_t parent;

    parent = get_parent_page_offset(page_offset);

    if (get_one_more_page_offset(parent) == page_offset) 
        return -1;
    for (i = 0; i < get_number_of_keys(parent); i++) {
       if(get_entry_offset(parent, i) == page_offset)
           return i;
    }
}

int coalesce_nodes(int64_t page_offset, int64_t neighbor_offset, 
        int neighbor_index, int64_t k_prime) {
    
    int i, j, neighbor_insertion_index, n_end;
    int64_t temp;
    int64_t entry_key, entry_offset;
    int number_of_keys_neighbor;
    struct Record r;


    if (neighbor_index == -1) {
        temp = page_offset;
        page_offset = neighbor_offset;
        neighbor_offset = temp;
    }
 
    neighbor_insertion_index = get_number_of_keys(neighbor_offset);

    if (get_is_leaf(page_offset) == 0) {
        number_of_keys_neighbor = get_number_of_keys(neighbor_offset);

        entry_offset = get_one_more_page_offset(page_offset);
        move_to_entry_layer(&neighbor_offset, neighbor_insertion_index);
        fwrite(&k_prime, sizeof(k_prime), 1, db);
        fwrite(&entry_offset, sizeof(entry_offset), 1, db);
        number_of_keys_neighbor++;

        n_end = get_number_of_keys(page_offset);

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            entry_key = get_entry_key(page_offset, j);
            entry_offset = get_entry_offset(page_offset, j);
            move_to_entry_layer(neighbor_offset, i);
            fwrite(&entry_key, sizeof(entry_key), 1, db);
            fwrite(&entry_offset, sizeof(entry_offset), 1, db);
            number_of_keys_neighbor++;
        }

        move_to_number_of_keys_layer(neighbor_offset);
        fwrite(&number_of_keys_neighbor, sizeof(number_of_keys_neighbor), 1, db);

        temp = get_one_more_page_offset(neighbor_offset);
        move_to_parent_page_offset_layer(temp);
        fwrite(&neighbor_offset, sizeof(neighbor_offset), 1, db);

        for (i = 0; i < get_number_of_keys(neighbor_offset); i++) {
            temp = get_entry_offset(neighbor_offset, i);
            move_to_parent_page_offset_layer(temp);
            fwrite(&neighbor_offset, sizeof(neighbor_offset), 1, db);
        }
    }
    else {
        number_of_keys_neighbor = get_number_of_keys(neighbor_keys);
        for (i = neighbor_insertion_index, j = 0; j < get_number_of_keys(page_offset);
                i++, j++) {
            r = get_record(page_offset, j);
            move_to_record_layer(neighbor_offset, neighbor_insertion_index);
            fwrite(&r, sizeof(r), 1, db);
            numbuer_of_keys_neighbor++;
        }
        move_to_number_of_keys_layer(neighbor_offset);
        fwrite(&number_of_keys_neighbor, sizeof(number_of_keys_neighbor), 1, db);

        temp = get_right_sibling_page_offset(page_offset);
        move_to_right_sibling_page_offset_layer(neighbor_offset);
        fwrite(&temp, sizeof(temp), 1, db);
    }

    delete_entry(get_parent_page_offset(page_offset), k_prime);
    // Free page  ??? is this ok?
    add_free_page_offset(page_offset);
    
    fflush(db);

    return 0;
}

int redistribute_nodes(int64_t page_offset, int64_t neighbor_offset, int neighbor_index,
        int k_prime_index, int k_prime) {

    int i;
    int64_t temp;
    int64_t entry_key, entry_offset;
    int64_t parent;
    struct Record r;
    int64_t number_of_keys;

    if (neighbor_index != -1) {
        
        if (get_is_leaf(page_offset) == 0) {
            for (i = get_number_of_keys(page_offset); i > 0; i--) {            
                entry_key = get_entry_key(page_offset, i - 1);
                entry_offset = get_entry_offset(page_offset, i - 1);
                move_to_entry_layer(page_offset, i);
                fwrite(&entry_key, sizeof(entry_key), 1, db);
                fwrite(&entry_offset, sizeof(entry_offset), 1, db);
            } 
            entry_offset = get_one_more_page_offset(page_offset);
            move_to_entry_layer(page_offset, 0);
            fwrite(&k_prime, sizeof(k_prime), 1, db);
            fwrite(&entry_offset, sizeof(entry_offset), 1, db);

            entry_key = get_entry_key(neighbor_offset, get_number_of_keys(neighbor_offset) - 1);
            entry_offset = get_entry_key(neighbor_offset, get_number_of_keys(neigbor_offset)-1);
            
            parent = get_parent_page_offset(page_offset);
            move_to_entry_layer(parent, k_prime_index);
            fwrite(&entry_key, sizeof(entry_key), 1, db);

            move_to_one_more_page_offset_layer(page_offset);
            fwrite(&entry_offset, sizeof(entry_offset), 1, db);

            // change page's parent
            move_to_parent_page_offset_layer(entry_offset);
            fwrite(&page_offset, sizeof(page_offset), 1, db);
        }
        else {
            for (i = get_number_of_keys(page_offset); i > 0; i--) {
                r = get_record(page_offset);
                move_to_record_layer(page_offset, i);
                fwrite(&r, sizeof(r), 1, db);
            }
            r = get_record(neighbor_offset, get_number_of_keys(neighbor_offset) - 1);
            move_to_record_layer(page_offset, 0);
            fwrite(&r, sizeof(r), 1, db);

            entry_key = get_record_key(page_offset, 0);
            parent = get_parent_offset(page_offset);
            move_to_entry_layer(parent, k_prime_index);
            fwrite(&entry_key, sizeof(entry_key), 1, db);

        }       
        
    }

    else {
        if (get_is_leaf(page_offset) == 1) {
            r = get_record(neighbor_offset, 0);
            move_to_record_layer(page_offset, get_number_of_keys(page_offset));
            fwrite(&r, sizeof(r), 1, db);
            entry_key = get_record_key(neighbor_offset, 1);
            parent = get_parent_page_offset(page_offset);
            move_to_entry_layer(parent, k_prime_index);
            fwrite(&entry_key, sizeof(entry_key), 1, db);

            for (i = 0; i < get_number_of_keys(neighbor_offset) - 1; i++) {
                r = get_record(neighbor_offset, i + 1);
                move_to_record_layer(neighbor_offset, i);
                fwrite(&r, sizeof(r), 1, db);
            }
        }
        else {
            entry_offset = get_one_more_page_offset(neighbor_offset);
            move_to_entry_layer(page_offset, get_number_of_keys(page_offset));
            fwrite(&k_prime, sizeof(k_prime), 1, db);
            fwrite(&entry_offset, sizeof(entry_offset), 1, db);

            move_to_parent_page_offset_layer(entry_offset);
            fwrite(&page_offset, sizeof(page_offset), 1, db);

            parent = get_parent_page_offset(page_offset);
            entry_key = get_entry_key(neighbor_offset, 0);
            move_to_entry_offset_layer(parent, k_prime_index);
            fwrite(&entry_key, sizeof(entry_key), 1, db);

            entry_offset = get_entry_offset(neighbor_offset, 0);
            move_to_one_more_page_offset_layer(neighbor_offset);
            fwrite(&entry_offset, sizeof(entry_offset), 1, db);

            for (i = 0; i < get_number_of_keys - 1; i++) {
                entry_key = get_entry_key(neighbor_offset, i + 1);
                entry_offset = get_entry_offset(neighbor_offset, i + 1);
                move_to_entry_layer(neighbor_offset, i);
                fwrite(&entry_key, sizeof(entry_key), 1, db);
                fwrite(&entry_offset, sizeof(entry_offset), 1, db);
            }


        }
    }

    number_of_keys = get_number_of_keys(page_offset);
    number_of_keys++;
    move_to_number_of_keys_layer(page_offset);
    fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

    number_of_keys = get_number_of_keys(neighbor_offset);
    number_of_keys--;
    move_to_number_of_keys_layer(neighbor_offset);
    fwrite(&number_of_keys, sizeof(number_of_keys), 1, db);

    fflush(db);

    return 0;

}

int delete_entry(int64_t page_offset, int64_t key) {
	
	int64_t root;
	int64_t k_prime;
    int64_t neighbor_offset;
	int k_prime_index, neighbor_index;
	int min_keys;
	int capacity;
	
	if (remove_entry_from_node(page_offset, key) != 0) return -1;

	root = get_root_page_offset();

	if(page_offset == root)
		return adjust_root();

    if (get_is_leaf(page_offset) == 1)
        min_keys = cut(LEAF_PAGE_FACTOR - 1);
    else 
        min_keys = cut(INTERNAL_PAGE_FACTOR) - 1;

    if (get_number_of_keys(page_offset) > min_keys)
        return 0;
    
    // Left most(one_more_page)'s neighbor : -1     
    neighbor_index = get_neighbor_index(page_offset);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = get_entry_key(parent, k_prime_index);


    if (neighbor_index == -1) 
        neighbor_offset = get_entry_offset(parent, 0);
    else if (neighbor_index == 0)
        neighbor_offset = get_one_more_page_offset(parent);
    else
        neighbor_offset = get_entry_offset(parent, neighbor_index - 1);

    capacity = get_is_leaf(page_offset) == 1 ? LEAF_PAGE_FACTOR : INTERNAL_PAGE_FACTOR - 1;


    if (get_number_of_keys(neighbor_offset) + get_number_of_keys(page_offset) < capacity)
        return coalesce_nodes(page_offset, neighbor_offset, neighbor_index, k_prime);
    else
        return redistribute_nodes(page_offset, neighbor_offset, neighbor_index, k_prime);

}

int delete(int64_t key) {
	key_leaf = find_leaf(key);
	
	if (key_leaf != (int64_t)-1) {
		delete_entry(key_leaf, key);
	}

	return 0;
}
