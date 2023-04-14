#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<time.h>
#include<sys/time.h>

#define RECORD_SIZE 91
#define BOOK_SIZE 33

typedef struct userList {
	char id[11];
	char name[10];
	char addr[50];
	char email[20];
} userList;

typedef struct spotList {
	int room[6];
} spotList;

typedef struct bookList {
	char id[11];
	int spot_num;
	int room_num;
	int time;
	int year;
	int month;
	int day;
	int use_time;
	int people;
} bookList;

void admin();
void user();

int check_day(int mon, int day);

int newBook();
int changeBook();
void print_room();
void print_spot();

void readBook();
void writeBook();
int input_func();

int searchRecord(int fd, userList *user, char *id);

int appendRecord(int fd, userList *user);
int readRecord(int fd, userList *user, int rrn);

void pack(char *recordbuf, const userList *user);
void unpack(const char *recordbuf, userList *user);

int spot_manage();
void readSpot(int fd, spotList *spot);
int modifySpot(int fd, spotList *spot);
int addSpot(int fd, spotList *spot);
int deleteSpot(int fd, spotList *spot);

int modifyRoom();
int addRoom(int fd, int *room);
int deleteRoom(int fd, int *room);

//성공 시 입력된 번호, 실패 시 0 리턴
int scan_num(int max_num)
{
	char c;
	char buf[100] = {0, };
	int select_num;

	while(buf[0] == 0) {
		memset(buf, 0, sizeof(buf));
		printf("(%d 입력 시 초기 화면)>> ", max_num);
		scanf("%[ \t]", buf);
		scanf("%[^\n]", buf);
		c = getchar();
	}

	if(buf[1] != '\0' && buf[1] != '\t' && buf[1] != ' ' && (buf[1] > '9' || buf[1] < '0'))
		return 0;

	if(buf[2] != ' ' && buf[2] != '\t' && buf[2] != '\0')
		return 0;

	select_num = atoi(buf);

	if(select_num < 1 || select_num > max_num)
		return 0;

	return select_num;
}

int scan_year()
{
	char c;
	char buf[100] = {0, };
	int year;

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	while(buf[0] == 0) {
		memset(buf, 0, sizeof(buf));
		printf("(-1 입력 시 초기 화면)>> ");
		scanf("%[ \t]", buf);
		scanf("%[^\n]", buf);
		c = getchar();
	}

	if(!strcmp(buf, "-1"))
		return -1;

	if(buf[2] != '\0' && buf[2] != ' ' && buf[2] != '\t')
		return 0;
	buf[2] = '\0';

	year = atoi(buf);
	if(year < t->tm_year-100) {
		printf("이미 지난 연도입니다.\n");
		return 0;
	}

	return year;
}

int scan_mon(int year)
{
	char c;
	char buf[100] = {0, };
	int month;

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	while(buf[0] == 0) {
		memset(buf, 0, sizeof(buf));
		printf("(-1 입력 시 초기 화면)>> ");
		scanf("%[ \t]", buf);
		scanf("%[^\n]", buf);
		c = getchar();
	}

	if(!strcmp(buf, "-1"))
		return -1;

	if(buf[2] != '\0' && buf[2] != ' ' && buf[2] != '\t')
		return 0;
	buf[2] = '\0';

	month = atoi(buf);
	if(year == t->tm_year-100 && month < t->tm_mon+1) {
		if(month > 0)
			printf("이미 지난 날짜입니다.\n");
		return 0;
	}
	if (month > 12 || month < 1)
		printf("존재하지 않는 날짜입니다. (1 ~ 12)\n");

	return month;
}

int scan_day(int year, int month)
{
	char c;
	char buf[100] = {0, };
	int day;

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	while(buf[0] == 0) {
		memset(buf, 0, sizeof(buf));
		printf("(-1 입력 시 초기 화면)>> ");
		scanf("%[ \t]", buf);
		scanf("%[^\n]", buf);
		c = getchar();
	}

	if(!strcmp(buf, "-1"))
		return -1;

	if(buf[2] != '\0' && buf[2] != ' ' && buf[2] != '\t')
		return 0;
	buf[2] = '\0';

	day = atoi(buf);
	if(year == t->tm_year-100 && month == t->tm_mon+1 && day <= t->tm_mday) {
		printf("내일부터 예약 가능합니다.\n");
		return 0;
	}
	else if (!check_day(month, day)) {
		printf("존재하지 않는 날짜입니다.\n");
		return 0;
	}

	return day;
}

int check_day(int mon, int day)
{
	if(mon == 1 || mon == 3 || mon == 5 || mon == 7 || mon == 8 || mon == 10 || mon == 12)
		if(day > 31)
			return 0;
		else
			return 1;

	if(mon == 2)
		if(day > 28)
			return 0;
		else
			return 1;

	if(mon == 4 || mon == 6 || mon == 9 || mon == 11)
		if(day > 30)
			return 0;
		else
			return 1;
}

void print_spot(spotList *spot, int min_people)
{
	printf("----- 이용 가능한 지점 -----\n");
	for(int i = 0; i < 6; i++)
		if(spot[0].room[i] > 0)
			printf("지점 %d, ", i+1);

	printf("\b\b \n\n");

	for (int i = 1; i <= 6; i++)
		if(spot[0].room[i-1] > 0)
			print_room(i, spot[i].room, min_people);
}

int IS_BOOK(bookList *book, int spot_num, int room_num, int year, int month, int day, int time, int use_time)
{
	int i = 0;
	int end_time = time + use_time;

	while(book[i].id[0] != 0) {
		if(book[i].spot_num == spot_num && book[i].room_num == room_num && book[i].year == year && book[i].month == month && book[i].day == day) {
			if((time >= book[i].time && time < book[i].time+book[i].use_time) || (end_time > book[i].time && end_time <= book[i].time+book[i].use_time))
				return 1;
			else if(time < book[i].time && end_time >= book[i].time+book[i].use_time)
				return 1;
		}

		i++;
	}
	return 0;
}

void print_room(int spot_num, int *room, int min_people)
{
	printf("* 지점 %d 스터디룸 현황 *\n", spot_num);
	for(int i = 1; i < 6; i++)
	{
		if(room[i] > 0) {
			if(room[i] >= min_people)
				printf("스터디룸 %d (수용 인원: %d)\n", i, room[i]);
			else
				printf("스터디룸 %d (수용 인원: %d)\n", i, room[i]);
		}
	}
	printf("\n");
}

int main()
{
	int num;

	while (1) {
		printf("----- 초기 화면 -----\n");
		printf("1. 관리자 모드\n2. 사용자 모드\n3. 프로그램 종료\n");

		if((num = scan_num(4)) == 0) {
			fprintf(stderr, "Error: Wrong input\n");
			continue;
		}

		printf("\n");

		switch(num)
		{
			case 1:
				admin();
				break;
			case 2:
				user();
				break;
			case 3:
				printf("Program End\n");
				exit(0);
				break;
		}
	}
	exit(0);
}

void user()
{
	userList user_buf;
	char input_id[11] = {0, };
	char *fname = "userfile.txt";
	int fd;
	int user_num = 1;
	int nul;

	if((fd = open(fname, O_RDWR | O_CREAT, 0644)) < 0) {
		fprintf(stderr, "open error for %s\n", fname);
		exit(1);
	}

	if(read(fd, &user_num, sizeof(int)) == 0) {
		user_num = 1;
		write(fd, &user_num, sizeof(int));
	}

	printf("----- 사용자 모드 -----\n");
	while(!(nul = input_func(input_id, 10)));

	if(nul == -1) {
		close(fd);
		return;
	}

	if(searchRecord(fd, &user_buf, input_id) == 0) {
		strcpy(user_buf.id, input_id);
		sprintf(user_buf.name, "user %d", user_num);
		sprintf(user_buf.addr, "서울시 동작구 상도로 %d", user_num);
		sprintf(user_buf.email, "user%d@ssu.ac.kr", user_num);

		user_num++;
		lseek(fd, 0, SEEK_SET);
		write(fd, &user_num, sizeof(int));
		appendRecord(fd, &user_buf);
	}

	printf("\n");

	close(fd);

	spotList spot[7];
	bookList book[1000];
	fname = "spotfile.txt";
	char *fname2 = "bookfile.txt";
	int fd2;

	memset(book, 0, sizeof(book));
	memset(spot, 0, sizeof(spot));

	if((fd = open(fname, O_RDWR | O_CREAT, 0644)) < 0) {
		fprintf(stderr, "open error for %s\n", fname);
		return;
	}

	readSpot(fd, spot);

	if((fd2 = open(fname2, O_RDWR | O_CREAT, 0644)) < 0) {
		fprintf(stderr, "open error for %s\n", fname);
		exit(1);
	}

	readBook(fd2, book);

	printf("----- %s -----\n", input_id);
	printf("1. 스터디 공간 조회\n2. 예약 변경\n3. 신규 예약\n4. 초기 화면\n");

	int num;
	while((num = scan_num(4)) == 0)
		fprintf(stderr, "Error: Wrong input\n");
	printf("\n");

	switch(num)
	{
		case 1:
			print_spot(spot, 0);
			break;
		case 2:
			changeBook(fd2, spot, book, user_buf.id);
			break;
		case 3:
			newBook(fd2, spot, book, user_buf.id);
			break;
		case 4:

			break;
	}

	lseek(fd, 0, SEEK_SET);
	for(int i = 0; i < 7; i++)
		write(fd, &spot[i], sizeof(spotList));

	close(fd);
	close(fd2);
}

int changeBook(int fd, spotList *spot, bookList *book, char *user_id)
{
	int index[1000], book_count = 0, j = 0;

	printf("----- %s: 예약 목록 -----\n", user_id);
	while(book[book_count].id[0] != 0)
	{
		if(!strcmp(book[book_count].id, user_id)) {
			index[j] = book_count;
			j++;
		}
		book_count++;
	}

	for(int i = 0; i < j; i++) {
		printf("[%d] 지점 번호: %d, 스터디룸 번호: %d, 날짜: %02d-%02d-%02d, 시간: %02d:00-%02d:00, 예약 인원: %d\n", i+1, book[index[i]].spot_num, book[index[i]].room_num, book[index[i]].year, book[index[i]].month, book[index[i]].day, book[index[i]].time, book[index[i]].time+book[index[i]].use_time, book[index[i]].people);
	}
	printf("\n");

	if(j == 0) {
		printf("%s 님의 예약이 없습니다.\n", user_id);
		return 0;
	}

	int num;
	bookList book_tmp;

	printf("변경할 번호를 입력하세요 ");
	while((num = scan_num(j+1)) == 0) {
		printf("Error: Input 1 ~ %d number\n", j);
		printf("변경할 번호를 다시 입력하세요 ");
	}
	printf("\n");
	if(num == j+1)
		return 0;

	memcpy(&book_tmp, &book[num-1], sizeof(bookList));
	memcpy(&book[num-1], &book[book_count-1], sizeof(bookList));
	memcpy(&book[book_count-1], &book_tmp, sizeof(bookList));

	memset(&book[book_count-1], 0, sizeof(bookList));

	printf("----- 예약 변경 -----\n");
	printf("1. 삭제\n2. 재예약\n\n");
	while((num = scan_num(3)) == 0) 
		printf("Error: Input 1 ~ 2 number\n");

	if(num == 3)
		return 0;
	else if(num == 2)
		newBook(fd, spot, book, user_id);
	else
		writeBook(fd, book, user_id);

	return 1;
}

int newBook(int fd, spotList *spot, bookList *book, char *user_id)
{
	int spot_num, room_num;
	int people;
	int time, year, month, day, use_time;

	time = 5;
	use_time = 1;

	printf("예약 인원을 입력하세요 ");
	while((people = scan_num(11)) == 0) {
		printf("Error: Input 1 ~ 10 number\n");
		printf("예약 인원을 다시 입력하세요 ");
	}
	if(people == 11)
		return 0;

	print_spot(spot, people);

	printf("예약할 지점 번호를 입력하세요 ");
	while(1) {
		while((spot_num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("예약 인원을 다시 입력하세요 ");
		}
		if(spot_num == 7)
			return 0;
		if(spot[0].room[spot_num-1] == 0) {
			printf("존재하지 않는 지점입니다.\n");
			printf("지점 번호를 다시 입력하세요 ");
			continue;
		}
		break;
	}

	printf("예약할 스터디룸 번호를 입력하세요 ");
	while(1){
		while((room_num = scan_num(6)) == 0) {
			printf("Error: Input 1 ~ 5 number\n");
			printf("예약 인원을 다시 입력하세요 ");
		}
		if(room_num == 6)
			return 0;
		else if(spot[spot_num].room[room_num] == 0) {
			printf("존재하지 않는 스터디룸입니다.\n");
			printf("스터디룸 번호를 다시 입력하세요 ");
			continue;
		}
		else if(spot[spot_num].room[room_num] < people) {
			printf("예약할 수 없는 스터디룸입니다. (스터디룸 %d의 수용 인원: %d)\n", room_num, spot[spot_num].room[room_num]);
			printf("스터디룸 번호를 다시 입력하세요 ");
			continue;
		}
		break;
	}

	printf("연도를 입력하세요 ");
	while((year = scan_year()) < 1) {
		if(year == -1)
			return 0;
		printf("연도를 다시 입력하세요 ");
	}

	printf("월을 입력하세요 ");
	while((month = scan_mon(year)) < 1) {
		if(month == -1)
			return 0;
		printf("월을 다시 입력하세요 ");
	}

	printf("일을 입력하세요 ");
	while((day = scan_day(year, month)) < 1) {
		if(day == -1)
			return 0;
		printf("일을 다시 입력하세요 ");
	}
	// 예약 가능 시간 출력하는 코드
	printf("-----예약 가능 시간-----\n");
	for(int i = 8; i < 22; i++)
		if(!IS_BOOK(book, spot_num, room_num, year, month, day, i, 1))
			printf("%d:00 ", i);
	printf("\n\n");

	// 예약 시간 입력 받는 코드
	printf("시간을 입력하세요 ");
	while((time = scan_num(22)) < 8) {
		printf("오전 8시부터 22시까지 예약할 수 있습니다.\n");
		printf("시간을 다시 입력하세요 ");
	}
	if(time == 22)
		return 0;

	printf("이용할 시간을 입력하세요 ");
	while((use_time = scan_num(16)) < 1 || (time + use_time) > 22) {
		if(use_time == 16)
			return 0;
		if((time + use_time) > 22)
			printf("22시 이후에는 이용할 수 없습니다. (%d시에 예약할 경우 최대 %d시간만 이용 가능)\n", time, 22-time);
		printf("이용할 시간을 다시 입력하세요 ");
	}

	if(IS_BOOK(book, spot_num, room_num, year, month, day, time, use_time)){
		printf("\n예약 실패: 이미 예약된 스터디룸입니다.\n");
		printf("초기 화면으로 이동합니다.\n\n");
		return 0;
	}

	int book_count = 0;
	while(book[book_count].id[0] != 0)
		book_count++;

	strcpy(book[book_count].id, user_id);
	book[book_count].spot_num = spot_num;
	book[book_count].room_num = room_num;
	book[book_count].year = year;
	book[book_count].month = month;
	book[book_count].day = day;
	book[book_count].time = time;
	book[book_count].use_time = use_time;
	book[book_count].people = people;

	writeBook(fd, book, user_id);
	printf("지점 %d의 스터디룸 %d 예약이 완료되었습니다.\n\n", spot_num, room_num);
}

void writeBook(int fd, bookList *book, char *user_id)
{
	char book_buf[100] = {0, };
	int i = 0;

	ftruncate(fd, 0);

	while(book[i].id[0] != 0) {
		memset(book_buf, 0, sizeof(book_buf));
		sprintf(book_buf, "%1d%1d#%02dy%02dm%02dd%02dt%02du%02dp%s", book[i].spot_num, book[i].room_num, book[i].year, book[i].month, book[i].day, book[i].time, book[i].use_time, book[i].people, book[i].id);
		lseek(fd, 0, SEEK_END);
		write(fd, book_buf, sizeof(char)*BOOK_SIZE);
		i++;
	}

}
void admin()
{
	int num;
	char *fname = "spotfile.txt";
	spotList spot[7];
	int fd;

	if((fd = open(fname, O_RDWR | O_CREAT, 0644)) < 0) {
		fprintf(stderr, "open error for %s\n", fname);
		return;
	}

	readSpot(fd, spot);

	printf("----- 관리자 모드 -----\n");
	printf("1. 지점 관리\n2. 지점별 스터디 공간 관리\n3. 초기 화면\n");

	while((num = scan_num(3)) == 0)
		fprintf(stderr, "Error: Wrong input\n");
	printf("\n");

	switch(num)
	{
		case 1:
			spot_manage(fd, spot);
			break;
		case 2:
			modifySpot(fd, spot);
			break;
		case 3:
			break;
	}

	lseek(fd, 0, SEEK_SET);
	for(int i = 0; i < 7; i++)
		write(fd, &spot[i], sizeof(spotList));

	close(fd);
}

int spot_manage(int fd, spotList *spot)
{
	int num;

	printf("----- 지점 관리 -----\n");
	printf("1. 지점 추가\n2. 지점 삭제\n3. 초기 화면\n");

	while((num = scan_num(3)) == 0)
		fprintf(stderr, "Error: Wrong input\n");
	printf("\n");

	switch(num)
	{
		case 1:
			addSpot(fd, spot);
			break;
		case 2:
			deleteSpot(fd, spot);
			break;
		case 3:
			break;
	}

	return 1;
}

int modifySpot(int fd, spotList *spot)
{
	int num = 0;

	printf("수정할 지점 번호를 입력하세요 ");

	while(1)
	{
		while((num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("수정할 지점 번호를 다시 입력하세요 ");
		}
		if(num == 7)
			return 0;
		if(spot[0].room[num-1] == 1) {
			break;
		}

		printf("존재하지 않는 지점입니다.\n");
		printf("수정할 지점 번호를 다시 입력하세요 ");
	}

	printf("----- 지점 %d 수정 -----\n", num);
	printf("1. 스터디 공간 추가\n2. 스터디 공간 수정\n3. 스터디 공간 삭제\n4. 초기 화면\n");

	int room_num;
	while((room_num = scan_num(4)) == 0)
		fprintf(stderr, "Error: Wrong input\n");
	printf("\n");

	switch(room_num)
	{
		case 1:
			addRoom(fd, spot[num].room);
			break;
		case 2:
			modifyRoom(fd, spot[num].room);
			break;
		case 3:
			deleteRoom(fd, spot[num].room);
			break;
		case 4:
			break;
	}

	return 1;
}

int modifyRoom(int fd, int *room)
{
	int num;

	printf("수정할 스터디룸 번호를 입력하세요 ");
	while(1) {
		while((num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("수정할 스터디룸 번호를 다시 입력하세요 ");
		}

		if(num == 7)
			return 0;

		if(room[num] == 0) {
			printf("존재하지 않는 스터디룸입니다.\n");
			printf("수정할 스터디룸 번호를 다시 입력하세요 ");
			num = 0;
		}
		else
			break;
	}


	int allow;
	printf("허용 인원을 재설정하세요 ");
	while (1) {
		while((allow = scan_num(11)) == 0) {
			printf("Error: Input 1 ~ 10 number\n");
			printf("허용 인원을 다시 입력하세요 ");
		}

		if(allow == 11)
			return 0;
		else
			break;
	}

	room[num] = allow;

	return 1;
}

int addRoom(int fd, int *room)
{
	int num;

	printf("추가할 스터디룸 번호를 입력하세요 ");
	while(1) {
		while((num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("추가할 스터디룸 번호를 다시 입력하세요 ");
		}

		if(num == 7)
			return 0;

		if(room[num] > 0) {
			printf("이미 추가된 스터디룸입니다.\n");
			printf("추가할 스터디룸 번호를 다시 입력하세요 ");
			num = 0;
		}
		else
			break;
	}

	int allow;
	printf("허용 인원을 설정하세요 ");
	while (1) {
		while((allow = scan_num(11)) == 0) {
			printf("Error: Input 1 ~ 10 number\n");
			printf("허용 인원을 다시 입력하세요 ");
		}

		if(allow == 11)
			return 0;
		else
			break;
	}

	room[0] = 0;
	room[num] = allow;

	return 1;
}

int deleteRoom(int fd, int *room)
{
	int num;

	printf("예약 정보도 모두 삭제됩니다.\n");
	printf("삭제할 스터디룸 번호를 입력하세요 ");
	while(1) {
		while((num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("삭제할 스터디룸 번호를 다시 입력하세요 ");
		}

		if(num == 7)
			return 0;

		if(room[num] == 0) {
			printf("존재하지 않는 스터디룸입니다.\n");
			printf("삭제할 스터디룸 번호를 다시 입력하세요 ");
			num = 0;
		}
		else
			break;
	}

	room[0] = 0;
	room[num] = 0;

	return 1;
}

int addSpot(int fd, spotList *spot)
{
	int num;

	printf("추가할 지점 번호를 입력하세요 ");
	while(1) {
		while((num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("추가할 지점 번호를 다시 입력하세요 ");
		}

		if(num == 7)
			return 0;

		if(spot[0].room[num-1] == 1) {
			printf("이미 추가된 지점입니다.\n");
			printf("추가할 지점 번호를 다시 입력하세요 ");
			num = 0;
		}
		else
			break;
	}

	spot[0].room[num-1] = 1;

	memset(&spot[num], 0, sizeof(int)*6);

	return 1;
}

int deleteSpot(int fd, spotList *spot)
{
	int num;

	printf("예약된 스터디룸이 있어도 삭제됩니다.\n");
	printf("삭제할 지점 번호를 입력하세요 ");
	while(1) {
		while((num = scan_num(7)) == 0) {
			printf("Error: Input 1 ~ 6 number\n");
			printf("삭제할 지점 번호를 다시 입력하세요 ");
		}

		if(num == 7)
			return 0;

		if(spot[0].room[num-1] == 0) {
			printf("존재하지 않는 지점입니다.\n");
			printf("삭제할 지점 번호를 다시 입력하세요 ");
			num = 0;
		}
		else
			break;
	}

	spot[0].room[num-1] = 0;

	memset(&spot[num], 0, sizeof(int)*6);

	return 1;
}

void readSpot(int fd, spotList *spot)
{
	lseek(fd, 0, SEEK_SET);
	if(read(fd, spot, sizeof(spotList)*7) == 0) {
		memset(spot, 0, sizeof(spotList)*7);
		write(fd, spot, sizeof(spotList)*7);
	}
}

void readBook(int fd, bookList *book)
{
	int i = 0;
	char buf[BOOK_SIZE+1];
	memset(buf, 0, BOOK_SIZE);

	lseek(fd, 0, SEEK_SET);
	while(read(fd, buf, BOOK_SIZE) > 0) {
		char time[3] = {0, };
		char spot_num[2] = {0, };
		char room_num[2] = {0, };
		char year[3] = {0, };
		char month[3] = {0, };
		char day[3] = {0, };
		char use_time[3] = {0, };
		char people[3] = {0, };

		buf[5] = '\0';
		buf[8] = '\0';
		buf[11] = '\0';
		buf[14] = '\0';
		buf[17] = '\0';
		buf[20] = '\0';
		buf[sizeof(buf)-1] = '\0';

		spot_num[0] = buf[0];
		room_num[0] = buf[1];
		strcpy(year, &buf[3]);
		strcpy(month, &buf[6]);
		strcpy(day, &buf[9]);
		strcpy(time, &buf[12]);
		strcpy(use_time, &buf[15]);
		strcpy(people, &buf[18]);

		book[i].spot_num = atoi(spot_num);
		book[i].room_num = atoi(room_num);
		book[i].year = atoi(year);
		book[i].month = atoi(month);
		book[i].day = atoi(day);
		book[i].time = atoi(time);
		book[i].use_time = atoi(use_time);
		book[i].people = atoi(people);

		strcpy(book[i].id, &buf[21]);

		//		printf("book[%d] - ", i);
		//		printf("spot_num : %d, room_num : %d, year : %d, month : %d, day : %d, time : %d, use_time : %d, people : %d\n", book[i].spot_num, book[i].room_num, book[i].year, book[i].month, book[i].day, book[i].time, book[i].use_time, book[i].people);
		i++;
	}
}

int input_func(char *input, int len)
{
	char c;
	char buf[100] = {0, };

	while(buf[0] == 0) {
		memset(buf, 0, sizeof(buf));
		printf("----- 로그인 화면 -----\n");
		printf("id (-1 입력 시 초기화면): ");
		scanf("%[^\n]", buf);
		c = getchar();
	}

	if(!strcmp(buf, "-1"))
		return -1;

	if(strlen(buf) < 5 || strlen(buf) > len) {
		fprintf(stderr, "Input 5 to %d character\n", len);
		return 0;
	}

	strcpy(input, buf);
	return 1;
}

int searchRecord(int fd, userList *user, char *id)
{
	int rrn = 0;

	while(readRecord(fd, user, rrn) > 0)
	{
		if(!strcmp(id, user->id))
			return 1;
		rrn++;
	}

	return 0;
}

int readRecord(int fd, userList *user, int rrn)
{
	char recordbuf[RECORD_SIZE+1];
	memset(recordbuf, 0, sizeof(recordbuf));

	lseek(fd, 4+(rrn*RECORD_SIZE), SEEK_SET);
	if(read(fd, recordbuf, RECORD_SIZE) == 0)
		return 0;

	unpack(recordbuf, user);

	return 1;
}

void unpack(const char *recordbuf, userList *user)
{
	int i = 0, j;

	j = 0;
	while(recordbuf[i] != '#') {
		(user->id)[j] = recordbuf[i];
		i++;
		j++;
	}
	(user->id)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(user->name)[j] = recordbuf[i];
		i++;
		j++;
	}
	(user->name)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(user->addr)[j] = recordbuf[i];
		i++;
		j++;
	}
	(user->addr)[j] = '\0';

	i++;
	j = 0;
	while(recordbuf[i] != '#') {
		(user->email)[j] = recordbuf[i];
		i++;
		j++;
	}
	(user->email)[j] = '\0';

}

int appendRecord(int fd, userList *user)
{
	char recordbuf[RECORD_SIZE+1];

	memset(recordbuf, 0, sizeof(recordbuf));

	pack(recordbuf, user);

	lseek(fd, 0, SEEK_END);
	if(write(fd, recordbuf, RECORD_SIZE) < 1)
		return 0;

	return 1;
}

void pack(char *recordbuf, const userList *user)
{
	strcpy(recordbuf, user->id);
	strcat(recordbuf, "#");
	strcat(recordbuf, user->name);
	strcat(recordbuf, "#");
	strcat(recordbuf, user->addr);
	strcat(recordbuf, "#");
	strcat(recordbuf, user->email);
	strcat(recordbuf, "#");
}
