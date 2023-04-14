#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<dirent.h>
#include<errno.h>
#include<unistd.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/time.h>

void find(const char*, const char*);
void find_dfs(const char*, const char*, char [][1024], int*, const long int, const int);
void cal_size(const char *, long int *);
void diff(const char *, const char *, const int *);
void diff_dir(const char*, const char*, const int*);
int is_sametxt(const char*, const char*, const int *);
int stricmp(const char *, const char *);


//scandir()함수에 사용할 필터함수
int filter(const struct dirent *entry){
	return (strcmp(entry->d_name, ".") && (strcmp(entry->d_name, "..")));
}

int main(){
	char instruction[6] = {0,}, input[1024] = {0,};		//명령어 입력받을 변수
	char filename[1024], path[1024];		//find 명령어일 경우 filename과 path 저장하는 변수
	char c;		//입력버퍼에 남아있는 엔터 없앨 변수
	int p = 5, i = 0;		//input을 구분하기 위한 index, i는 반복문을 위한 index

	struct timeval start_time, end_time;	//프로그램 시작 시간과 끝나는 시간을 담을 변수
	long int diff_time_sec, diff_time_usec;	//실행시간 계산하기 위한 변수
	gettimeofday(&start_time, NULL);		//프로그램 시작 시간 저장

	while(1){
		printf("20182610> ");
		input[0] = '\0';				//변수 초기화
		scanf("%[ \t]", input);			//처음 공백을 제외하고 명령어 나오는 순간부터 입력받음
		scanf("%[^\n]", input);
		c = getchar();					//버퍼 비우기

		for(int i = 0; i < 5; i++)		//내장 명령어 구분하여 instruction에 저장
			instruction[i] = input[i];
		instruction[5] = '\0';


		if(instruction[0] == '\0')	//엔터 키만 입력 시 다시 입력
			continue;



		//find 명령어일 경우
		else if((strcmp(instruction, "find ") == 0) || (strcmp(instruction, "find") == 0)){
			p = 5;
			i = 0;
			while((input[p] == ' ') || (input[p] == '\t'))		//공백 제거, 이제 input[p]는 filename의 시작
				p++;

			for(i = 0; ((input[p] != ' ') && (input[p] != '\t')) && (input[p] != '\0'); i++){	//filename 부분만 저장
				filename[i] = input[p];
				p++;
			}
			filename[i] = '\0';		//문자열 끝에 널문자 넣기

			while((input[p] == ' ') || (input[p] == '\t'))		//공백 제거, 이제 input[p]는 path의 시작
				p++;

			for(i = 0; (input[p] != ' ') && (input[p] != '\0'); i++){	//path 부분만 저장
				path[i] = input[p];
				p++;
			}
			path[i] = '\0';			//문자열 끝에 널문자 넣기


			//잘 입력 받을 경우 find 명령어 실행
			if((strcmp(path,"") != 0) && (strcmp(filename, "") != 0)) {
				find(filename, path);	//find함수 실행
			}

			//filename이나 path 입력이 제대로 되지 않을 경우
			else{
				printf("Error: No such file or directory\n");
			}
		}




		//exit 명령어일 경우
		else if((strcmp(instruction, "exit") == 0) || (strcmp(instruction, "exit ") == 0)){
			gettimeofday(&end_time, NULL);		//끝나기 직전 시간 저장
			diff_time_sec = end_time.tv_sec - start_time.tv_sec;
			if((diff_time_usec = end_time.tv_usec - start_time.tv_usec) < 0){	//usec부분이 음수일 경우 양수로 바꾸기 위해 1000000 을 더하고 sec부분을 1 줄여야 한다.
				diff_time_usec += 1000000;
				diff_time_sec--;
			}
			printf("Prompt End\nRuntime: %ld:%ld(sec:usec)\n", diff_time_sec, diff_time_usec);	//프로그램 실행시간 출력
			exit(0);		//프로그램 종료
		}



		//help 명령어일 경우
		else{
			printf(" Usage:\n\t> find [FILENAME] [PATH]\n\t\t>> [INDEX] [OPTION ... ]\n\t> help\n\t> exit\n\n");
			printf("\t[OPTION ... ]\n\t q: report only when files differ\n\t s: report when two files are the same\n\t i: ignore case differences in file contents\n\t r: recursively compare any subdirectories found\n");
		}

	}
	return 0;
}


void find(const char *filename, const char *path){

	struct dirent **namelist;		//디렉토리내 파일 이름 저장하는 변수
	struct stat buf;	//파일 정보를 담을 변수
	struct tm *ptm;		//

	int count;	//파일 및 디렉토리 전체 개수
	int i, search = 0, len_filename = 0;	//for문 반복할 변수 i, 파일이나 디렉토리가 있는지 검사할 변수 search, filename의 길이 len_filename
	int is_file;		//디렉토리인지 아닌지 구분, 디렉토리면 is_file = 0, 파일이면 is_file = 1
	int fd, findex = 0;		//파일 디스크립터, index

	long int dsize = 0;		//디렉토리 크기 저장하는 변수

	char c;		//입력버퍼 비우는 용도
	char real_path[1024];		//파일 절대경로 담는 변수
	char filepath[1024] = {0, }, only_filename[1024];	//파일 경로를 담을 변수, 경로없이 파일 이름만 담을 변수
	char mode[10] = {'-','-','-','-','-','-','-','-','-','\0'};	//접근모드 저장
	char path_array[100][1024];		//탐색한 파일들의 경로를 담는배열


	//filename이 절대경로인지 확인
	len_filename = strlen(filename);
	for(i = 0; i < len_filename; i++) {
		//filename이 경로일 경우 절대경로로 변환해서 저장
		if(filename[i] == '/'){
			//절대 경로로 변환, 변환이 안되면 파일이 없다는 것
			if(realpath(filename, filepath) == NULL){
				printf("Error: No such file or directory\n");
				return;
			}
			search = 1;	//변환이 됐다면 검색 성공

			//파일 절대경로에서 파일 이름만 추출하기 위해 j를 마지막 / 를 가리키도록 한다.
			for(int j = len_filename - 1; j >= 0; j--){
				if(filename[j] == '/'){
					strcpy(only_filename, &filename[j+1]);	//파일이름 저장
					break;
				}
			}
			break;
		}
	}
	if(search == 0) {

		strcpy(only_filename, filename);
		//scandir()함수로 디렉토리 내 파일 저장
		if((count = scandir(path, &namelist, filter, alphasort)) == -1){
			fprintf(stderr, "scandir Error: %s\n", strerror(errno));
			return;
		}

		//검색한 파일이나 디렉토리가 있는지 확인
		for(i = 0; i < count; i++){
			if(strcmp(only_filename, namelist[i]->d_name) == 0){
				search = 1;		//파일이 있으면 search에 1을 넣어준다.
				break;
			}
		}


		//namelist 메모리 해제
		for(i = 0; i < count; i++){
			free(namelist[i]);
		}
		free(namelist);


		//파일이나 디렉토리가 없을 경우 에러메시지 띄우고 종료
		if(search == 0){
			printf("Error: No such file or directory\n");
			return;
		}


		//절대 경로로 바꾸고 파일명 결합
		if(realpath(path, filepath) == NULL) {
			fprintf(stderr, "Realpath Error\n");
			return;
		}
		if(strcmp(filepath,"/"))
			strcat(filepath, "/");
		strcat(filepath, only_filename);
	}
	//path_array배열에 0번 index 부분에 검색할 파일 저장
	strcpy(path_array[0], filepath);


	//파일 정보 담기
	lstat(filepath, &buf);

	//파일인지 디렉토리인지 구분
	//디렉토리라면
	if((buf.st_mode & S_IFMT) == S_IFDIR){

		is_file = 0;		//디렉토리이면 is_file = 0
		//디렉토리 열고 크기 계산
		cal_size(filepath, &dsize);
	}

	//파일이라면
	else {

		is_file = 1;		//파일이면 is_file = 1
		//사이즈 저장
		dsize = buf.st_size;
	}

	//절대경로로 변환
	realpath(path, real_path);

	//파일 탐색
	find_dfs(only_filename, real_path, path_array, &findex, dsize, is_file);
	
	
	printf("Index Size        Mode      Blocks Links UID  GID  Access           Change           Modify           Path\n");

	//검색된 파일의 절대경로가 담긴 배열 path_array에 있는 모든 파일 정보 출력
	for(int j = 0; j <= findex; j++) {

		//사이즈 초기화
		dsize = 0;
		//파일 정보 담기
		lstat(path_array[j], &buf);

		//접근권한 저장
		if(buf.st_mode & S_IRUSR)
			mode[0] = 'r';
		if(buf.st_mode & S_IWUSR)
			mode[1] = 'w';
		if(buf.st_mode & S_IXUSR)
			mode[2] = 'x';
		if(buf.st_mode & S_IRGRP)
			mode[3] = 'r';
		if(buf.st_mode & S_IWGRP)
			mode[4] = 'w';
		if(buf.st_mode & S_IXGRP)
			mode[5] = 'x';
		if(buf.st_mode & S_IROTH)
			mode[6] = 'r';
		if(buf.st_mode & S_IWOTH)
			mode[7] = 'w';
		if(buf.st_mode & S_IXOTH)
			mode[8] = 'x';

		//파일인지 디렉토리인지 구분
		//디렉토리라면
		if((buf.st_mode & S_IFMT) == S_IFDIR){

			//디렉토리 열고 크기 계산
			cal_size(path_array[j], &dsize);


			//디렉토리 정보 출력(index, size)
			printf("%-6d%-12ld", j, dsize);
		}

		//파일이라면
		else {

			//파일 정보 출력(index. size)
			printf("%-6d%-12ld", j, buf.st_size);
		}

		//파일 정보 출력(mode)
		printf("%s ", mode);
		//파일 정보 출력(block, nlink, uid, gid)
		printf("%-7ld%-6ld%-5u%-5u", buf.st_blocks, buf.st_nlink, buf.st_uid, buf.st_gid);
		//파일 정보 출력(access)
		ptm = localtime(&buf.st_atime);
		printf("%02d-%02d-%02d %02d:%02d   ", ptm->tm_year-100, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min);
		//파일 정보 출력(change)
		ptm = localtime(&buf.st_ctime);
		printf("%02d-%02d-%02d %02d:%02d   ", ptm->tm_year-100, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min);
		//파일 정보 출력(modify)
		ptm = localtime(&buf.st_mtime);
		printf("%02d-%02d-%02d %02d:%02d   ", ptm->tm_year-100, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min);
		//파일 정보 출력(path)
		printf("%s\n", path_array[j]);

	}

	if(findex == 0) {		//findex는 총 검색된 파일의 개수, 검색된 파일이 없다면 None 출력
		printf("(None)\n");
		return;
	}

	else {
		int index, err = 0;			//인덱스 입력받는 변수
		int option[4] = {0, };		//option을 저장할 변수 option

		char *tmp_index;		//index를 정수로 변환하기 전 문자열을 담을 변수
		char input[110];		//index와 option 입력받을 변수 input
		char *tok;				//input 문자열을 쪼갤 변수

		while(1) {
			//반복을 위한 변수 초기화
			input[0] = '\0';
			for(i = 0; i < 4; i++)
				option[i] = '\0';
			tok = NULL;
			err = 0;

			//공백 제외하고 입력받기
			printf(">> ");
			scanf("%[ \t]", input);
			scanf("%[^\n]", input);
			c = getchar();	//개행 제거

			if((tok = strtok(input, " ")) == NULL) //공백을 기준으로 input 쪼갠다.
				continue;	//index 구분

			tmp_index = tok;	//첫 부분은 index이므로 정수로 변환



			//index 부분에 숫자가 아닌 문자가 있으면 잘못된 입력이므로 에러메시지 출력
			for(i = 0; i < strlen(tmp_index); i++){
				if((tmp_index[i] < '0') || (tmp_index[i] > '9'))
					break;
			}
			if(i < strlen(tmp_index)) {
				printf("Error: 잘못된 입력\n\t>> [INDEX] [OPTION ...]\n\n");
				printf("\t[OPTION ... ]\n\t q: report only when files differ\n\t s: report when two files are the same\n\t i: ignore case differences in file contents\n\t r: recursively compare any subdirectories found\n");
				continue;
			}



			index = atoi(tmp_index);	//정수로 변환

			//index가 검색된 파일 개수보다 크거나 음수일 경우 에러메시지 출력
			if((index <= 0) || (index > findex)) {
				printf("Error: 잘못된 입력\n\t>> [INDEX] [OPTION ...]\n\n");
				printf("\t[OPTION ... ]\n\t q: report only when files differ\n\t s: report when two files are the same\n\t i: ignore case differences in file contents\n\t r: recursively compare any subdirectories found\n");
				continue;
			}


			//옵션 부분 저장
			for(i = 0; ((tok = strtok(NULL, " ")) != NULL) && (i < 50); i++) {
				//옵션이 두 자 이상이면 에러처리
				if((tok[1] != '\0')) {
					err = 1;
					break;
				}
				//옵션 저장, [0]은 q자리, [1]은 s자리, [2]는 i자리, [3]은 r자리이다.
				//옵션을 입력받으면 해당 옵션 자리에 1을 더해준다.
				if(tok[0] == 'q')
					option[0]++;
				else if(tok[0] == 's')
					option[1]++;
				else if(tok[0] == 'i')
					option[2]++;
				else if(tok[0] == 'r')
					option[3]++;
				//네 개 다 아니면 잘못된 입력
				else
					err = 1;
			}
			

			//옵션이 두 자 이상이면 에러처리
			if(err == 1) {
				printf("Error: 잘못된 입력\n\t>> [INDEX] [OPTION ...]\n\n");
				printf("\t[OPTION ... ]\n\t q: report only when files differ\n\t s: report when two files are the same\n\t i: ignore case differences in file contents\n\t r: recursively compare any subdirectories found\n");
				continue;
			}


			//디렉토리인지 확인하고 적절한 함수 실행
			if((buf.st_mode & S_IFMT) != S_IFDIR){
				//디렉토리가 아닌데 r옵션이 있다면 에러메시지 출력
				if((option[3] > 0)) {
					printf("Error: %d번 파일은 디렉토리가 아닙니다.(r옵션 사용 불가)\n\t>> [INDEX] [OPTION ...}\n\n", index);
				printf("\t[OPTION ... ]\n\t q: report only when files differ\n\t s: report when two files are the same\n\t i: ignore case differences in file contents\n\t r: recursively compare any subdirectories found\n");
					continue;
				}
				//파일비교하는 diff함수 실행
				diff(path_array[0], path_array[index], option);
			}
			else{
				//디렉토리 비교하는 diff_dir함수 실행
				diff_dir(path_array[0], path_array[index], option);
			}

			break;
		}
	}
	return;

}


//디렉토리 사이즈 계산하는 함수, 매개변수는 디렉토리 경로, 디렉토리 크기 담는 변수 주소
void cal_size(const char *path, long int *dsize_p){
	char tmp_path[1024];	//경로 저장하는 변수
	struct stat buf;		//파일 정보 저장하는 변수
	struct dirent **namelist;		//디렉토리 내의 파일 정보 저장하기 위한 함수
	int count;	//scandir() 사용한 경로의 파일 및 디렉토리 개수
	int i;		//for문을 위한 변수

	strcpy(tmp_path, path);
	//scandir()사용
	if((count = scandir(path, &namelist, filter, alphasort)) == -1){
		return;
	}



	for(i = 0; i < count; i++) {
		//절대 경로 파일이름 만들기
		strcpy(tmp_path, path);
		if(strcmp(tmp_path, "/"))
			strcat(tmp_path, "/");
		strcat(tmp_path, namelist[i]->d_name);
		//파일 정보 담기
		lstat(tmp_path, &buf);
		//이 파일이 디렉토리라면 그 하위 디렉토리 검사
		if((buf.st_mode & S_IFMT) == S_IFDIR) {
			cal_size(tmp_path, dsize_p);
		}
		//파일이라면 파일 크기 더함
		else {
			*dsize_p += buf.st_size;
		}
	}

	//메모리 헤제
	for(i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);

	return;
}


//같은 크기의 파일 찾는 함수, 매개변수는 파일이름, 파일 경로, 이름이 같은 파일 경로 저장하는 배열, 인덱스 주소, 파일 크기, is_file로 디렉토리(0)인지 파일인지(1) 구분
void find_dfs(const char *filename, const char *path, char path_array[][1024], int *index_p, const long int size, const int is_file){
	char tmp_path[1024];	//파일 경로와 이름 저장할 변수
	struct stat buf;		//stat정보 담을 변수
	struct dirent **namelist;	//scandir() 담을 변수
	int count;			//scandir() 담을 변수
	int i;				//for문을 위한 변수
	long int fsize;		//파일 사이즈 담는 변수

	strcpy(tmp_path, path);
	//scandir()사용
	if((count = scandir(path, &namelist, filter, alphasort)) == -1){
		fprintf(stderr, "scandir Error: %s in %s\n", strerror(errno), path);
		return;
	}


	else {
		for(i = 0; i < count; i++) {
			//절대경로 파일 이름 만들기
			strcpy(tmp_path, path);
			if(strcmp(tmp_path, "/"))
				strcat(tmp_path, "/");
			strcat(tmp_path, namelist[i]->d_name);
			//파일정보 담기
			lstat(tmp_path, &buf);


			//현재 검사하는 파일이 디렉토리인 경우
			if((buf.st_mode & S_IFMT) == S_IFDIR) {
				if(is_file == 0) {	//찾을 파일이 디렉토리가 맞으면 탐색
					fsize = 0;		//크기 초기화
					//디렉토리 크기 계산하고 이름과 크기가 같으면 저장 후 하위 디렉토리 검색
					cal_size(tmp_path, &fsize);
					//이름과 크기가 같은지 검사
					if((strcmp(namelist[i]->d_name, filename) == 0) && (fsize == size)){
						if(strcmp(tmp_path, path_array[0])) {						
							(*index_p)++;
							strcpy(path_array[*index_p], tmp_path);
						}
							
					}
					//하위 디렉토리 검사
					find_dfs(filename, tmp_path, path_array, index_p, size, is_file);
				}
				else {	//찾는 게 디렉토리가 아니면 바로 하위 디렉토리 탐색(is_file 변수의 값이 1인 경우이다.)
					find_dfs(filename, tmp_path, path_array, index_p, size, is_file);
				}
			}
			//현재 살펴보고 있는 파일이 디렉토리가 아닌 경우
			else {
				if(is_file == 0)	//is_file이 0 이면 디렉토리만 검색해야하므로 다음 검색
					continue;

				fsize = buf.st_size;

				//이름과 크기가 같은 파일 저장
				if((strcmp(namelist[i]->d_name, filename) == 0) && (fsize == size)){
						if(strcmp(tmp_path, path_array[0])) {						
							(*index_p)++;
							strcpy(path_array[*index_p], tmp_path);
						}
				}
			}
		}

		//메모리 해제
		for(i = 0; i < count; i++) {
			free(namelist[i]);
		}
		free(namelist);
		return;
	}

	return;

}


//diff구현하는 함수, 매개변수는 비교할 파일의 절대경로와 이름
void diff(const char *ori_file, const char *cmp_file, const int *option){
	FILE *fp;	//함수 포인터
	int num_ori = 0, num_cmp = 0, num_lcs = 0;	//각 파일의 라인 개수
	int table[1024][1024] = {0, };		//lcs 테이블 정보 저장하는 변수
	char ori_txt[1024][1024], cmp_txt[1024][1024], lcs_txt[1024][1024];	//각 파일의 내용을 저장

	int i = 0, j = 0, di_index = 0;	//for문을 위한 index i와 j, 경로를 나타내기 위한 index di_index
	int is_same = 1;			//파일의 내용이 같은 지 확인하는 변수

	if((fp = fopen(ori_file, "r")) == NULL) {
		fprintf(stderr, "Error: No such file or directory\n");
		exit(1);
	}
	//원본 파일의 내용을 저장, num_ori는 원본 파일의 라인 개수를 나타내게 된다.
	for(num_ori = 0; fgets(ori_txt[num_ori], 1024, fp) != NULL; num_ori++);

	fclose(fp);
	if((fp = fopen(cmp_file, "r")) == NULL) {
		fprintf(stderr, "Error: No such file or directory\n");
		exit(1);
	}
	//비교본 파일의 내용을 저장, num_cmp는 비교본 파일의 라인 개수를 나타내게 된다.
	for(num_cmp = 0; fgets(cmp_txt[num_cmp], 1024, fp) != NULL; num_cmp++);

	fclose(fp);

	

	//공통된 경로 삭제하기
	for(di_index = 0; di_index < 1024; di_index++) {
		if(ori_file[di_index] != cmp_file[di_index])
			break;
	}
	while(ori_file[di_index] != '/') {
		di_index--;
	}
	di_index++;		//두 문자열 ori_file, cmp_file은 di_index번째 원소까지는 완전히 같은 문자열(같은 경로)



	//i옵션을 줬을 경우 파일 내용이 같은 지 확인해서 is_same변수에 저장, stricmp함수 사용
	if(option[2] > 0) {
		if(num_ori != num_cmp)	//길이가 다르면 내용이 다른 파일, is_same변수에 0을 저장
			is_same = 0;
		else {
			for(i = 0; i < num_ori; i++) {		//모든 줄에 대하여 검사한다.
				if(stricmp(ori_txt[i], cmp_txt[i])) {	//대소문자 구분하지 않고 문자열 비교
					is_same = 0;	//다를 경우 is_same에 0을 저장한다.
					break;
				}
			}
			if(i == num_ori)		//모든 라인의 문자열이 같을 경우 is_same에 1을 저장한다.
				is_same = 1;
		}
	}
	//i옵션을 안 줬을 경우 파일 내용이 같은 지 확인해서 is_same변수에 저장, strcmp함수 사용
	else {
		if(num_ori != num_cmp)	//길이가 다르면 내용이 다른 파일, is_same에 0을 저장
			is_same = 0;
		else {
			for(i = 0; i < num_ori; i++) {		//모든 줄에 대햐여 검사
				if(strcmp(ori_txt[i], cmp_txt[i])) {	//strcmp함수로 비교
					is_same = 0;	//다를 경우 is_same에 0을 저장
					break;
				}
			}
			if(i == num_ori)	//모든 라인의 문자열이 같을 경우 is_same에 1 저장
				is_same = 1;
		}
	}

	//s옵션을 줬을 경우
	if((option[1] > 0) && is_same){		//s옵션인데 파일 내용이 같으면 출력
		printf("Files %s and %s are identical\n", &ori_file[di_index], &cmp_file[di_index]);
		return;
	}

	//q옵션을 줬을 경우
	if((option[0] > 0) && !is_same) {	//q옵션인데 파일 내용이 다르면 아래 문장만 출력 후 종료
		printf("Files %s and %s differ\n", &ori_file[di_index], &cmp_file[di_index]);
		return;
	}

	//lcs table 채우기
	//각 첫줄은 전부 0으로 채운다.
	for(j = 0; j <= num_cmp; j++)
		table[0][j] = 0;
	for(i = 0; i <= num_ori; i++)
		table[i][0] = 0;

	//i는 num_ori의 index, j는 num_cmp의 index
	//문자열이 같으면 대각선에서 +1, 다르면 왼쪽 혹은 위쪽 중 큰 수와 같게 한다.

	//i옵션을 줬을 경우 stricmp함수 사용
	if(option[2] > 0) {
		for(i = 1; i <= num_ori; i++) {
			for(j = 1; j <= num_cmp; j++) {
				if(stricmp(ori_txt[i-1], cmp_txt[j-1]) == 0){	//대소문자 구분없이 내용이 같을 경우 테이블에 대각선에서 +1 해서 저장
					table[i][j] = table[i-1][j-1] + 1;
					continue;
				}
				else {										//내용이 다를 경우 왼쪽이나 위쪽 중 큰 수로 저장
					if(table[i-1][j] > table[i][j-1])
						table[i][j] = table[i-1][j];
					else
						table[i][j] = table[i][j-1];
				}
			}
		}
	}

	//i옵션을 안 줬을 경우 strcmp함수 사용
	else {
		for(i = 1; i <= num_ori; i++) {
			for(j = 1; j <= num_cmp; j++) {
				if(strcmp(ori_txt[i-1], cmp_txt[j-1]) == 0){	//strcmp함수 사용해서 내용이 같을 경우 대각선에서 +1 해서 저장
					table[i][j] = table[i-1][j-1] + 1;
					continue;
				}
				else {									//내용이 다를 경우 왼쪽이나 위쪽 중 큰 수로 저장
					if(table[i-1][j] > table[i][j-1])
						table[i][j] = table[i-1][j];
					else
						table[i][j] = table[i][j-1];
				}
			}
		}
	}

	i = num_ori;	//테이블의 가장 오른쪽 밑부터 검사
	j = num_cmp;
	num_lcs = 0;
	//i는 num_ori의 index, j는 num_cmp의 index
	while(table[i][j] > 0){
		//왼쪽이 작으면 왼쪽으로
		if(table[i-1][j] > table[i][j-1])
			i--;
		//위쪽이 작으면 위쪽으로
		else if(table[i-1][j] < table[i][j-1])
			j--;
		//왼쪽이랑 위쪽이랑 같은데 현재랑도 같으면 왼쪽으로
		else if(table[i-1][j] == table[i][j])
			i--;
		//대각선으로 가면서 lcs에 저장
		else{
			strcpy(lcs_txt[num_lcs], ori_txt[i-1]);
			num_lcs++;
			i--;
			j--;
		}
	}		//이제 num_lcs는 lcs_txt의 원소의 개수를 나타냄


	//각 파일에서 lcs가 있는 line 저장 [0]의 원소는 0 
	int ori_line[1025] = {0}, cmp_line[1025] = {0};


	//i옵션을 줬을 경우 stricmp함수 사용
	if(option[2] > 0) {
		//i는 lcs_txt의 index, j는 ori_txt의 index, ori_txt에서 lcs인 문장을 선택하여 배열에 저장
		i = num_lcs-1;
		for(j = 0; i >= 0; j++) {
			//연속되는 문장이 최대가 되도록 lcs 문장을 선택하여 그 문장의 라인 수를 배열에 저장한다.
			if(((i < num_lcs-1) && (stricmp(lcs_txt[i+1], ori_txt[j-1]) == 0)) && (stricmp(lcs_txt[i], ori_txt[j]) == 0)) {
				ori_line[num_lcs-i-1] = j;	//연속되는 문장이 있다면 그 문장을 lcs로 선택하여 라인 수를 배열에 저장
				ori_line[num_lcs-i] = j + 1;
				i--;
			}
			else if(stricmp(lcs_txt[i], ori_txt[j]) == 0) {		//lcs문장의 라인 수를 배열에 저장
				ori_line[num_lcs-i] = j + 1;
				i--;
			}
		}

		//i는 lcs_txt의 index, j는 cmp_txt의 index, cmp_txt에서 lcs인 문장을 선택하여 배열에 저장
		i = num_lcs-1;
		for(j = 0; i >= 0; j++) {
			//연속되는 문장이 최대가 되도록 lcs 문장을 선택하여 그 문장의 라인 수를 배열에 저장한다.
			if(((i < num_lcs-1) && (stricmp(lcs_txt[i+1], cmp_txt[j-1]) == 0)) && (stricmp(lcs_txt[i], cmp_txt[j]) == 0)) {
				cmp_line[num_lcs-i-1] = j;	//연속되는 문장이 있다면 그 문장을 lcs로 선택하여 라인 수를 배열에 저장
				cmp_line[num_lcs-i] = j + 1;
				i--;
			}
			else if(stricmp(lcs_txt[i], cmp_txt[j]) == 0) {	//lcs문장의 라인 수를 배열에 저장
				cmp_line[num_lcs-i] = j + 1;
				i--;
			}
		}
	}
	//i옵션을 안 줬을 경우 strcmp함수 사용
	else {
		//i는 lcs_txt의 index, j는 ori_txt의 index, ori_txt에서 lcs인 문장을 선택하여 배열에 저장
		i = num_lcs-1;
		for(j = 0; i >= 0; j++) {
			//연속되는 문장이 최대가 되도록 lcs 문장을 선택하여 그 문장의 라인 수를 배열에 저장한다.
			if(((i < num_lcs-1) && (strcmp(lcs_txt[i+1], ori_txt[j-1]) == 0)) && (strcmp(lcs_txt[i], ori_txt[j]) == 0)) {
				ori_line[num_lcs-i-1] = j;	//연속되는 문장이 있다면 그 문장을 lcs로 선택
				ori_line[num_lcs-i] = j + 1;
				i--;
			}
			else if(strcmp(lcs_txt[i], ori_txt[j]) == 0) {
				ori_line[num_lcs-i] = j + 1;
				i--;
			}
		}

		//i는 lcs_txt의 index, j는 cmp_txt의 index, cmp_txt에서 lcs인 문장을 선택하여 배열에 저장
		i = num_lcs-1;
		for(j = 0; i >= 0; j++) {
			//연속되는 문장이 최대가 되도록 lcs 문장을 선택하여 그 문장의 라인 수를 배열에 저장한다.
			if(((i < num_lcs-1) && (strcmp(lcs_txt[i+1], cmp_txt[j-1]) == 0)) && (strcmp(lcs_txt[i], cmp_txt[j]) == 0)) {
				cmp_line[num_lcs-i-1] = j;	//연속되는 문장이 있다면 그 문장을 lcs로 선택하여 라인 수를 배열에 저장
				cmp_line[num_lcs-i] = j + 1;
				i--;
			}
			else if(strcmp(lcs_txt[i], cmp_txt[j]) == 0) {
				cmp_line[num_lcs-i] = j + 1;
				i--;
			}
		}
	}




	for(i = 1; i <= num_lcs; i++){
		if((ori_line[i] == (ori_line[i-1]+1)) && (cmp_line[i] > (cmp_line[i-1]+1))) {	//원본은 그대로 비교본에 내용이 추가됐을 경우 a
			if(cmp_line[i-1]+1 == (cmp_line[i]-1))		//append가 한 줄일 경우
				printf("%da%d\n", ori_line[i-1], cmp_line[i-1]+1);
			else		//append가 두 줄 이상일 경우
				printf("%da%d,%d\n", ori_line[i-1], cmp_line[i-1]+1, cmp_line[i]-1);
			for(j= cmp_line[i-1]; j < cmp_line[i]-1; j++)	//여기서 비교본 파일은 cmp_line[i-1]-1번째 줄부터 append 됐다, j가 index이기 때문에 +1된 값인 cmp_line[i-1]을 넣어준다.
				printf("> %s", cmp_txt[j]);
		}

		else if((ori_line[i] > (ori_line[i-1]+1)) && (cmp_line[i] == (cmp_line[i-1]+1))) {	//원본에서 삭제됐을 경우 d
			if(ori_line[i-1]+1 == (ori_line[i]-1))	//d가 한 줄일 경우
				printf("%dd%d\n", ori_line[i-1]+1, cmp_line[i-1]);
			else		//delete가 두 줄 이상일 경우
				printf("%d,%dd%d\n", ori_line[i-1]+1, ori_line[i]-1, cmp_line[i-1]);
			for(j = ori_line[i-1]; j < ori_line[i]-1; j++)		//원본 파일이 ori_line[i-1]-1번째 줄부터 delete 됐다, j가 index이기 때문에 +1된 값인 ori_line[i-1]을 넣어준다.
				printf("< %s", ori_txt[j]);
		}

		else if((ori_line[i] > (ori_line[i-1]+1)) && (cmp_line[i] > (cmp_line[i-1]+1))) {	//수정(c)일 경우

			//(ori_line[i-1]+1번째 줄 부터 ori_line[i]-1번째 줄까지)가 (cmp_line[i-1]+1번째 줄부터 cmp_line[i]-1번째 줄까지로 수정됐다.
			if((ori_line[i-1]+1 == (ori_line[i]-1)) && (cmp_line[i-1]+1 == (cmp_line[i]-1)))	//한 줄이 한 줄로 수정될 경우
				printf("%dc%d\n", ori_line[i]-1, cmp_line[i]-1);
			else if((ori_line[i-1]+1 != (ori_line[i]-1)) && (cmp_line[i-1]+1 == (cmp_line[i]-1)))	//여러 줄이 한 줄로 수정될 경우
				printf("%d,%dc%d\n", ori_line[i-1]+1, ori_line[i]-1, cmp_line[i]-1);
			else if((ori_line[i-1]+1 == (ori_line[i]-1)) && (cmp_line[i-1]+1 != (cmp_line[i]-1)))	//한 줄이 여러 줄로 수정될 경우
				printf("%dc%d,%d\n", ori_line[i]-1, cmp_line[i-1]+1, cmp_line[i]-1);
			else if((ori_line[i-1]+1 != (ori_line[i]-1)) && (cmp_line[i-1]+1 != (cmp_line[i]-1)))	//여러 줄이 여러 줄로 수정될 경우
				printf("%d,%dc%d,%d\n",ori_line[i-1]+1, ori_line[i]-1, cmp_line[i-1]+1, cmp_line[i]-1);
			for(j = ori_line[i-1]; j < ori_line[i]-1; j++)		//원본 파일이 ori_line[i-1]+1번째 줄부터 ori_line[i]-1번째 줄까지 수정됐고 j가 index이기 때문에 -1한 값을 출력해준다.
				printf("< %s", ori_txt[j]);
			printf("---\n");
			for(j = cmp_line[i-1]; j < cmp_line[i]-1; j++)		//비교본 파일도 마찬가지로 cmp_line[i-1]+1번째 줄부터 cmp_line[i]-1번째 줄까지 수정됐고 j가 index이기 때문에 -1한 값을 출력해준다.
				printf("> %s", cmp_txt[j]);

		}
	}



	//마지막에 추가되거나 삭제되거나 수정된 문장 점검, 위의 코드와 동일한 방식, 파일 끝에 개행이 없을 경우의 코드만 추가
	if((ori_line[num_lcs] < num_ori) && (cmp_line[num_lcs] < num_cmp)) {	//수정됐을 경우
		if(((ori_line[num_lcs]+1) == num_ori) && ((cmp_line[num_lcs]+1) == num_cmp))
			printf("%dc%d\n", num_ori, num_cmp);
		else if(((ori_line[num_lcs]+1) != num_ori) && ((cmp_line[num_lcs]+1) == num_cmp))
			printf("%d,%dc%d\n", ori_line[num_lcs]+1, num_ori, num_cmp);
		else if(((ori_line[num_lcs]+1) == num_ori) && ((cmp_line[num_lcs]+1) != num_cmp))
			printf("%dc%d,%d\n", num_ori, cmp_line[num_lcs]+1, num_cmp);
		else if(((ori_line[num_lcs]+1) != num_ori) && ((cmp_line[num_lcs]+1) != num_cmp))
			printf("%d,%dc%d,%d\n", ori_line[num_lcs]+1, num_ori, cmp_line[num_lcs]+1, num_cmp);
		for(j = ori_line[num_lcs]; j < num_ori; j++){
			if(ori_txt[j][strlen(ori_txt[j])-1] == '\n')
				printf("< %s", ori_txt[j]);
			else
				printf("< %s\n\\ No newline at end of file\n", ori_txt[j]);		//파일 끝에 개행이 없을 경우 출력
		}
		printf("---\n");
		for(j = cmp_line[num_lcs]; j < num_cmp; j++){
			if(cmp_txt[j][strlen(cmp_txt[j])-1] == '\n')
				printf("> %s", cmp_txt[j]);
			else
				printf("> %s\n\\ No newline at end of file\n", cmp_txt[j]);		//파일 끝에 개행이 없을 경우 출력
		}
	}
	else if((ori_line[num_lcs] == num_ori) && (cmp_line[num_lcs] < num_cmp)) {	//추가됐을 경우
		if(cmp_line[num_lcs]+1 == num_cmp)
			printf("%da%d\n", num_ori, num_cmp);
		else
			printf("%da%d,%d\n", num_ori, cmp_line[num_lcs]+1, num_cmp);
		for(j = cmp_line[num_lcs]; j < num_cmp; j++){
			if(cmp_txt[j][strlen(cmp_txt[j])-1] == '\n')
				printf("> %s", cmp_txt[j]);
			else
				printf("> %s\n\\ No newline at end of file\n", cmp_txt[j]);		//파일 끝에 개행이 없을 경우 출력
		}
	}
	else if((ori_line[num_lcs] < num_ori) && (cmp_line[num_lcs] == num_cmp)) {	//삭제됐을 경우
		if(ori_line[num_lcs]+1 == num_ori)
			printf("%dd%d\n", num_ori, num_cmp);
		else
			printf("%d,%dd%d\n", ori_line[num_lcs]+1, num_ori, num_cmp);
		for(j = ori_line[num_lcs]; j < num_ori; j++) {
			if(ori_txt[j][strlen(ori_txt[j])-1] == '\n')
				printf("< %s", ori_txt[j]);
			else
				printf("< %s\n\\ No newline at end of file\n", ori_txt[j]);		//파일 끝에 개행이 없을 경우 출력
		}
	}

	return;
}

void diff_dir(const char *ori_dir, const char *cmp_dir, const int *option){
	struct dirent **ori_namelist, **cmp_namelist;	//scandir() 담을 변수
	int ori_count, cmp_count;			//scandir() 담을 변수
	int i = 0, j = 0;		//for문을 위한 변수
	int di_index = 0, only = 1;		//경로 중 공통된 부분을 제외하기 위한 변수 di_index, 해당 파일이 한 쪽 디렉토리에만 있는지 확인하여 저장하는 변수 only

	char ori_realpath[1024], cmp_realpath[1024];	//각 파일의 절대경로
	struct stat ori_buf, cmp_buf;		//각 파일의 정보를 저장하는 변수


	if((ori_count = scandir(ori_dir, &ori_namelist, filter, alphasort)) == -1){
		fprintf(stderr, "scandir Error: %s\n", strerror(errno));
		return;
	}
	if((cmp_count = scandir(cmp_dir, &cmp_namelist, filter, alphasort)) == -1){
		fprintf(stderr, "scandir Error: %s\n", strerror(errno));
		return;
	}

	
	//공통된 경로 삭제하기
	for(di_index = 0; di_index < 1024; di_index++) {
		if(ori_dir[di_index] != cmp_dir[di_index])
			break;
	}
	while(ori_dir[di_index] != '/') {
		di_index--;
	}
	di_index++;		//이제 문자열 ori_dir, cmp_dir에서 di_index번째부터 다른 경로 이름이 다르다.


	for(i = 0; i < ori_count; i++) {
		only = 1;
		for(j = 0; j < cmp_count; j++) {
			if(strcmp(ori_namelist[i]->d_name, cmp_namelist[j]->d_name) == 0) {
				//원본 디렉토리의 절대경로와 파일 정보 ori_buf에 저장
				strcpy(ori_realpath, ori_dir);
				strcat(ori_realpath, "/");
				strcat(ori_realpath, ori_namelist[i]->d_name);
				lstat(ori_realpath, &ori_buf);
				//비교본 디렉토리의 절대경로와 파일 정보 cmp_buf에 저장
				strcpy(cmp_realpath, cmp_dir);
				strcat(cmp_realpath, "/");
				strcat(cmp_realpath, cmp_namelist[j]->d_name);
				lstat(cmp_realpath, &cmp_buf);




				//둘 다 파일일 경우
				if((!S_ISDIR(ori_buf.st_mode)) && (!S_ISDIR(cmp_buf.st_mode))) {
					//파일 내용이 다를 경우 차이점 출력
					if(!is_sametxt(ori_realpath, cmp_realpath, option)) {	//내용이 다르면 차이 출력

						if((!option[0]) && (!option[2]) &&(!option[3]))	//q옵션 없고 다른 옵션도 없을 경우
							printf("diff %s %s\n", &ori_realpath[di_index], &cmp_realpath[di_index]);

						else if((!option[0]) && (option[2]) &&(!option[3]))	//q옵션 없고 i옵션만 있을 경우
							printf("diff -i %s %s\n", &ori_realpath[di_index], &cmp_realpath[di_index]);

						else if((!option[0]) && (!option[2]) &&(option[3]))	//q옵션 없고 r옵션만 있을 경우
							printf("diff -r %s %s\n", &ori_realpath[di_index], &cmp_realpath[di_index]);

						else if((!option[0]) && (option[2]) &&(option[3]))	//q옵션 없고 i,r 옵션이 있을 경우
							printf("diff -i -r %s %s\n", &ori_realpath[di_index], &cmp_realpath[di_index]);

						//diff결과 출력
						diff(ori_realpath, cmp_realpath, option);	//diff 함수에는 절대경로 전체를 넣어줘야 한다.
					}

					//파일 내용이 같은 경우 s옵션이 있다면 결과 출력
					else {
						if(option[1] > 0)
							printf("Files %s and %s are identical\n", &ori_realpath[di_index], &cmp_realpath[di_index]);
					}
				}
				//하나는 파일 하나는 디렉토리일 경우
				else if((!S_ISDIR(ori_buf.st_mode)) && (S_ISDIR(cmp_buf.st_mode)))
					printf("File %s is a regular file while file %s is a directory file\n", &ori_realpath[di_index], &cmp_realpath[di_index]);
				else if((S_ISDIR(ori_buf.st_mode)) && (!S_ISDIR(cmp_buf.st_mode)))
					printf("File %s is a directory file while file %s is a regular file\n", &ori_realpath[di_index], &cmp_realpath[di_index]);
				//둘 다 디렉토리일 경우
				else {
					if(option[3] > 0)	//r옵션일 경우
						diff_dir(ori_realpath, cmp_realpath, option);	//diff_dir 함수에는 절대경로 전체를 넣어줘야 한다.
					else
						printf("Common subdirectories: %s and %s\n", &ori_realpath[di_index], &cmp_realpath[di_index]);
				}

				//같은 이름의 파일이 여러 개일 수 없으므로 다음 for문 탈출
				only = 0;
				break;
			}
		}

		//파일이 원본 디렉토리에만 있다면 출력
		if(only == 1)
			printf("Only in %s: %s\n", &ori_dir[di_index], ori_namelist[i]->d_name);
	}


	//비교본 디렉토리에만 파일이 있는 경우 출력하는 코드
	for(i = 0; i < cmp_count; i++) {
		only = 1;
		for(j = 0; j < ori_count; j++) {
			if(strcmp(cmp_namelist[i]->d_name, ori_namelist[j]->d_name) == 0) {
				only = 0;
				break;
			}
		}

		//파일이 비교본 디렉토리에만 있다면 출력
		if(only == 1)
			printf("Only in %s: %s\n", &cmp_dir[di_index], cmp_namelist[i]->d_name);
	}



	//메모리 해제
	for(i = 0; i < ori_count; i++) {
		free(ori_namelist[i]);
	}
	free(ori_namelist);
	//메모리 해제
	for(i = 0; i < cmp_count; i++) {
		free(cmp_namelist[i]);
	}
	free(cmp_namelist);

	return;
}

int is_sametxt(const char *ori_file, const char *cmp_file, const int *option) {
	FILE *fp;		//함수 포인터
	char ori_txt[1024][1024] = {0, }, cmp_txt[1024][1024] = {0, };	//각 파일의 내용을 저장
	int num_ori, num_cmp;		//각 파일의 라인 개수를 저장하는 변수
	int i;		//for문을 위한 변수

	if((fp = fopen(ori_file, "r")) == NULL) {
		fprintf(stderr, "Error: No such file or directory\n");
		exit(1);
	}
	//원본 파일의 내용을 저장, num_ori는 원본 파일의 라인 개수를 나타내게 된다.
	for(num_ori = 0; fgets(ori_txt[num_ori], 1024, fp) != NULL; num_ori++);
	fclose(fp);

	if((fp = fopen(cmp_file, "r")) == NULL) {
		fprintf(stderr, "Error: No such file or directory\n");
		exit(1);
	}
	//비교본 파일의 내용을 저장, num_cmp는 비교본 파일의 라인 개수를 나타내게 된다.
	for(num_cmp = 0; fgets(cmp_txt[num_cmp], 1024, fp) != NULL; num_cmp++);
	fclose(fp);

	//길이가 다르면 내용이 다르므로 0반환
	if(num_ori != num_cmp)
		return 0;

	//i옵션일 경우 stricmp함수로 비교한다.
	if(option[2] > 0) {
		for(i = 0; i <num_ori; i++) {
			//내용이 다르면 break
			if(stricmp(ori_txt[i], cmp_txt[i]))
				break;
		}
	}
	//i옵션이 아닐 경우 strcmp함수로 비교한다.
	else {
		for(i = 0; i < num_ori; i++) {
			//내용이 다르면 break
			if(strcmp(ori_txt[i], cmp_txt[i]))
				break;
		}
	}

	//i값이 num_ori와 같다는 것은 for문 안의 if문에 걸리지 않았다는 뜻이므로 완전히 같은 내용이다.
	if(i == num_ori)
		return 1;
	else
		return 0;
}

//대소문자 구분없이 같은 문자열인지 비교하는 함수, 매개변수는 비교할 두 문자열
int stricmp(const char *ori, const char *cmp) {
	int length = 0;
	int gap;
	int i;

	//길이가 다르면 다른 문자열
	length = strlen(ori);
	if(length != strlen(cmp))
		return 1;

	//대소문자간의 아스키코드 값 차이를 저장한다.
	gap = 'a' - 'A';

	for(i = 0; i < length; i++) {
		//문자가 다를 때 알파벳인지 검사
		if(ori[i] != cmp[i]) {
			//ori[i]가 대문자일 경우 cmp[i]가 같은 문자 소문자인지 검사
			if(((ori[i] >= 'A') && (ori[i] <= 'Z')) && ((ori[i] + gap) == cmp[i]))
				continue;
			//ori[i]가 소문자일 경우 cmp[i]가 같은 문자 대문자인지 검사
			else if(((ori[i] >= 'a') && (ori[i] <= 'z')) && ((ori[i] - gap) == cmp[i]))
				continue;
			//둘 다 아닐경우 다른 문자열
			else
				break;
		}
	}
	if(i == length)
		return 0;
	else
		return 1;
}
