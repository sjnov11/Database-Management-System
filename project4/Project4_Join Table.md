# Join Table

#### Database System Management Project#4



## Object

Implement a **Join operation** with maintaing maximum memory usage. 
The system only uses a memory region that buffer manager maintains.



## Join Operation

**int join_table(int table_id_****1, int table_id_2, char* pathname)**

- Do natural join with given two tables.
- Write result table to the file using given pathname.
  - Result file format : "table\_1.key, table\_1.value, table\_2.key, table\_2.value"
  - The result should be sorted by the same key in increasing order.
- Return 0 if success, otherwise return non-zero value.
- Two tables should have been opened earlier.




Our disk based b+tree has a feature that the order of *key* is ***sorted*** and we want the join result sorted by the *key*.  So we can use **Sort-Merge Join** as the best method for the join operation. (Because it works for natural joins and it is especially good choice when inputs are sorted and output is required to be sorted.)



##Sort-Merge Join 

 The **Sort-Merge Join** is one kind of join opertation which works for equi-joins & natural joins.  It is especially good method when inputs are sorted or output is required to be sorted.

 The b+tree we implemented before already has the feature of sorted key which use for join attribute. So, the **Sort-Merge Join** we implement only need *Merge* stage.  Since each key using for join attribute is unique, we don't need to retrace record when we are done merge stage of that record. 

```c
while (done) {
  while (table_1.key < table_2.key) {
    // Move to next tuple.
    // If current is last, set done to false.
    done = advance(table_1);	    
    if (!done) break;
  }  	
  while (table_1.key > table_2.key) {
    // Move to next tuple.
    // If current is last, set done to false.
    done = advance(table_2);
  	if (!done) break;
  }
  if (table_1.key == table_2.key) {
    join_tuple(table_1.key, table_1.value, table_2.key, table_2.value);
    done = advance(table_1);
  }  
}
```



## Buffer Manager for Join Result

 While *join_table()* operates, the system only uses a memory region that buffer manager maintains. I modified buffer structure to support *join result frame* as well as *page frame*.

```c
typedef struct _Buffer {
	union {
		Page *frame;
		JoinFrame *join_frame;
	};
	union {
		int table_id;
		int num_join;			// the number of join records in join_frame
	};
	off_t page_offset;			// page offset of join result buffer is -1
	int is_dirty;
	int pin_count;
	int ref_bit;
} Buffer;
```

 The size of a join result record is 256KB, because size of a record is 128KB (key(8) + value(120)). Since size of buffer frame is 4096KB,  buffer frame can hold 16 join record maximum. 

```c
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
	// in-memory data (to have same structure as Page)
	off_t file_offset;
} JoinFrame;
```

 First, join results are written down to the buffer up to 16 times.

```c
// Write key and value of table_1 to the join frame
JOIN_KEY1(join_frame, result_buf->num_join) = key1;
memcpy(JOIN_VALUE1(join_frame, result_buf->num_join), 
LEAF_VALUE(leaf_node_1, index_1), SIZE_VALUE);
// Write key and value of table_2 to the join frame
JOIN_KEY2(join_frame, result_buf->num_join) = key2;			
memcpy(JOIN_VALUE2(join_frame, result_buf->num_join),
LEAF_VALUE(leaf_node_2, index_2), SIZE_VALUE);

result_buf->num_join++;
```

If the join buffer is full, then flush *join_frame* to the file.

```c
if (result_buf->num_join == 16) {							
	for (i = 0; i < 16; i++) {
		fprintf(fp, "%" PRIu64",%s,%" PRIu64",%s\n",
			JOIN_KEY1(join_frame, i), JOIN_VALUE1(join_frame, i),
			JOIN_KEY2(join_frame, i), JOIN_VALUE2(join_frame, i))		
	}
	result_buf->num_join = 0;
}
```

After join opeation, if there is join records in the join buffer, then flush to the file.

```c
if (result_buf->num_join != 0) {		
	for (i = 0; i < result_buf->num_join; i++) {
		fprintf(fp, "%" PRIu64",%s,%" PRIu64",%s\n",
				JOIN_KEY1(join_frame, i), JOIN_VALUE1(join_frame, i),
				JOIN_KEY2(join_frame, i), JOIN_VALUE2(join_frame, i));
	}
	
	result_buf->num_join = 0;
}
```









