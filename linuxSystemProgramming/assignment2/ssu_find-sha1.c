#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include<errno.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<openssl/sha.h>

extern char **environ;

struct node {
	char path[4097];
	off_t size;
	int check;			//fake list에서 real list에 넣을 때 검사했는지 확인하는 부분
	struct node *next;	//size 순으로 바로 다음 크기의 노드를 가리키는 포인터
	struct node *list;	//같은 size의 연결리스트 포인터
	struct node *last;	//같은 size의 연결리스트의 마지막으로 들어온 노드를 가리키는 포인터
};

struct queue {
	struct node *front, *rear;
};

//큐와 관련된 함수입니다.
void init_queue(struct queue *);
int is_empty(struct queue *);
void bfs_enqueue(struct queue *, char *);
void bfs_dequeue(struct queue *, char *);

//리스트를 만드는 함수입니다.
//fake_list는 2차원 리스트로 크기가 같은 파일끼리 연결되어 있습니다.
//real_list는 2차원 리스트로 크기와 hash값이 같은 파일끼리 연결되어 있습니다.
void make_fake_list(struct node **, char *, off_t);
void make_real_list(struct node **, struct node **);

//정수를 콤마찍어서 문자열로 반환하는 함수입니다.
char *comma(off_t , char *);

//해당 중복 파일 리스트를 인자로 받아 f옵션(mtime 가장 최근을 제외하고 모두 삭제)을 구현하는 함수입니다.
void option_f(struct node **, int);
void option_t(struct node **, int);

//scandir() 함수에서 '.'디렉토리와 ".." 디렉토리를 거르는 함수입니다.
int filter(const struct dirent *entry) {
	return (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."));
}

int main(int argc, char *argv[]) {
	//FILE_EXTENSION: argv[1], MINSIZE: argv[2], MAXSIZE: argv[3], TARGET_DIRECTORY: argv[4]
	off_t min_size, max_size;
	int count, i;	//인덱스 또는 잠깐 쓰는 정수들
	char target_directory[4097], real_path[4097], tmp_path[4097];	//인자로 들어온 디렉토리 경로의 절대 경로를 담는 변수와 경로 정보를 잠시 담아두는 변수입니다.
	struct dirent **namelist;	//scandir()함수 호출 후 하위 디렉토리를 담는 변수
	struct stat buf;		//stat 구조체 변수
	struct node *fake_head = NULL;	//fake_list의 첫 노드를 가리키는 포인터
	struct queue *bfsQ = (struct queue *)malloc(sizeof(struct queue));	//큐 만듦
	char trashpath[1024];
	strcpy(trashpath, getenv("PWD"));
	strcat(trashpath, "/trash20182610");

	struct timeval start_time, end_time;
	long int diff_time_sec, diff_time_usec;
	gettimeofday(&start_time, NULL);

	//에러처리
	if(argc != 5) {
		fprintf(stderr, "fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
		exit(1);
	}

	//최대값 최소값을 정수로 변환합니다.
	min_size = atol(argv[2]);
	max_size = atol(argv[3]);

	//'~'가 있으면 홈 디렉토리 경로로 바꿔줍니다.
	if(argv[4][0] == '~') {
		strcpy(target_directory, getenv("HOME"));
		strcat(target_directory, &argv[4][1]);
	}
	else {
		strcpy(target_directory, argv[4]);
	}
	//절대 경로로 바꿔줍니다.
	realpath(target_directory, real_path);

	//큐를 만들고 큐에 넣어 줄 경로를 만들어 tmp_path에 저장했다가 큐에 집어넣습니다.
	init_queue(bfsQ);
	if((count = scandir(real_path, &namelist, filter, alphasort)) == -1) {
		fprintf(stderr, "Error: %s for %s\n", strerror(errno), real_path);
		exit(1);
	}

	//경로를 만들어 큐에 삽입
	for(i = 0; i < count; i++) {
		strcpy(tmp_path, real_path);
		if(strcmp(tmp_path, "/"))
			strcat(tmp_path, "/");
		strcat(tmp_path, namelist[i]->d_name);
		bfs_enqueue(bfsQ, tmp_path);
	}
	//namelist는 다 썼으니까 메모리 해제
	for(i = 0; i < count; i++)
		free(namelist[i]);
	free(namelist);

	//모든 파일을 bfs로 탐색합니다.
	while(!is_empty(bfsQ)) {
		//큐에서 하나 빼서 경로를 tmp_path에 잠시 저장합니다.
		bfs_dequeue(bfsQ, tmp_path);
		lstat(tmp_path, &buf);

		//tmp_path가 디렉토리라면 하위 디렉토리를 큐에 담습니다.
		if(S_ISDIR(buf.st_mode)) {
			if(!strcmp(tmp_path, "/proc") || !strcmp(tmp_path, "/run") || !strcmp(tmp_path, "/sys") || !strcmp(tmp_path, trashpath))
				continue;

			if((count = scandir(tmp_path, &namelist, filter, alphasort)) == -1) {
				fprintf(stderr, "Error: %s for %s\n", strerror(errno), tmp_path);
				continue;
			}
			//tmp_path를 저장할 변수 path_buf를 만들고 복사해둡니다.
			char path_buf[4097];
			strcpy(path_buf, tmp_path);
			for(i = 0; i < count; i++) {
				//tmp_path가 손상됐으므로
				//저장해둔 path_buf를 가져와서 하위 파일들의 경로를 만들어줍니다.
				strcpy(tmp_path, path_buf);
				strcat(tmp_path, "/");
				strcat(tmp_path, namelist[i]->d_name);
				bfs_enqueue(bfsQ, tmp_path);
			}
			//메모리 해제
			for(i = 0; i < count; i++)
				free(namelist[i]);
			free(namelist);
		}

		else if(S_ISREG(buf.st_mode)) {
			//파일 크기가 탐색 범위 안에 있을 경우만 탐색합니다.
			if((buf.st_size >= min_size) && (buf.st_size <= max_size)) {

				//모든 파일을 탐색
				if(!strcmp(argv[1], "*")) {
					//tmp_path가 일반 파일이라면 같은 파일을 찾아 fake list에 담는 함수 실행
					make_fake_list(&fake_head, tmp_path, buf.st_size);
				}

				//특정 확장자의 파일만 탐색하는 경우
				else {

					//확장자 부분의 인덱스를 뽑아냅니다.
					for(i = 0; tmp_path[i] != '.' && tmp_path[i] != '\0'; i++);

					//tmp_path인 파일이 확장자를 갖고 있으며 탐색하려는 확장자인 경우 탐색
					if(tmp_path[i] == '.' && !strcmp(&tmp_path[i], &argv[1][1])) {
						//tmp_path가 일반 파일이라면 같은 파일을 찾아 fake list에 담는 함수 실행
						make_fake_list(&fake_head, tmp_path, buf.st_size);
					}
				}
			}
		}
	}

	//큐 메모리 해제
	while(!is_empty(bfsQ))
		bfs_dequeue(bfsQ, tmp_path);
	free(bfsQ);
	bfsQ = NULL;

	struct node *cur;		//리스트를 관리할 포인터 변수들 cur, ori, cmp
	struct node *ori, *cmp;
	struct node *real_head;	//real list의 첫 노드를 가리키는 포인터
	struct tm *ptm;
	int j;	//index로 쓸 변수
	real_head = NULL;
	make_real_list(&real_head, &fake_head);	//fake list를 가지고 real list를 만듭니다.

	gettimeofday(&end_time, NULL);
	diff_time_sec = end_time.tv_sec - start_time.tv_sec;
	if((diff_time_usec = end_time.tv_usec - start_time.tv_usec) < 0) {
		diff_time_usec += 1000000;
		diff_time_sec--;
	}

	//최종 리스트 출력
	cur = real_head;
	for(i = 1; cur != NULL; i++) {
		cmp = cur->list;
		if(!strcmp(cur->path, "")) {
			ori = cur;
			for(cur = real_head; cur->next != ori; cur = cur->next);
			cur->next = ori->next;
			free(ori);
			cur = cur->next;
			i--;
			continue;
		}
		sprintf(tmp_path, "%ld", cur->size);
		printf("---- Identical files #%d (%s bytes -%s)----\n", i, comma(cur->size, tmp_path), cur->path);
		j = 1;
		for(cmp = cur->list; cmp != NULL; j++) {
			lstat(cmp->path, &buf);
			ptm = localtime(&buf.st_mtime);
			printf("[%d] %s (mtime : %04d-%02d-%02d %02d:%02d:%02d) (atime : ", j, cmp->path, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			ptm = localtime(&buf.st_atime);
			printf("%04d-%02d-%02d %02d:%02d:%02d)\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

			cmp = cmp->list;
		}
		cur = cur->next;
		printf("\n");
	}

	//이때 index i의 값은 중복 파일 리스트의 개수보다 1 큽니다.
	//i가 2보다 작으면 중복되는 파일이 없을 경우
	if(i < 2) {
		//메시지 출력하고 프로세스 종료합니다.
		printf("No duplicates in %s\n\n", real_path);
		printf("Searching time: %ld:%06ld(sec:usec)\n\n", diff_time_sec, diff_time_usec);
		//메모리 해제
		while(real_head != NULL) {
			ori = real_head;
			cmp = real_head->list;
			real_head = real_head->next;
			while(cmp != NULL) {
				free(ori);
				ori = cmp;
				cmp = cmp->list;
			}
			free(ori);
		}
		exit(0);
	}

	//탐색 소요시간 출력
	printf("Searching time: %ld:%06ld(sec:usec)\n\n", diff_time_sec, diff_time_usec);

	int input_len = 0, index, set_index, list_index;
	int err = 1;
	char word[25];
	char c;
	char option;

	//tmp_path를 잠시 입력받은 문자열을 저장하는 용도로 사용합니다.
	memset(tmp_path, 0, sizeof(tmp_path));
	while(1) {
		//변수 초기화
		set_index = 0;
		list_index = 0;
		option = '\0';
		memset(tmp_path, 0, sizeof(tmp_path));
		err = 0;
		memset(word, 0, sizeof(word));

		//옵션을 입력받습니다.
		printf(">> ");
		scanf("%[ \t]", tmp_path);
		scanf("%[^\n]", tmp_path);
		c = getchar();

		input_len = strlen(tmp_path);
		if(!strcmp(tmp_path, ""))
			continue;

		//공백을 찾습니다.
		index = 0;
		while(tmp_path[index] != ' ' && tmp_path[index] != '\t' && tmp_path[index] != '\0')
			index++;

		if(index > 23) {
			//잘못된 입력 에러처리 합니다.
			//첫 마디는 파일 리스트 번호이므로 23자리 이상이 될 수 없음.
			printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
			continue;
		}
		else {
			//문자열로 만듭니다.
			for(j = 0; j < index; j++)
				word[j] = tmp_path[j];
			word[index] = '\0';
		}
		//exit 명령어를 입력받았으면 프로세스 종료
		if(!strcmp(word, "exit")) {
			printf(">>Back to Prompt\n");
			while(real_head != NULL) {
				ori = real_head;
				cmp = real_head->list;
				real_head = real_head->next;
				while(cmp != NULL) {
					free(ori);
					ori = cmp;
					cmp = cmp->list;
				}
				free(ori);
			}
			exit(0);
		}

		//exit이 아니면서 자연수가 아닌 문자가 입력되면 에러
		for(j = 0; j < index; j++) {
			if(word[j] > '9' || word[j] < '0') {
				err = 1;
				break;
			}
		}
		//자연수가 아닌 문자가 입력되었거나 잘못된 index가 입력되었을 경우 에러메시지
		if((err == 1) || ((set_index = atoi(word)) <= 0) || set_index >= i) {
			printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
			continue;
		}

		//이제 옵션 인덱스를 뽑아냅니다.
		while(tmp_path[index] == ' ' || tmp_path[index] == '\t')
			index++;

		option = tmp_path[index];
		index++;
		//d, i, t, f 중 하나가 아니면 에러메시지를 띄웁니다.(두 자 이상도 에러메시지)
		if(tmp_path[index] != ' ' && tmp_path[index] != '\t' && tmp_path[index] != '\0') {
			printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
			continue;
		}
		else if(option != 'd' && option != 'i' && option != 't' && option != 'f') {
			printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
			continue;
		}

		//d 옵션일 경우
		if(option == 'd') {
			//이제 [LIST_INDEX] 부분을 뽑아냅니다.(d)
			while(tmp_path[index] == ' ' || tmp_path[index] == '\t')
				index++;
			j = 0;
			while(tmp_path[index] != ' ' && tmp_path[index] != '\t' && tmp_path[index] != '\0') {
				if(tmp_path[index] > '9' || tmp_path[index] < '0') {
					err = 1;
					break;
				}
				word[j] = tmp_path[index];
				j++;
				index++;
			}
			//잘못된 숫자 또는 입력이면 에러처리(d)
			word[j] = '\0';
			if((err == 1) || ((list_index = atoi(word)) <= 0)) {
				printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
				continue;
			}
			//다음 인자가 더 들어왔는지 검사(d)
			while(tmp_path[index] == ' ' || tmp_path[index] == '\t')
				index++;
			//더 있다면 잘못된 입력이므로 에러 메시지를 띄웁니다.(d)
			if(tmp_path[index] != '\0') {
				printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
				continue;
			}

			//cur을 [SET_INDEX]번 째 중복 파일 리스트를 가리키도록 만듭니다.(d)
			cur = real_head;
			for(j = 1; cur != NULL; j++) {
				cmp = cur->list;
				if(j == set_index)
					break;
				cur = cur->next;
			}

			//ori는 [SET_INDEX] 중복 파일 리스트의 시작부분을 가리키고(d)
			//cmp는 그 리스트에서 첫 파일이 담긴 노드를 가리킵니다.(d)
			ori = cur;
			cmp = cur->list;
			//그 다음 [LIST_INDEX]번 째 파일을 cmp가 가리키도록 합니다.(d)
			for(j = 1; j < list_index; j++) {
				cmp = cmp->list;
				ori = ori->list;
				if(cmp == NULL) {
					err = 1;
					break;
				}
			}
			//[LIST_INDEX]가 파일 수보다 크다면 에러처리
			if(err == 1) {
				printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
				continue;
			}
			

			//cmp에 저장된 파일명으로 파일을 삭제하고 리스트에서도 제거해줍니다.(d)
			if(unlink(cmp->path) < 0) {
				fprintf(stderr, "삭제할 수 없는 파일입니다.\n");
				cmp = cmp->list;
				continue;
			}
			printf("\"%s\" has been deleted in #%d\n\n", cmp->path, set_index);
			//리스트에서 제거(d)
			ori->list = cmp->list;
			free(cmp);
			
			//만약 파일 삭제 후 중복 파일이 없어졌다면 리스트에서도 해시값을 빼줍니다.(d)
			if(cur->list == NULL || cur->list->list == NULL) {
				if(set_index == 1) {
					cur = real_head;

					free(cur->list);
					ori = cur->next;
					free(cur);
					cur = ori;
					real_head = cur;
					if(real_head != NULL && !strcmp(real_head->path, "")) {
						free(real_head);
						real_head = NULL;
					}
				}

				else {
				//여기서는 cur을 [SET_INDEX] 바로 이전을 가리키도록 합니다.(d)
					cur = real_head;
					for(j = 1; cur != NULL; j++) {
						cmp = cur->list;
						if(j == set_index-1)
							break;
						cur = cur->next;
					}
					//[SET_INDEX]에 파일이 하나 있다면 그 노드의 메모리 해제를 먼저 해줍니다.(d)
					if((cmp = cur->next->list) != NULL) {
						free(cmp);
					}

					//hash값이 든 노드를 리스트에서 제거 후 메모리 해제(d)
					cmp = cur->next;
					cur->next = cmp->next;
					free(cmp);
				}
			}

			//최종 리스트 출력(d)
			cur = real_head;
			for(i = 1; cur != NULL; i++) {
				cmp = cur->list;
				sprintf(tmp_path, "%ld", cur->size);
				printf("---- Identical files #%d (%s bytes -%s)----\n", i, comma(cur->size, tmp_path), cur->path);
				j = 1;
				for(cmp = cur->list; cmp != NULL; j++) {
					lstat(cmp->path, &buf);
					ptm = localtime(&buf.st_mtime);
					printf("[%d] %s (mtime : %04d-%02d-%02d %02d:%02d:%02d) (atime : ", j, cmp->path, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
					ptm = localtime(&buf.st_atime);
					printf("%04d-%02d-%02d %02d:%02d:%02d)\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

					cmp = cmp->list;
				}
				cur = cur->next;
				printf("\n");
			}

			if(i < 2) {
				//메모리 해제(d)
				while(real_head != NULL) {
					ori = real_head;
					cmp = real_head->list;
					real_head = real_head->next;
					while(cmp != NULL) {
						free(ori);
						ori = cmp;
						cmp = cmp->list;
					}
					free(ori);
				}
				exit(0);
			}
			continue;
		}

		//d옵션이 아닌데 다음 입력이 있으면 에러
		while(tmp_path[index] == ' ' || tmp_path[index] == '\t')
			index++;
		if(tmp_path[index] != '\0') {
			printf("  >> [SET_INDEX] [OPTION ...]\n     [OPTION ...]\n     d [LIST_IDX] : delete [LIST_IDX] file\n     i: ask for confirmation before delete\n     f: force delete except the recently modified file\n     t: force move to Trash except the recently modified file\n     >> exit\n\n");
			continue;
		}

		//i 옵션일 경우
		else if(option == 'i') {
			//cur을 [SET_INDEX]번 째 중복 파일 리스트를 가리도록 만듭니다.(i)
			cur = real_head;
			for(j = 1; cur != NULL; j++) {
				cmp = cur->list;
				if(j == set_index)
					break;
				cur = cur->next;
			}

			//cur은 이제 i번째 중복 파일 리스트를 가리킵니다.(i)
			j = 1;
			//처음엔 cmp는 첫 번째 파일을 가리킵니다.(i)
			//ori는 cmp 이전 노드를 가리킵니다.(i)
			ori = cur;
			for(cmp = cur->list; cmp != NULL;) {
				printf("Delete \"%s\"? [y/n] ", cmp->path);
				c = getchar();
				//입력받은 값이 y이면 삭제합니다.(i)
				if(c == 'y' || c == 'Y') {

					//삭제할 노드에 저장된 경로 이름으로 파일을 삭제합니다.(i)
					if(unlink(cmp->path) < 0) {
						fprintf(stderr, "삭제할 수 없는 파일입니다.\n");
						cmp = cmp->list;
						continue;
					}

					//리스트에서도 제거해주고 다음 파일 삭제 여부를 묻기 위해(i)
					//cmp를 다음 노드를 가리키도록 합니다.(i)
					ori->list = cmp->list;
					free(cmp);
					cmp = ori->list;
				}
				else if(c == 'n' || c == 'N') {
					ori = ori->list;
					cmp = ori->list;
				}
				while((c = getchar()) != '\n');
			}

			//만약 파일 삭제 후 중복 파일이 없어졌다면 리스트에서도 해시값을 빼줍니다.(i)
			if(cur->list == NULL || cur->list->list == NULL) {
				if(set_index == 1) {
					cur = real_head;

					free(cur->list);
					ori = cur->next;
					free(cur);
					cur = ori;
					real_head = cur;
					if(real_head != NULL && !strcmp(real_head->path, "")) {
						free(real_head);
						real_head = NULL;
					}
				}
				else {
					//여기서는 cur을 [SET_INDEX] 바로 이전을 가리키도록 합니다.(i)
					cur = real_head;
					for(j = 1; cur != NULL; j++) {
						cmp = cur->list;
						if(j == set_index-1)
							break;
						cur = cur->next;
					}
					//[SET_INDEX]에 파일이 하나 있다면 그 노드의 메모리 해제를 먼저 해줍니다.(i)
					if((cmp = cur->next->list) != NULL) {
						free(cmp);
					}

					//hash값이 든 노드를 리스트에서 제거 후 메모리 해제(i)
					cmp = cur->next;
					cur->next = cmp->next;
					free(cmp);
				}
			}
			printf("\n");
		}


		else if(option == 'f') {
			//set index가 1인 경우에는 데이터 삭제 후 real head를 옮겨줘야 합니다.(f)
			if(set_index == 1) {
				//real_head로 f옵션을 수행하고 메모리를 해제해줍니다.(f)
				//그리고 real_head를 옮겨줍니다.
				cur = real_head->next;
				option_f(&real_head, 1);
				free(real_head);
				real_head = cur;
			}

			else {
				//cur을 set index 바로 이전을 가리키도록 합니다.(f)
				cur = real_head;
				for(j = 1; cur != NULL; j++) {
					cmp = cur->list;
					if(j == set_index-1)
						break;
					cur = cur->next;
				}
				//set index에 해당하는 중복 파일 리스트를 인자로 넘겨 f옵션을 수행합니다.(f)
				option_f(&(cur->next), set_index);
				//이제 중복 파일이 없어졌으므로 리스트에서 제거해줍니다.(f)
				ori = cur->next;
				cur->next = ori->next;
				free(ori);
				ori = NULL;
			}
		}

		else if(option == 't') {
			//set index가 1인 경우에는 데이터 삭제 후 real head를 옮겨줘야합니다.(t)
			if(set_index == 1) {
				//real_head로 f옵션을 수행하고 메모리를 해제해줍니다.(t)
				//그리고 real_head를 옮겨줍니다.(t)
				cur = real_head->next;
				option_t(&real_head, 1);
				free(real_head);
				real_head = cur;
			}

			else {
				//cur을 set index 바로 이전을 가리키도록 합니다.(t)
				cur = real_head;
				for(j = 1; cur != NULL; j++) {
					cmp = cur->list;
					if(j == set_index-1)
						break;
					cur = cur->next;
				}
				//set index에 해당하는 중복 파일 리스트를 인자로 넘겨 f옵션을 수행합니다.(t)
				option_t(&(cur->next), set_index);
				//이제 중복 파일이 없어졌으므로 리스트에서 제거해줍니다.(t)
				ori = cur->next;
				cur->next = ori->next;
				free(ori);
				ori = NULL;
			}
		}	//t옵션 if문 닫기(t)

		//최종 리스트 출력(i, f, t 옵션 수행 후)
		cur = real_head;
		for(i = 1; cur != NULL; i++) {
			cmp = cur->list;
			sprintf(tmp_path, "%ld", cur->size);
			printf("---- Identical files #%d (%s bytes -%s)----\n", i, comma(cur->size, tmp_path), cur->path);

			j = 1;
			for(cmp = cur->list; cmp != NULL; j++) {
				lstat(cmp->path, &buf);
				ptm = localtime(&buf.st_mtime);
				printf("[%d] %s (mtime : %04d-%02d-%02d %02d:%02d:%02d) (atime : ", j, cmp->path, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				ptm = localtime(&buf.st_atime);
				printf("%04d-%02d-%02d %02d:%02d:%02d)\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				cmp = cmp->list;
			}
			cur = cur->next;
			printf("\n");
		}

		if(i < 2) {
			//메모리 해제(i, f, t 옵션 수행 후)
			while(real_head != NULL) {
				ori = real_head;
				cmp = real_head->list;
				real_head = real_head->next;
				while(cmp != NULL) {
					free(ori);
					ori = cmp;
					cmp = cmp->list;
				}
				free(ori);
			}
			exit(0);
		}
	}//while문 (옵션입력)


	//메모리 해제
	while(real_head != NULL) {
		ori = real_head;
		cmp = real_head->list;
		real_head = real_head->next;
		while(cmp != NULL) {
			free(ori);
			ori = cmp;
			cmp = cmp->list;
		}
		free(ori);
	}
	exit(0);
}



void make_real_list(struct node **real_head, struct node **fake_head) {
	struct node *tmp;
	struct node *cur, *ori, *cmp;
	FILE *fp;
	char buf[1024*16];
	char tok[5] = {0, };
	SHA_CTX ctx;
	unsigned char ori_outmd[20], cmp_outmd[20];
	int i = 0, count;

	for(cur = *fake_head; cur != NULL; cur = cur->next) {
		for(ori = cur; ori->list != NULL; ori = ori->list) {
			//이미 진짜 리스트에 넣은 파일이라면 다음 파일을 비교합니다.
			if(ori->check == 1)
				continue;

			for(cmp = ori->list; cmp != NULL; cmp = cmp->list) {
			//이미 진짜 리스트에 넣은 파일이라면 다음 파일을 비교합니다.
				if(cmp->check == 1)
					continue;

				if(ori->size == 0 && cmp->size == 0) {
					ori->check = 1;
					cmp->check = 1;
					continue;
				}
				//변수 초기화
				memset(ori_outmd, 0, sizeof(ori_outmd));
				memset(cmp_outmd, 0, sizeof(cmp_outmd));
				memset(buf, 0, sizeof(buf));

				//기준 파일 해시값을 뽑습니다.
				if((fp = fopen(ori->path, "r")) == NULL) {
					fprintf(stderr, "fopen error for %s\n", ori->path);
					ori->check = 1;
					break;
				}
				SHA1_Init(&ctx);
				while((count = fread(buf, 1, 1024*16, fp)) > 0) {
					SHA1_Update(&ctx, buf, (unsigned long)count);
					memset(buf, 0, sizeof(buf));
				}
				SHA1_Final(ori_outmd, &ctx);
				fclose(fp);

				//비교본 파일 해시값을 뽑습니다.
				if((fp = fopen(cmp->path, "r")) == NULL) {
					fprintf(stderr, "fopen error for %s\n", cmp->path);
					cmp->check = 1;
					continue;
				}
				SHA1_Init(&ctx);
				while((count = fread(buf, 1, 1024*16, fp)) > 0) {
					SHA1_Update(&ctx, buf, (unsigned long)count);
					memset(buf, 0, sizeof(buf));
				}
				SHA1_Final(cmp_outmd, &ctx);
				fclose(fp);

				//해시값을 하나하나비교합니다.
				for(count = 0; count < 20; count++) {
					if(ori_outmd[count] != cmp_outmd[count])
						break;
				}


				//해시값이 같은 경우
				if(count == 20) {
					//리스트가 비어있다면 동적할당해서 해시값을 넣어주고
					//구조체 내의 포인터 변수인 list 방향으로 동적할당을 하며
					//두 파일 모두 리스트에 넣어줍니다.
					if((*real_head) == NULL) {
						(*real_head) = (struct node *)malloc(sizeof(struct node));
						tmp = *real_head;
						tmp->size = ori->size;
						strcpy(tmp->path, "");
						for(i = 0; i < 20; i++) {
							sprintf(tok, "%02x", ori_outmd[i]);
							strcat(tmp->path, tok);
						}
						tmp->next = NULL;
						tmp->check = 0;
						tmp->last = tmp;
						tmp->last->list = NULL;

						//기준 파일을 리스트에 넣습니다.
						tmp->last->list = (struct node *)malloc(sizeof(struct node));
						tmp->last = tmp->last->list;
						tmp->last->next = NULL;
						tmp->last->size = ori->size;
						tmp->last->check = 0;
						strcpy(tmp->last->path, ori->path);
						tmp->last->list = NULL;

						//비교본 파일을 리스트에 넣습니다.
						tmp->last->list = (struct node *)malloc(sizeof(struct node));
						tmp->last = tmp->last->list;
						tmp->last->next = NULL;
						tmp->size = cmp->size;
						tmp->last->check = 0;
						strcpy(tmp->last->path, cmp->path);
						tmp->last->list = NULL;

						//리스트에 넣었다는 표시
						ori->check = 1;
						cmp->check = 1;

					}
					//리스트는 비어있지는 않지만 새로운 해시값을 반영해야할 경우
					//해시값을 먼저 넣어주고 list 방향으로 다음 두 노드에
					//기준 파일 정보와 비교본 파일 정보를 넣어줍니다.
					else if(tmp->list == NULL) {
						//맨 앞 노드에는 해시값 저장
						tmp->size = ori->size;
						strcpy(tmp->path, "");
						for(i = 0; i < 20; i++) {
							sprintf(tok, "%02x", ori_outmd[i]);
							strcat(tmp->path, tok);
						}
						tmp->next = NULL;
						tmp->check = 0;
						tmp->last = tmp;
						tmp->last->list = NULL;

						//구조체 내의 포인터 변수 list 방향으로 기준 파일의 정보 저장
						tmp->last->list = (struct node *)malloc(sizeof(struct node));
						tmp->last = tmp->last->list;
						tmp->last->next = NULL;
						tmp->last->size = ori->size;
						tmp->last->check = 0;
						strcpy(tmp->last->path, ori->path);
						tmp->last->list = NULL;

						//list 방향으로 비교본 파일의 정보 저장
						tmp->last->list = (struct node *)malloc(sizeof(struct node));
						tmp->last = tmp->last->list;
						tmp->last->next = NULL;
						tmp->size = cmp->size;
						tmp->last->check = 0;
						strcpy(tmp->last->path, cmp->path);
						tmp->last->list = NULL;

						//리스트에 넣었다는 표시
						ori->check = 1;
						cmp->check = 1;

					}
					else {
						//기준 파일은 이미 리스트에 반영되어 있을 경우 비교본 파일 정보만
						//list 방향으로 저장합니다.
						tmp->last->list = (struct node *)malloc(sizeof(struct node));
						tmp->last = tmp->last->list;
						tmp->last->next = NULL;
						tmp->last->size = cmp->size;
						tmp->last->check = 0;
						strcpy(tmp->last->path, cmp->path);
						tmp->last->list = NULL;

						cmp->check = 1;
					}
				}
			}
			//hash값이 다른 파일을 담기 위해 구조체 내의 포인터 next 방향으로 동적할당을 해둡니다.
			if(tmp != NULL && strcmp(tmp->path, "")) {
				tmp->next = (struct node *)malloc(sizeof(struct node));
				tmp = tmp->next;
				memset(tmp->path, 0, sizeof(tmp->path));
				tmp->list = NULL;
				tmp->next = NULL;
				tmp->last = NULL;
			}
		}
		//fake list에서 해당 크기의 파일을 모두 검사했다면
		//더 이상 사용하지 않으니 해당 크기의 파일은 리스트에서 제거하고 메모리를 해제합니다.
		ori = cur;
		for(cmp = cur->list; cmp != NULL; cmp = cmp->list) {
			free(ori);
			ori = cmp;
		}
		free(ori);
	}

	return;
}

void make_fake_list(struct node **head, char *path, off_t size) {
	struct node *cur;
	struct node *tmp;

	tmp = (struct node *)malloc(sizeof(struct node));
	strcpy(tmp->path, path);
	tmp->size = size;
	tmp->check = 0;
	tmp->last = tmp;
	tmp->next = NULL;
	tmp->list = NULL;

	if(*head == NULL)
		*head = tmp;

	else {
		//연결리스트의 처음부터 시작하여 탐색합니다.
		cur = *head;

		//리스트에 다음 탐색할 노드가 없거나 다음 노드의 사이즈가
		//현재 탐색하는 파일의 크기보다 크다면 멈춥니다.
		while(((cur->next) != NULL) && ((cur->next->size) <= size))
			cur = cur->next;

		//크기가 같다면 list방향으로 맨 끝에 노드를 이어주고 last를 갱신합니다.
		if(size == (cur->size)) {
			cur->last->list = tmp;
			cur->last = tmp;
		}
		else if(size < cur->size) {
			tmp->next = cur;
			*head = tmp;
		}
		//크기가 cur보다는 크고 cur->next보다는 작다면 next 방향으로 사이에 껴줍니다.
		else {
			tmp->next = cur->next;
			cur->next = tmp;
		}
	}
}

//큐를 시작합니다.
void init_queue(struct queue *Q) {
	Q->front = NULL;
	Q->rear = NULL;
	return;
}

//큐가 비었는지 확인합니다.
int is_empty(struct queue *Q) {
	return (Q->front == NULL);
}

//큐에 노드를 추가합니다. 인자로 들어온 데이터를 node 구조체의 path에 복사하여 추가합니다.
void bfs_enqueue(struct queue *Q, char *path_data) {
	struct node *tmp;
	tmp = (struct node *)malloc(sizeof(struct node));
	tmp->next = NULL;
	tmp->list = NULL;
	tmp->last = NULL;

	strcpy(tmp->path, path_data);

	if(is_empty(Q)) {
		Q->front = tmp;
		Q->rear = tmp;
	}

	else {
		Q->rear->next = tmp;
		Q->rear = tmp;
	}
	return;
}

//큐에서 노드를 제거하고 데이터를 buf에 저장합니다.
void bfs_dequeue(struct queue *Q, char *buf) {
	struct node *tmp;

	//큐가 비어있으면 함수 종료
	if(is_empty(Q)) {
		strcpy(buf, "");
		return;
	}

	strcpy(buf, Q->front->path);
	tmp = Q->front;

	//큐에 데이터가 하나면 삭제하고 큐 비우기
	if(Q->front == Q->rear) {
		free(tmp);
		tmp = NULL;
		Q->front = NULL;
		Q->rear = NULL;
	}
	else {
		Q->front = tmp->next;
		free(tmp);
		tmp = NULL;
	}
	return;
}

//세자리 마다 콤마를 찍어 문자열로 반환합니다.
char *comma(long n, char *comma_str) {
	char str[25];
	int idx, len, cidx = 0, mod;

	sprintf(str, "%ld", n);
	len = strlen(str);
	mod = len%3;

	for(idx = 0; idx < len; idx++) {
		if(idx != 0 && (idx % 3 == mod)) {
			comma_str[cidx++] = ',';
		}
		comma_str[cidx++] = str[idx];
	}
	comma_str[cidx] = 0;
	return comma_str;

}

//1차원 중복파일 리스트를 인자로 받아 f 옵션을 수행하는 함수입니다.
void option_f(struct node **cur, int set_index) {
	struct node *ori, *cmp;
	struct stat ori_buf, cmp_buf;
	struct tm *ptm;

	ori = (*cur)->list;
	cmp = ori->list;

	while(cmp != NULL) {
		lstat(ori->path, &ori_buf);
		lstat(cmp->path, &cmp_buf);

		//mtime이 작은 것 삭제 후 리스트에서 제거하고 메모리를 해제합니다.
		if(ori_buf.st_mtime < cmp_buf.st_mtime) {
			(*cur)->list = cmp;
			//파일 삭제
			if(unlink(ori->path) < 0) {
				fprintf(stderr, "unlink error for %s\n", ori->path);
				break;
			}
			//메모리 해제
			free(ori);
			ori = cmp;
			cmp = ori->list;
		}
		else {
			ori->list = cmp->list;
			//파일 삭제
			if(unlink(cmp->path) < 0) {
				fprintf(stderr, "unlink error for %s\n", cmp->path);
				break;
			}
			//메모리 해제
			free(cmp);
			cmp = ori->list;
		}
	}
	lstat(ori->path, &ori_buf);
	ptm = localtime(&ori_buf.st_mtime);
	printf("Left file in #%d : %s (%04d-%02d-%02d %02d:%02d:%02d)\n\n", set_index, ori->path, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	//메모리 해제
	(*cur)->list = NULL;
	free(ori);
	return;
}

//1차원 중복파일 리스트를 인자로 받아 t 옵션을 수행하는 함수입니다.
void option_t(struct node **cur, int set_index) {
	char trashpath[1024], trashfile[1030], int_buf[20];
	int i;
	struct node *ori, *cmp;
	struct stat ori_buf, cmp_buf;
	struct tm *ptm;

	ori = (*cur)->list;
	cmp = ori->list;

	//휴지통 디렉토리는 현재 디렉토리 밑 trash20182610 이라는 이름으로 만듭니다.
	strcpy(trashpath, getenv("PWD"));
	strcat(trashpath, "/trash20182610");

	mkdir(trashpath, 0766);		//이미 있으면 냅둡니다.

	while(cmp != NULL) {
		lstat(ori->path, &ori_buf);
		lstat(cmp->path, &cmp_buf);

		//mtime이 작은 것 삭제
		//cmp->path가 더 최근이면 ori->path 삭제하고 리스트 반영
		if(ori_buf.st_mtime < cmp_buf.st_mtime) {
			(*cur)->list = cmp;
			strcpy(trashfile, trashpath);

			//전체 경로를 제외하고 파일 이름만 추출하여 쓰레기통으로 옮길 이름을 만듭니다.
			for(i = strlen(ori->path)-1; ori->path[i] != '/'; i--);
			strcat(trashfile, &(ori->path[i]));
			//쓰레기통에 하드링크를 만들고 기존 파일을 삭제하여 쓰레기통으로 옮겨줍니다.
			//같은 이름의 파일이 있다면 이름 뒤에 숫자를 붙여서 하드링크를 만듭니다.
			for(int j = 1; link(ori->path, trashfile) < 0; j++) {
				int index = 0;

				strcpy(trashfile, trashpath);
				strcat(trashfile, &(ori->path[i]));

				//.이 있는지 찾습니다.
				for(index = strlen(trashfile)-1; trashfile[index] != '.'; index--) {
					if(index == 0)
						break;
				}

				//.이 있는 경우 . 앞에 숫자를 붙입니다.
				if(index > 0) {
					trashfile[index] = '\0';
					sprintf(int_buf, "(%d)", j);
					strcat(trashfile, int_buf);
					for(index = strlen(cmp->path)-1; cmp->path[index] != '.'; index--);
					strcat(trashfile, &(cmp->path[index]));
				}
				//.이 없는 경우 그냥 뒤에 괄호와 숫자 붙이기
				else {
					sprintf(int_buf, "(%d)", j);
					strcat(trashfile, int_buf);
				}
			}
			if(unlink(ori->path) < 0) {
				fprintf(stderr, "unlink error for %s\n", ori->path);
				break;
			}
			//삭제 후 메모리 해제
			free(ori);
			ori = cmp;
			cmp = ori->list;
		}
		//ori->path가 더 최근이면 cmp->path를 삭제하고 리스트 반영
		else {
			ori->list = cmp->list;
			strcpy(trashfile, trashpath);

			//파일 이름만 추출
			for(i = strlen(cmp->path)-1; cmp->path[i] != '/'; i--);
			strcat(trashfile, &(cmp->path[i]));
			//link와 unlink로 파일을 쓰레기통으로 이동
			for(int j = 1; link(cmp->path, trashfile) < 0; j++) {
				int index = 0;

				strcpy(trashfile, trashpath);
				strcat(trashfile, &(cmp->path[i]));

				//.이 있는지 찾습니다.
				for(index = strlen(trashfile)-1; trashfile[index] != '.'; index--) {
					if(index == 0)
						break;
				}
				//.이 있는 경우 . 앞에 숫자를 붙입니다.
				if(index > 0) {
					trashfile[index] = '\0';
					sprintf(int_buf, "(%d)", j);
					strcat(trashfile, int_buf);
					for(index = strlen(cmp->path)-1; cmp->path[index] != '.'; index--);
					strcat(trashfile, &(cmp->path[index]));
				}
				//.이 없는 경우 그냥 뒤에 괄호와 숫자 붙이기
				else {
					sprintf(int_buf, "(%d)", j);
					strcat(trashfile, int_buf);
				}
					
			}
			if(unlink(cmp->path) < 0) {
				fprintf(stderr, "unlink error for %s\n", cmp->path);
				break;
			}
			//메모리 해제
			free(cmp);
			cmp = ori->list;
		}
	}
	lstat(ori->path, &ori_buf);
	ptm = localtime(&ori_buf.st_mtime);
	printf("All files in #%d have moved to Trash excpty \"%s\" (%04d-%02d-%02d %02d:%02d:%02d)\n\n", set_index, ori->path, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	//메모리 해제
	(*cur)->list = NULL;
	free(ori);
	return;
}
