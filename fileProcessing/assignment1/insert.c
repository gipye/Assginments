#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>

int main(int argc, char *argv[]) {
	FILE *fp;
	char *buf;
	int offset, len;
	char *data;
	int size;

	if(argc != 4) {
		printf("a.out <오프셋> <데이터> <파일명>\n");
		exit(0);
	}

	offset = atoi(argv[1]);
	data = argv[2];

	if(offset < 0) {
		fprintf(stderr, "Error: <오프셋>에 음수가 입력됐습니다.\n");
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

	len = strlen(data);		//데이터 길이 저장
	buf = malloc(sizeof(char) * (len + 1));	//동적할당, 문자열 끝에 널문자가 들어가야 하므로 +1 해서 할당

	fseek(fp, offset+1, SEEK_SET);	//오프셋과 오프세 다음 문자 사이에 끼워 넣어야 하므로 오프셋 다음 문자를 가리킨다.


	//마지막 읽을 때까지 반복하면서 파일 내용을 뒤로 밀어낸다.
	while(fread(buf, sizeof(char), len, fp) == len) {
		fseek(fp, 0 - len, SEEK_CUR);
		fwrite(data, sizeof(char), len, fp);
		strcpy(data, buf);
		for(int i = 0; i < len + 1; i++)	//변수 초기화
			buf[i] = '\0';
	}

	//마지막에 파일 내용을 len보다 조금 읽었을 경우 처리
	fseek(fp, 0 - strlen(buf), SEEK_CUR);
	fwrite(data, sizeof(char), len, fp);
	fwrite(buf, sizeof(char), strlen(buf), fp);


	free(buf);
	fclose(fp);
	exit(0);
}	
