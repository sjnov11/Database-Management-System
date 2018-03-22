#include <stdio.h>


char* get_value() {
	char s[10] = "aaaaa";
	return s;
}

int main() {
	char* ret;
	ret = "asdf";
	printf("%s\n", ret);

}
