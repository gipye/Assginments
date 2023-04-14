#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>

int main(int argc, char *argv[]) {
	FILE *fp;
	int offset;
	char *data;
	int size;

	if(argc != 4) {
		printf("a.out <오프셋> <데이터> <파일명>\n");
		return 0;
	}

	offset = atoi(argv[1]);
	data = argv[2];

	if(offset < 0) {
		fprintf(stderr, "Error: <오프셋>이 음수가 입력됐습니다.\n");
		exit(1);
	}

	if((fp = fopen(argv[3], "r+")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[3]);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	if(size < offset) {
		fprintf(stderr, "Error: <오프셋>이 파일 크기보다 큽니다.\n");
		exit(1);
	}

	//오프셋으로 덮어 쓸 부분을 찾아가서 쓴다.
	fseek(fp, offset, SEEK_SET);
	fwrite(data, sizeof(char), strlen(data), fp);

	fclose(fp);
	return 0;
}
