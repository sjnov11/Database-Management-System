#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define DEFAULT_ORDER 4

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// Constants for file offsets
#define FREE_PAGE_OFFSET	0
#define ROOT_PAGE_OFFSET	8
#define	NUMBER_OF_PAGES		16

#define PARENT_PAGE_OFFSET	0
#define NEXT_FREE_PAGE_OFFSET   0
#define IS_LEAF				8
#define NUMBER_OF_KEYS		12
#define RIGHT_SIBLING_PAGE_OFFSET	120
#define ONE_MORE_PAGE_OFFSET	120

// PAGE size
#define PAGE_SIZE			4096

#define RECORD_SIZE			128
#define KEY_SIZE			8
#define VALUE_SIZE			120

#define BEFORE_ENTRY_SIZE	128
#define ENTRY_SIZE			16

// Branch factor
#define LEAF_PAGE_FACTOR	32
#define INTERNAL_PAGE_FACTOR	249
// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
struct Record {
    int64_t key;
	char value[120];
};

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */

typedef struct node {
	int64_t offset;
	struct node *next;
} node;

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
//extern int order;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
extern node * queue;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
//extern bool verbose_output;


// FUNCTION PROTOTYPES.

// Output and utility.
/*
void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );
void enqueue( node * new_node );
node * dequeue( void );
int height( node * root );
int path_to_root( node * root, node * child );
void print_leaves( node * root );
void print_tree( node * root );
void find_and_print(node * root, int key, bool verbose); 
void find_and_print_range(node * root, int range1, int range2, bool verbose); 
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[]); 
node * find_leaf( node * root, int key, bool verbose );
record * find( node * root, int key, bool verbose );
*/
void enqueue(node * new_node);
node dequeue(void);
int path_to_root(int64_t page_offset);
void print_tree();
int cut(int length);
int open_db(char* pathname);
char* find(int64_t key);
int64_t find_leaf(int64_t key);

// Helper Functions.

void move_to_free_page_offset_layer();
void move_to_root_page_offset_layer();
void move_to_number_of_pages_layer();
int64_t get_free_page_offset();
int64_t get_root_page_offset();
int64_t get_number_of_pages();

void move_to_parent_page_offset_layer(int64_t page_offset);
void move_to_is_leaf_layer(int64_t page_offset);
void move_to_number_of_keys_layer(int64_t page_offset);
int64_t get_parent_page_offset(int64_t page_offset);
int get_number_of_keys(int64_t page_offset);
int get_is_leaf(int64_t page_offset);

void move_to_right_sibling_page_offset_layer(int64_t leaf_ppage_offset);
void move_to_record_layer(int64_t leaf_page_offset, int order);
int64_t get_record_key(int64_t leaf_page_offset, int order);
//char* get_record_value(int64_t leaf_page_offset, int order, char* ret_val);
struct Record get_record(int64_t leaf_page_offset, int order);

void move_to_one_more_page_offset_layer(int64_t internal_page_offset);
void move_to_entry_layer(int64_t internal_page_offset, int order);
int64_t get_one_more_page_offset(int64_t internal_page_offset);
int64_t get_entry_key(int64_t internal_page_offset, int order);
int64_t get_entry_offset(int64_t internal_page_offset, int order);


// Insertion.
int64_t make_leaf_page();
int64_t make_internal_page();
int insert_into_leaf(int64_t leaf_page_offset, int64_t key, char* value);
int insert_into_leaf_after_splitting(int64_t leaf_page_offset, int64_t key, char* value);
int insert_into_internal(int64_t internal_page_offset, int left_index, 
				int64_t key, int64_t right_page_offset);
int insert_int_internal_after_splitting(int64_t internal_page_offset, int left_index,
				int64_t key, int64_t right_page_offset);
int get_left_index(int64_t parent_page_offset, int64_t left_leaf_page_offset);
int insert_into_new_root(int64_t left_page_offset, int64_t key, int64_t right_page_offset);
int insert_into_parent(int64_t left_page_offset, int64_t key, int64_t right_page_offset);
int start_new_tree(int64_t key, char* value);
int insert(int64_t key, char* value);


// Deletion.
int remove_entry_from_node(int64_t page_offset, int64_t key);
void add_free_page_offset(int64_t page_offset);
int adjust_root();
int get_neighbor_index(int64_t page_offset);
int coalesce_nodes(int64_t page_offset, int64_t neighbor_offset, 
        int neighbor_index, int64_t k_prime);
int redistrivute_nodes(int64_t page_offset, int64_t neighbor_offset, 
        int neighbor_index, int k_prime_index, int64_t k_prime);
int delete_entry(int64_t page_offset, int64_t key);
int delete(int64_t key);

/*
int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
        int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, int key, void * pointer );
node * delete( node * root, int key );

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);
*/
#endif /* __BPT_H__*/
