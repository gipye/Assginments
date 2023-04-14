#include <stdio.h>		// �ʿ��� header file �߰� ����
#include <string.h>
#include <stdlib.h>
#include "student.h"

void pack(char *recordbuf, const STUDENT *s);
void unpack(const char *recordbuf, STUDENT *s);
void printRecord(const STUDENT *s);
//
// �Լ� readRecord()�� �л� ���ڵ� ���Ͽ��� �־��� rrn�� �ش��ϴ� ���ڵ带 �о 
// recordbuf�� �����ϰ�, ���� unpack() �Լ��� ȣ���Ͽ� �л� Ÿ���� ������ ���ڵ���
// �� �ʵ尪�� �����Ѵ�. �����ϸ� 1�� �׷��� ������ 0�� �����Ѵ�.
// unpack() �Լ��� recordbuf�� ����Ǿ� �ִ� record���� �� field�� �����ϴ� ���� �Ѵ�.
//
int readRecord(FILE *fp, STUDENT *s, int rrn)
{
	char recordbuf[RECORD_SIZE+1];
	memset(recordbuf, 0, sizeof(recordbuf));

	fseek(fp, 8+(rrn*RECORD_SIZE), SEEK_SET);
	if(fread(recordbuf, 85, 1, fp) == 0)
		return 0;

	unpack(recordbuf, s);

	return 1;
}
void unpack(const char *recordbuf, STUDENT *s)
{
	int i = 0, j;

	j = 0;
	while(recordbuf[i] != '#') {
		(s->id)[j] = recordbuf[i];
		i++;
		j++;
	}
	(s->id)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(s->name)[j] = recordbuf[i];
		i++;
		j++;
	}
	(s->name)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(s->dept)[j] = recordbuf[i];
		i++;
		j++;
	}
	(s->dept)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(s->addr)[j] = recordbuf[i];
		i++;
		j++;
	}
	(s->addr)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(s->email)[j] = recordbuf[i];
		i++;
		j++;
	}
	(s->email)[j] = '\0';

}

//
// �Լ� writeRecord()�� �л� ���ڵ� ���Ͽ� �־��� rrn�� �ش��ϴ� ��ġ�� recordbuf�� 
// ����Ǿ� �ִ� ���ڵ带 �����Ѵ�. ������ pack() �Լ��� ȣ���Ͽ� recordbuf�� �����͸� ä�� �ִ´�.
// ���������� �����ϸ� '1'��, �׷��� ������ '0'�� �����Ѵ�.
//
int writeRecord(FILE *fp, const STUDENT *s, int rrn)
{
	char recordbuf[RECORD_SIZE+1];
	memset(recordbuf, 0, sizeof(recordbuf));

	pack(recordbuf, s);

	fseek(fp, 8+(rrn*RECORD_SIZE), SEEK_SET);
	return fwrite(recordbuf, 85, 1, fp);
}

void pack(char *recordbuf, const STUDENT *s)
{
	strcpy(recordbuf, s->id);
	strcat(recordbuf, "#");
	strcat(recordbuf, s->name);
	strcat(recordbuf, "#");
	strcat(recordbuf, s->dept);
	strcat(recordbuf, "#");
	strcat(recordbuf, s->addr);
	strcat(recordbuf, "#");
	strcat(recordbuf, s->email);
	strcat(recordbuf, "#");
}

//
// �Լ� appendRecord()�� �л� ���ڵ� ���Ͽ� ���ο� ���ڵ带 append�Ѵ�.
// ���ڵ� ���Ͽ� ���ڵ尡 �ϳ��� �������� �ʴ� ��� (ù ��° append)�� header ���ڵ带
// ���Ͽ� �����ϰ� ù ��° ���ڵ带 �����Ѵ�. 
// �翬�� ���ڵ带 append�� �� ������ header ���ڵ忡 ���� ������ �ڵ���� �Ѵ�.
// �Լ� appendRecord()�� ���������� writeRecord() �Լ��� ȣ���Ͽ� ���ڵ� ������ �ذ��Ѵ�.
// ���������� �����ϸ� '1'��, �׷��� ������ '0'�� �����Ѵ�.
//
int appendRecord(FILE *fp, char *id, char *name, char *dept, char *addr, char *email)
{
	int rrn, nulll = 0;
	STUDENT student;

	strcpy(student.id, id);
	strcpy(student.name, name);
	strcpy(student.dept, dept);
	strcpy(student.addr, addr);
	strcpy(student.email, email);

	if(!fread(&rrn, 4, 1, fp))
		rrn = -1;

	rrn++;
	fseek(fp, 0, SEEK_SET);
	fwrite(&rrn, 4, 1, fp);
	fwrite(&nulll, 4, 1, fp);

	return writeRecord(fp, &student, rrn);
}

//
// �л� ���ڵ� ���Ͽ��� �˻� Ű���� �����ϴ� ���ڵ尡 �����ϴ����� sequential search ����� 
// ���� ã�Ƴ���, �̸� �����ϴ� ��� ���ڵ��� ������ ����Ѵ�. �˻� Ű�� �л� ���ڵ带 �����ϴ�
// � �ʵ嵵 �����ϴ�. ���������� readRecord() �Լ��� ȣ���Ͽ� sequential search�� �����Ѵ�.
// �˻� ����� ����� �� �ݵ�� printRecord() �Լ��� ����Ѵ�. (�ݵ�� ���Ѿ� �ϸ�, �׷���
// �ʴ� ��� ä�� ���α׷����� �ڵ������� Ʋ�� ������ �ν���)
//
void searchRecord(FILE *fp, enum FIELD f, char *keyval)
{
	int rrn = 0;
	STUDENT student;

	while(readRecord(fp, &student, rrn) > 0)
	{
		if(f == ID && !strcmp(student.id, keyval))
			printRecord(&student);
		else if(f == NAME && !strcmp(student.name, keyval))
			printRecord(&student);
		else if(f == DEPT && !strcmp(student.dept, keyval))
			printRecord(&student);
		else if(f == ADDR && !strcmp(student.addr, keyval))
			printRecord(&student);
		else if(f == EMAIL && !strcmp(student.email, keyval))
			printRecord(&student);
		rrn++;
	}

}
void printRecord(const STUDENT *s);

//
// ���ڵ��� �ʵ���� enum FIELD Ÿ���� ������ ��ȯ���� �ش�.
// ���� ���, ����ڰ� ������ ��ɾ��� ���ڷ� "NAME"�̶�� �ʵ���� ����Ͽ��ٸ� 
// ���α׷� ���ο��� �̸� NAME(=1)���� ��ȯ�� �ʿ伺�� ������, �̶� �� �Լ��� �̿��Ѵ�.
//
enum FIELD getFieldID(char *fieldname)
{
	if(!strcmp(fieldname, "ID"))
		return ID;
	if(!strcmp(fieldname, "NAME"))
		return NAME;
	if(!strcmp(fieldname, "DEPT"))
		return DEPT;
	if(!strcmp(fieldname, "ADDR"))
		return ADDR;
	if(!strcmp(fieldname, "EMAIL"))
		return EMAIL;
}

void main(int argc, char *argv[])
{
	FILE *fp;			// ��� file processing operation�� C library�� ����� ��
	STUDENT s;

	if(argc != 8 && argc != 4){
		fprintf(stderr, "a.out -a record_file_name \"file_value1\" \"field_value2\" \"field_value3\" \"field_value4\" \"field_value5\"\n");
		fprintf(stderr, "a.out -s record_file_name \"field_name=field_value\"\n");
		exit(1);
	}

	if(!strcmp(argv[1], "-a")) {
		if(argc != 8) {
			fprintf(stderr, "a.out -a record_file_name \"file_value1\" \"field_value2\" \"field_value3\" \"field_value4\" \"field_value5\"\n");
			exit(1);
		}
		if(strlen(argv[3]) > 8) {
			fprintf(stderr, "id is within 8 characters\n");
			exit(1);
		}
		if(strlen(argv[4]) > 10) {
			fprintf(stderr, "name is within 10 characters\n");
			exit(1);
		}
		if(strlen(argv[5]) > 12) {
			fprintf(stderr, "dept is within 12 characters\n");
			exit(1);
		}
		if(strlen(argv[6]) > 30) {
			fprintf(stderr, "address is within 30 characters\n");
			exit(1);
		}
		if(strlen(argv[7]) > 20) {
			fprintf(stderr, "email is within 20 characters\n");
			exit(1);
		}

		if((fp = fopen(argv[2], "r+")) == NULL) {
			fp = fopen(argv[2], "w+");
		}
		fseek(fp, 0, SEEK_SET);

		if(appendRecord(fp, argv[3], argv[4], argv[5], argv[6], argv[7]) == 0) {
			fprintf(stderr, "appendRecord error\n");
			exit(1);
		}

		exit(0);
	}

	else if(!strcmp(argv[1], "-s"))
	{
		if(argc != 4) {
			fprintf(stderr, "a.out -s record_file_name \"field_name=field_value\"\n");
			exit(1);
		}
		if((fp = fopen(argv[2], "r")) == NULL){
			fprintf(stderr, "Error: %s cannot open\n", argv[2]);
			exit(1);
		}

		enum FIELD F;
		char field[7], key[31];
		int i;

		for(i = 0; argv[3][i] != '='; i++) {
			if(i > 6) {
				fprintf(stderr, "Error: Wrong field_name\n");
				exit(1);
			}
			field[i] = argv[3][i];
		}
		field[i] = '\0';

		if(strcmp(field, "ID") && strcmp(field, "NAME") && strcmp(field, "DEPT") && strcmp(field, "ADDR") && strcmp(field, "EMAIL")) {
			fprintf(stderr, "Error: Wrong field_name\n");
			exit(1);
		}

		F = getFieldID(field);

		i++;
		int j = 0;
		while(argv[3][i] != '\0') {
			if(j > 30) {
				fprintf(stderr, "Error: Wrong field_value\n");
				exit(1);
			}
				
			key[j] = argv[3][i];
			i++;
			j++;
		}
		key[j] = '\0';

		searchRecord(fp, F, key);
	}

	else {
		fprintf(stderr, "a.out -a record_file_name \"file_value1\" \"field_value2\" \"field_value3\" \"field_value4\" \"field_value5\"\n");
		fprintf(stderr, "a.out -s record_file_name \"field_name=field_value\"\n");
		exit(1);
	}
}

void printRecord(const STUDENT *s)
{
	printf("%s | %s | %s | %s | %s\n", s->id, s->name, s->dept, s->addr, s->email);
}
