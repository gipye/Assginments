#include <stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
// 필요하면 header 파일 추가 가능
struct record {
	char name[196];
	int id;
};
//
// input parameters: 학생 레코드 수, 레코드 파일
//
int main(int argc, char **argv)
{
	char name[196] = "Ki Pyeong Park!!!!!!!!";	//record 구조체의 이름에 넣을 내용입니다.
	FILE *fp;
	int n;
	char c;

	//입력 파라미터 제대로 입력해야 실행
	if(argc != 3) {
		fprintf(stderr, "%s <number_of_records> <record_file_name>\n", argv[0]);
		exit(1);
	}

	//n의 값 정수로 저장
	if((n = atoi(argv[1])) == 0) {
		fprintf(stderr, "Error: <number> is wrong!!\n");
		exit(1);
	}

	//파일 오픈
	if((fp = fopen(argv[2], "w")) == NULL) {
		fprintf(stderr, "open error for %s\n", argv[2]);
		exit(1);
	}

	//n번 개의 학생 정보 파일에 저장
	for (int i = 1; i <= n; i++) {

		//학생 name는 "Ki Pyeong Park!!!!!!!"로 저장
		if(fwrite(name, 1, 196, fp) < 196) {
			fprintf(stderr, "write error\n");
			exit(1);
		}

		//학생 id는 1부터 n까지 순서대로 저장
		if(fwrite(&i, 4, 1, fp) < 1) {
			fprintf(stderr, "write error\n");
			exit(1);
		}
	}



	fclose(fp);
	// 사용자로부터 입력 받은 레코드 수 만큼의 레코드 파일을 생성하는 코드 구현
	
	// 파일에 '학생 레코드' 저장할 때 주의 사항
	// 1. 레코드의 크기는 무조건 200 바이트를 준수
	// 2. 레코드 파일에서 레코드와 레코드 사이에 어떤 데이터도 들어가면 안됨
	// 3. 레코드에는 임의의 데이터를 저장해도 무방
	// 4. 만약 n개의 레코드를 저장하면 파일 크기는 정확히 200 x n 바이트가 되어야 함

	return 0;
}
