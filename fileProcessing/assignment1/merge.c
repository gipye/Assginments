#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>

int main(int argc, char *argv[]) {
	FILE *fp, *mfp;
	char buf[11];

	if(argc != 4) {
		printf("a.out <파일명1> <파일명2> <파일명3>\n");
		exit(1);
	}

	if((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[1]);
		exit(1);
	}

	if((mfp = fopen(argv[3], "w")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[3]);
		exit(1);
	}

	//첫 번째 파일을 먼저 쓴다.
	while(!feof(fp)) {
		for(int i = 0; i < 11; i++)
			buf[i] = '\0';
		fread(buf, sizeof(char), 10, fp);
		fwrite(buf, sizeof(char), strlen(buf), mfp);
	}


	//첫 번째 파일 닫고 두 번째 파일 연다.
	fclose(fp);
	if((fp = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "file open error for %s\n", argv[2]);
		exit(1);
	}

	//두 번째 파일을 이어서 쓴다.
	while(!feof(fp)) {
		for(int i = 0; i < 11; i++)
			buf[i] = '\0';
		fread(buf, sizeof(char), 10, fp);
		fwrite(buf, sizeof(char), strlen(buf), mfp);
	}

	fclose(fp);
	fclose(mfp);
	return 0;
}
