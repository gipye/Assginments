#include <stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
// �ʿ��ϸ� header ���� �߰� ����
struct record {
	char name[196];
	int id;
};
//
// input parameters: �л� ���ڵ� ��, ���ڵ� ����
//
int main(int argc, char **argv)
{
	char name[196] = "Ki Pyeong Park!!!!!!!!";	//record ����ü�� �̸��� ���� �����Դϴ�.
	FILE *fp;
	int n;
	char c;

	//�Է� �Ķ���� ����� �Է��ؾ� ����
	if(argc != 3) {
		fprintf(stderr, "%s <number_of_records> <record_file_name>\n", argv[0]);
		exit(1);
	}

	//n�� �� ������ ����
	if((n = atoi(argv[1])) == 0) {
		fprintf(stderr, "Error: <number> is wrong!!\n");
		exit(1);
	}

	//���� ����
	if((fp = fopen(argv[2], "w")) == NULL) {
		fprintf(stderr, "open error for %s\n", argv[2]);
		exit(1);
	}

	//n�� ���� �л� ���� ���Ͽ� ����
	for (int i = 1; i <= n; i++) {

		//�л� name�� "Ki Pyeong Park!!!!!!!"�� ����
		if(fwrite(name, 1, 196, fp) < 196) {
			fprintf(stderr, "write error\n");
			exit(1);
		}

		//�л� id�� 1���� n���� ������� ����
		if(fwrite(&i, 4, 1, fp) < 1) {
			fprintf(stderr, "write error\n");
			exit(1);
		}
	}



	fclose(fp);
	// ����ڷκ��� �Է� ���� ���ڵ� �� ��ŭ�� ���ڵ� ������ �����ϴ� �ڵ� ����
	
	// ���Ͽ� '�л� ���ڵ�' ������ �� ���� ����
	// 1. ���ڵ��� ũ��� ������ 200 ����Ʈ�� �ؼ�
	// 2. ���ڵ� ���Ͽ��� ���ڵ�� ���ڵ� ���̿� � �����͵� ���� �ȵ�
	// 3. ���ڵ忡�� ������ �����͸� �����ص� ����
	// 4. ���� n���� ���ڵ带 �����ϸ� ���� ũ��� ��Ȯ�� 200 x n ����Ʈ�� �Ǿ�� ��

	return 0;
}
