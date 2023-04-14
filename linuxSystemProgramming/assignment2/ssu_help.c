#include<stdio.h>
#include<stdlib.h>

int main() {
	fprintf(stderr, "Usage:\n    > fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n\t>> [SET_INDEX] [OPTION ...]\n\t   [OPTION ...]\n\t   d [LIST_IDX]: delete [LIST_IDX] file\n\t   i: ask for confirmation before delete\n\t   f: force delete except the recently modified file\n\t   t: force move to Trash except the recently modified file\n    > help\n    > exit\n\n");
	exit(0);
}
