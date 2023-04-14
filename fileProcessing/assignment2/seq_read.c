#include <stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<time.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>

//�ʿ��ϸ� header file �߰� ����

struct record {
	char name[196];
	int id;
};

//
// input parameters: ���ڵ� ����
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

	//���� ũ�⸦ �̿��� record ������ ���Ѵ�.
	fseek(fp, (off_t)0, SEEK_END);
	num_of_records = ftell(fp) / sizeof(struct record);

	//�����б�� ���� ���ǿ��� ���ϱ� ���� ����
	// ���ڵ� ���Ϸκ��� ��ü ���ڵ带 ���������� �о���̰�, �̶�
	// �ɸ��� �ð��� �����ϴ� �ڵ� ����
	//�ð� ���� ����
	struct timeval start, end;
	gettimeofday(&start, 0);

	//�����б�� ���� ���ǿ��� ���ϱ� ���� fseek�� ������� �ʾƵ� ������ ����Ѵ�.
	for(int i = 0; i < num_of_records; i++) {
		fseek(fp, i*200, SEEK_SET);
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

	fclose(fp);
	return 0;
}
