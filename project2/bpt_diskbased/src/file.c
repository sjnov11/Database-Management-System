#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>

#define FREE_PAGE_OFFSET	0
#define	ROOT_PAGE_OFFSET	8
#define	NUMBER_OF_PAGES		16

struct Record
{
	int64_t key;
	char value[120];
};

int main() {
	FILE *fp;
	int exist;
	int64_t temp;
	char temp2[120];
	struct Record ret;

	int64_t data;
	
	struct Record r;
	r.key = (int64_t)NULL;
	//strcpy(r.value, "\0");
	char* s ="teteetet";
	r.value = s;
	//r.value = NULL;

	exist = access("data.txt", 0);
	if (exist == -1) {
		fp = fopen("data.txt", "wb");
		
		fseek(fp, FREE_PAGE_OFFSET, SEEK_SET);
		printf("free page (%ld)\n", ftell(fp));
		data = (int64_t)-1;
		fwrite(&data, sizeof(int64_t), 1, fp);
		
		fseek(fp, ROOT_PAGE_OFFSET, SEEK_SET);
		printf("root page (%ld)\n", ftell(fp));
		data = (int64_t)5555;
//		fprintf(fp, "%"PRId64, (int64_t)5555);
		fwrite(&data, sizeof(int64_t), 1, fp);
		
		fseek(fp, NUMBER_OF_PAGES, SEEK_SET);
		printf("number pages (%ld)\n", ftell(fp));
//		fprintf(fp, "%"PRId64, (int64_t)222);
		data = (int64_t)222;
		fwrite(&data, sizeof(int64_t), 1, fp);
		
		fseek(fp, 128, SEEK_SET);
		printf("value (%ld)\n", ftell(fp));
		fwrite(&r, sizeof(r), 1, fp);
		
		fclose(fp);

	}
		
	else{
		fp = fopen("data.txt", "r");
		fseek(fp, NUMBER_OF_PAGES, SEEK_SET);
		printf("number pages (%ld)\n", ftell(fp));
		//fscanf(fp, "%"PRId64, &temp);
		fread(&temp, sizeof(temp), 1, fp);
		printf("number of pages : %"PRId64"\n", (int64_t)temp);
		
		fseek(fp, ROOT_PAGE_OFFSET, SEEK_SET);
		printf("root page (%ld)\n", ftell(fp));
		//fscanf(fp, "%"PRId64, &temp);
		fread(&temp, sizeof(int64_t), 1, fp);
		printf("root page offset : %"PRId64"\n", (int64_t)temp);

		fseek(fp, 128, SEEK_SET);
		fread(&ret, sizeof(ret), 1, fp);
		printf("(key, val) : %"PRId64", %s\n", ret.key, ret.value);
		printf("after read record (%ld)\n" ,ftell(fp));

		fseek(fp, 128 + 8, SEEK_SET);
		fread(&temp2, sizeof(temp2), 1, fp);
		printf("%s\n", temp2);

		printf("after read record (%ld)\n" ,ftell(fp));

	}
//		printf("file exists\n");

	return 0;
}
