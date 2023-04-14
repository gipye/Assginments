#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2;
	char buf[11];

	if(argc != 3) {
		fprintf(stderr, "a.out <원본파일명> <복사본파일명>\n");
		return 0;
	}

	if((fp1 = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[1]);
		exit(1);
	}

	if((fp2 = fopen(argv[2], "w")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[2]);
		exit(1);
	}

	//파일끝까지 원본파일에서 읽어서 복사본파일에 쓴다.
	while(!feof(fp1)) {
		for(int i = 0; i < 11; i++)		//변수 초기화
			buf[i] = '\0';
		fread(buf, sizeof(char), 10, fp1);
		fwrite(buf, sizeof(char), strlen(buf), fp2);	//마지막엔 읽은 만큼 써야 하므로 strlen()함수를 씀
	}

	fclose(fp1);
	fclose(fp2);

	return 0;
}
