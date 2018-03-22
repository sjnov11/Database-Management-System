#ifndef __BPT_H__
#define __BPT_H__

void usage_2(void);
int init_db(int num_buf);
int shutdown_db(void);
int open_table(const char* filename);
int close_table(int table_id);
int open_db(const char* filename);
char* find(int table_id, uint64_t key);
int insert(int table_id, uint64_t key, const char* value);
int delete(int table_id, uint64_t key);

void print_tree();
void find_and_print(int table_id, uint64_t key);
#endif // __BPT_H__
