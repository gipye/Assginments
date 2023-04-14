#include <stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<time.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>

//필요하면 header file 추가 가능

struct record {
	char name[196];
	int id;
};

//
// input parameters: 레코드 파일
//
int main(int argc, char **argv){
	int num_of_records;
	struct record students;
	FILE *fp;

	if(argc != 2) {
		fprintf(stderr, "<a.out> <filename>\n");
		exit(1);
	}

	if((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", argv[1]);
		exit(1);
	}

	//파일 크기를 이용해 record 개수를 구한다.
	fseek(fp, (off_t)0, SEEK_END);
	num_of_records = ftell(fp) / sizeof(struct record);

	//랜덤읽기와 같은 조건에서 비교하기 위해 만듦
	// 레코드 파일로부터 전체 레코드를 순차적으로 읽어들이고, 이때
	// 걸리는 시간을 측정하는 코드 구현
	//시간 측정 시작
	struct timeval start, end;
	gettimeofday(&start, 0);

	//랜덤읽기와 같은 조건에서 비교하기 위해 fseek는 사용하지 않아도 되지만 사용한다.
	for(int i = 0; i < num_of_records; i++) {
		fseek(fp, i*200, SEEK_SET);
		fread(students.name, 1, 196, fp);
		fread(&students.id, 4, 1, fp);
	}

	//측정 끝
	gettimeofday(&end, 0);
	long seconds = end.tv_sec - start.tv_sec;
	long microseconds = end.tv_usec - start.tv_usec;
	//마이크로 초는 빼면 음수가 나올 수 있기 때문에 처리해줍니다.
	if(microseconds < 0) {
		microseconds += 1000000;
		seconds--;
	}
	if(seconds > 0)
		printf("%ld%06ld usec\n", seconds, microseconds);
	else
		printf("%ld usec\n", microseconds);

	fclose(fp);
	return 0;
}
