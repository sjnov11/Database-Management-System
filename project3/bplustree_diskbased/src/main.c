#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include "bpt.h"
//#include "file.h"
#include "buffer.h"

// MAIN
int main( int argc, char ** argv ) {
	uint64_t input_key;
    char input_value[SIZE_VALUE];
	char instruction;
	int table_id;
	int t1, t2;

/*	
    license_notice();
	usage_1();  
	usage_2();
*/	

	init_db(16);
    table_id = open_table("DATA1");
	//printf("> ");
	while (scanf("%c", &instruction) != EOF) {
		switch (instruction) {
		case 'i':
			scanf("%" PRIu64 " %s", &input_key, input_value);
			insert(table_id, input_key, input_value);
			//print_tree(table_id);
			break;
        case 'd':
			scanf("%" PRIu64 "", &input_key);
			delete(table_id, input_key);
			//print_tree(table_id);
			break;
		case 'f':
			scanf("%" PRIu64 "", &input_key);
			find_and_print(table_id, input_key);
			fflush(stdout);
			break;
		case 'q':
			while (getchar() != (int)'\n');
			shutdown_db();
			close_db();
			return EXIT_SUCCESS;
			break;
		case 't':
			print_tree(table_id);
			break;
        default:
			usage_2();
			break;
		}
		while (getchar() != (int)'\n');
		//printf("> ");
	}
	printf("\n");

	shutdown_db();
    close_db();

	return EXIT_SUCCESS;
}
