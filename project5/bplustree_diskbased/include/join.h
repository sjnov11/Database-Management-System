#ifndef __JOIN_H__
#define __JOIN_H__

#include "buffer.h"

int join_table(int table_id_1, int table_id_2, char* pathname);
Buffer* get_left_most_leaf_page(int table_id);
int move_advance(Buffer** buf, LeafPage** leaf_node, int* index);
#endif
