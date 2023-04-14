#include <stdio.h>		// 필요한 header file 추가 가능
#include <string.h>
#include <stdlib.h>
#include "student.h"

void pack(char *recordbuf, const STUDENT *s);
void unpack(const char *recordbuf, STUDENT *s);
void printRecord(const STUDENT *s);
//
// 함수 readRecord()는 학생 레코드 파일에서 주어진 rrn에 해당하는 레코드를 읽어서 
// recordbuf에 저장하고, 이후 unpack() 함수를 호출하여 학생 타입의 변수에 레코드의
// 각 필드값을 저장한다. 성공하면 1을 그렇지 않으면 0을 리턴한다.
// unpack() 함수는 recordbuf에 저장되어 있는 record에서 각 field를 추출하는 일을 한다.
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
// 함수 writeRecord()는 학생 레코드 파일에 주어진 rrn에 해당하는 위치에 recordbuf에 
// 저장되어 있는 레코드를 저장한다. 이전에 pack() 함수를 호출하여 recordbuf에 데이터를 채워 넣는다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
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
// 함수 appendRecord()는 학생 레코드 파일에 새로운 레코드를 append한다.
// 레코드 파일에 레코드가 하나도 존재하지 않는 경우 (첫 번째 append)는 header 레코드를
// 파일에 생성하고 첫 번째 레코드를 저장한다. 
// 당연히 레코드를 append를 할 때마다 header 레코드에 대한 수정이 뒤따라야 한다.
// 함수 appendRecord()는 내부적으로 writeRecord() 함수를 호출하여 레코드 저장을 해결한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
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
// 학생 레코드 파일에서 검색 키값을 만족하는 레코드가 존재하는지를 sequential search 기법을 
// 통해 찾아내고, 이를 만족하는 모든 레코드의 내용을 출력한다. 검색 키는 학생 레코드를 구성하는
// 어떤 필드도 가능하다. 내부적으로 readRecord() 함수를 호출하여 sequential search를 수행한다.
// 검색 결과를 출력할 때 반드시 printRecord() 함수를 사용한다. (반드시 지켜야 하며, 그렇지
// 않는 경우 채점 프로그램에서 자동적으로 틀린 것으로 인식함)
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
// 레코드의 필드명을 enum FIELD 타입의 값으로 변환시켜 준다.
// 예를 들면, 사용자가 수행한 명령어의 인자로 "NAME"이라는 필드명을 사용하였다면 
// 프로그램 내부에서 이를 NAME(=1)으로 변환할 필요성이 있으며, 이때 이 함수를 이용한다.
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
	FILE *fp;			// 모든 file processing operation은 C library를 사용할 것
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
