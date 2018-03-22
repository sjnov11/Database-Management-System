#include <stdio.h>
#include <stdlib.h>

#define TEST 500
int main() {
	FILE * fp1, *fp2;

	int i;
	fp1 = fopen("op_insert.txt", "w+");
	fp2 = fopen("op_delete.txt", "w+");

	for (i =0; i <= TEST; i++) {
		fprintf(fp1, "i %d %d\n", i, i);
	}

	for (i = TEST; i>=0; i--) {
		fprintf(fp2, "d %d\n", i);
	}
	fclose(fp1);
	fclose(fp2);

}
