#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>

int main(int argc, char *argv[]) {
	FILE *fp;
	int offset;
	int bytes;
	char *buf;
	int size;

	if(argc != 4) {
		fprintf(stderr, "a.out <오프셋> <바이트 수> <파일명>\n");
		exit(1);
	}

	offset = atoi(argv[1]);
	bytes = atoi(argv[2]);

	if(offset < 0) {
		fprintf(stderr, "Error: <오프셋>이 음수가 입력됐습니다.\n");
		exit(1);
	}

	if((offset + bytes) < 0)
		bytes = 0 - offset;

	//동적할당, 바이트가 음수일 경우도 처리
	if(bytes < 0)
		buf = malloc(sizeof(char) * (1 - bytes));
	else
		buf = malloc(sizeof(char) * (bytes + 1));

	if((fp = fopen(argv[3], "r")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[3]);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	if(size < offset) {
		fprintf(stderr, "Error: <오프셋>이 파일 크기보다 큽니다.\n");
		exit(1);
	}


	//읽어들일 부분을 찾아간다. 바이트가 음수일 경우와 양수일 경우 처리
	if(bytes < 0) {
		fseek(fp, offset + bytes, SEEK_SET);
		bytes = 0 - bytes;	//fread하기 위해 양수로 바꿔준다. (읽어들일 바이트 수는 절댓값이기 때문에 읽기 시작하는 부분만 찾고 양수로 바꿔줌)
	}

	else 
		fseek(fp, offset+1, SEEK_SET);

	//읽어서 화면에 출력
	fread(buf, sizeof(char), bytes, fp);
	printf("%s", buf); 

	free(buf);
	fclose(fp);
	return 0;
}
