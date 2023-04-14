#include <stdio.h>
#include <sys/types.h>
#include<sys/time.h>
#include <time.h>
#include <stdlib.h>
//�ʿ��ϸ� header file �߰� ����


#define SUFFLE_NUM	10000	// �� ���� ������� ���� ����

struct record {
	char name[196];
	int id;
};
void GenRecordSequence(int *list, int n);
void swap(int *a, int *b);
// �ʿ��� �Լ��� ������ �� �߰��� �� ����

//
// input parameters: ���ڵ� ����
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

	//���� ũ�⸦ �̿��ؼ� �� ���ڵ� ���� �����Ѵ�.
	fseek(fp, (off_t)0, SEEK_END);
	num_of_records = ftell(fp) / sizeof(struct record);
	//���ڵ� ������ŭ �����Ҵ� ���ش�.
	read_order_list = (int *)malloc(sizeof(int)*num_of_records);

	// �� �Լ��� �����ϸ� 'read_order_list' �迭�� �о�� �� ���ڵ� ��ȣ���� �������
	// �����Ǿ� �����. 'num_of_records'�� ���ڵ� ���Ͽ� ����Ǿ� �ִ� ��ü ���ڵ��� ���� �ǹ���.
	GenRecordSequence(read_order_list, num_of_records);


	//�ð� ���� ����
	struct timeval start, end;
	gettimeofday(&start, 0);

	for(int i = 0; i < num_of_records; i++) {
		fseek(fp, read_order_list[i] * 200, SEEK_SET);
		fread(students.name, 1, 196, fp);
		fread(&students.id, 4, 1, fp);
	}

	//���� ��
	gettimeofday(&end, 0);
	long seconds = end.tv_sec - start.tv_sec;
	long microseconds = end.tv_usec - start.tv_usec;
	//����ũ�� �ʴ� ���� ������ ���� �� �ֱ� ������ ó�����ݴϴ�.
	if(microseconds < 0) {
		microseconds += 1000000;
		seconds--;
	}
	if(seconds > 0)
		printf("%ld%06ld usec\n", seconds, microseconds);
	else
		printf("%ld usec\n", microseconds);

	// 'read_order_list'�� �̿��Ͽ� ���ڵ� ���Ϸκ��� ��ü ���ڵ带 random �ϰ� �о���̰�,
	// �̶� �ɸ��� �ð��� �����ϴ� �ڵ� ����

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
