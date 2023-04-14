#include <stdio.h>
#include <sys/types.h>
#include<sys/time.h>
#include <time.h>
#include <stdlib.h>
//필요하면 header file 추가 가능


#define SUFFLE_NUM	10000	// 이 값은 마음대로 수정 가능

struct record {
	char name[196];
	int id;
};
void GenRecordSequence(int *list, int n);
void swap(int *a, int *b);
// 필요한 함수가 있으면 더 추가할 수 있음

//
// input parameters: 레코드 파일
//
int main(int argc, char **argv)
{
	int *read_order_list;
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

	//파일 크기를 이용해서 총 레코드 수를 저장한다.
	fseek(fp, (off_t)0, SEEK_END);
	num_of_records = ftell(fp) / sizeof(struct record);
	//레코드 개수만큼 동적할당 해준다.
	read_order_list = (int *)malloc(sizeof(int)*num_of_records);

	// 이 함수를 실행하면 'read_order_list' 배열에 읽어야 할 레코드 번호들이 순서대로
	// 나열되어 저장됨. 'num_of_records'는 레코드 파일에 저장되어 있는 전체 레코드의 수를 의미함.
	GenRecordSequence(read_order_list, num_of_records);


	//시간 측정 시작
	struct timeval start, end;
	gettimeofday(&start, 0);

	for(int i = 0; i < num_of_records; i++) {
		fseek(fp, read_order_list[i] * 200, SEEK_SET);
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

	// 'read_order_list'를 이용하여 레코드 파일로부터 전체 레코드를 random 하게 읽어들이고,
	// 이때 걸리는 시간을 측정하는 코드 구현

	free(read_order_list);
	fclose(fp);
	return 0;
}

void GenRecordSequence(int *list, int n)
{
	int i, j, k;

	srand((unsigned int)time(0));

	for(i=0; i<n; i++)
	{
		list[i] = i;
	}
	
	for(i=0; i<SUFFLE_NUM; i++)
	{
		j = rand() % n;
		k = rand() % n;
		swap(&list[j], &list[k]);
	}

	return;
}

void swap(int *a, int *b)
{
	int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;

	return;
}
