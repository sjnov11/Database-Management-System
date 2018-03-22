#include <stddef.h>
#include <inttypes.h>

#define BPTREE_INTERNAL_ORDER       249
#define BPTREE_LEAF_ORDER           32

#define PAGE_SIZE                   4096

#define SIZE_KEY                    8
#define SIZE_VALUE                  120
#define SIZE_RECORD                 (SIZE_KEY + SIZE_VALUE)

#define BPTREE_MAX_NODE             (1024 * 1024) // for queue

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
typedef struct _Record {
    uint64_t key;
    char value[SIZE_VALUE];
} Record;

typedef struct _InternalRecord {
    uint64_t key;
    off_t offset;
} InternalRecord;

typedef struct _Page {
    char bytes[PAGE_SIZE];
    
    // in-memory data
    off_t file_offset;
} Page;

typedef struct _FreePage {
    off_t next;
    char reserved[PAGE_SIZE - 8];

    // in-memory data
    off_t file_offset;
} FreePage;

typedef struct _HeaderPage {
    off_t freelist;
    off_t root_offset;
    uint64_t num_pages;
    char reserved[PAGE_SIZE - 24];

    // in-memory data
    off_t file_offset;
} HeaderPage;

#define INTERNAL_KEY(n, i)    ((n)->irecords[(i)+1].key)
#define INTERNAL_OFFSET(n, i) ((n)->irecords[(i)].offset)
typedef struct _InternalPage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
            char reserved[112 - 16];			/// ????? why?
            InternalRecord irecords[BPTREE_INTERNAL_ORDER];
        };
        char space[PAGE_SIZE];
    };
    // in-memory data
    off_t file_offset;
} InternalPage;

#define LEAF_KEY(n, i)      ((n)->records[(i)].key)
#define LEAF_VALUE(n, i)    ((n)->records[(i)].value)
typedef struct _LeafPage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
            char reserved[120 - 16];
            off_t sibling;
            Record records[BPTREE_LEAF_ORDER-1];
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    off_t file_offset;
} LeafPage;

typedef struct _NodePage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    off_t file_offset;
} NodePage;

typedef struct _JoinRecord {	
	int64_t key1;
	char value1[120];
	int64_t key2;
	char value2[120];	
} JoinRecord;

#define JOIN_KEY1(n, i)		((n)->join_records[(i)].key1)
#define JOIN_KEY2(n, i)		((n)->join_records[(i)].key2)
#define JOIN_VALUE1(n, i)	((n)->join_records[(i)].value1)
#define JOIN_VALUE2(n, i)	((n)->join_records[(i)].value2)
typedef struct _JoinFrame {
	JoinRecord join_records[16];

	// in-memory data
	off_t file_offset;
} JoinFrame;

/*
// Table id with matching file descriptor
typedef struct _TableInfo {
	int table_id;
	int table_file;
	int is_alloc;
} TableInfo;
*/

// Open a db file. Create a file if not exist.
int open_db(const char* filename);

// Close a db file
void close_db();

// Get free page to use
off_t get_free_page(int table_id);

// Put free page to the free list
void put_free_page(int table_id, off_t page_offset);

// Expand file size and prepend pages to the free list
void expand_file(int table_id, size_t cnt_page_to_expand);

// Load file page into the in-memory page
void load_page(int table_id, off_t offset, Page* page);

// Flush page into the file
void flush_page(int table_id, Page* page);

extern HeaderPage dbheader;
//extern int unique_table_id;
//extern TableInfo table_pool[10];
