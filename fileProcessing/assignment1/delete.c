#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>

int main(int argc, char *argv[]) {
	FILE *fp;
	int offset;
	int bytes;
	char *buf;
	int count;		//파일에서 읽어들인 문자의 개수
	int size;	//삭제하고 남은 파일의 크기 계산할 변수

	if(argc != 4) {
		printf("a.out <오프셋> <바이트 수> <파일명>\n");
		exit(0);
	}

	if((fp = fopen(argv[3], "r+")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[3]);	
		exit(1);
	}

	//오프셋과 바이트 수 정수로 변환, 사이즈는 처음에 오프셋 만큼 넣어준다.
	offset = atoi(argv[1]);
	bytes = atoi(argv[2]);

	if(offset < 0) {
		fprintf(stderr, "Error: <오프셋>에 음수가 입력됐습니다.\n");
		exit(1);
	}

	if(bytes == 0)
		exit(0);

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	if(size < offset) {
		fprintf(stderr, "Error: <오프셋>이 파일 크기보다 큽니다.\n");
		exit(1);
	}
	
	if(size < (offset + bytes)) {
		bytes = size - offset - 1;
	}
	else if((offset + bytes) < 0) {
		bytes = 0 - offset;
	}

	if(bytes == 0)
		exit(0);

	//음수일 경우와 양수일 경우 동적할당과 삭제를 시작할 부분 찾기
	//삭제할 바이트 수는 음수든 양수든 동일하기 때문에
	//당겨오기 시작할 부분만 찾고 바이트를 양수로 바꿔준다.
	if(bytes < 0) {
		buf = malloc(sizeof(char) * (1 - bytes));
		fseek(fp, offset, SEEK_SET);
		bytes = 0 - bytes;
	}
	else {
		buf = malloc(sizeof(char) * (bytes + 1));
		fseek(fp, offset + bytes + 1, SEEK_SET);
	}
	
	while((count = fread(buf, sizeof(char), bytes, fp)) == bytes) {
		fseek(fp, 0 - (bytes + bytes), SEEK_CUR);	//당겨쓸 곳 찾아감
		fwrite(buf, sizeof(char), bytes, fp);	//덮어쓰기
		for(int i = 0; i < bytes + 1; i++)		//변수 초기화
			buf[i] = '\0';
		fseek(fp, bytes, SEEK_CUR);		//다시 읽을 부분 찾아감
	}
	fseek(fp, 0 - (count + bytes), SEEK_CUR);
	fwrite(buf, sizeof(char), count, fp);

	size -= bytes;

	ftruncate(fileno(fp), (off_t) size);	//파일 크기 바꿔서 eof 설정

	free(buf);
	fclose(fp);
	exit(0);
}
