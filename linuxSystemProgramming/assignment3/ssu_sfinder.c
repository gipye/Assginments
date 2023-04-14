#define HASH_SHA1
#include "ssu_sdup.h"

#define STRMAX 10000 

extern int optind, opterr, optopt;
extern char *optarg;
extern char **environ;

int fsha1();
int list();
int trash();
int restore();
void help();

int main(void)
{
	// 변수 초기화
	dups_list_h = NULL;
	while (1) {
		// 변수 초기화
		char input[STRMAX];
		char *argv[ARGMAX];

		int argc = 0;

		optind = 1;
		opterr = 0;

		printf("20182610> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';

		argc = tokenize(input, argv);	// 토큰 분리
		argv[argc] = (char *)0;

		if (argc == 0)
			continue;

		if (!strcmp(argv[0], "exit"))
			break;

		else if (!strcmp(argv[0], "fsha1"))
		{
			char file_extension[1024], target_directory[4097], min_size[25], max_size[25], c, thread_num[2];
			char *argv2[ARGMAX];
			int argc2 = 1;

			memset(file_extension, 0, sizeof(file_extension));
			memset(target_directory, 0, sizeof(target_directory));
			min_size[0] = -1;
			max_size[0] = -1;
			strcpy(thread_num, "1");
			argv2[5] = thread_num;

			// 입력 받은 옵션과 인자를 argv2 배열에 저장합니다.
			while((c = getopt(argc, argv, "e:l:h:d:t:")) != -1)
			{
				switch(c)
				{
					case 'e':
						strcpy(file_extension, optarg);
						argv2[1] = file_extension;
						argc2++;
						break;
					case 'l':
						strcpy(min_size, optarg);
						argv2[2] = min_size;
						argc2++;
						break;
					case 'h':
						strcpy(max_size, optarg);
						argv2[3] = max_size;
						argc2++;
						break;
					case 'd':
						strcpy(target_directory, optarg);
						argv2[4] = target_directory;
						argc2++;
						break;
					case 't':
						strcpy(thread_num, optarg);
						argv2[5] = thread_num;
						argc2++;
						break;
					case '?':
						fprintf(stderr, "Error: Wrong option\n");
						break;
				}
			}

			if(file_extension[0] == 0 || target_directory[0] == 0 || min_size[0] == -1 || max_size[0] == -1)
			{
				printf("Error: option (e, l, h, d) must entered\n");
				continue;
			}
			fsha1(argc2, argv2);	// 입력받은 옵션으로 fsha1() 함수를 실행합니다.
		}
		else if (!strcmp(argv[0], "list")){
			char list_type[10], category[10], order[3], c;
			char *argv2[ARGMAX];

			optind = 1;
			strcpy(list_type, "fileset");
			strcpy(category, "size");
			strcpy(order, "1");

			// 입력 받은 옵션과 인자를 argv2 배열에 저장합니다.
			argv2[0] = argv[0];
			argv2[1] = list_type;
			argv2[2] = category;
			argv2[3] = order;
			while((c = getopt(argc, argv, "l:c:o:")) != -1)
			{
				switch(c)
				{
					case 'l':
						strcpy(list_type, optarg);
						break;
					case 'c':
						strcpy(category, optarg);
						break;
					case 'o':
						strcpy(order, optarg);
						break;
					case '?':
						fprintf(stderr, "Error: Wrong option\n");
						break;
				}
			}

			// 입력받은 옵션과 인자로 list() 함수를 실행합니다.
			list(argv2);
		}
		else if (!strcmp(argv[0], "trash")){
			char *argv2[ARGMAX], c, category[10] = {0, }, order[3] = {0, };

			optind = 1;
			strcpy(category, "filename");
			strcpy(order, "1");

			// 입력 받은 옵션과 인자를 argv2 배열에 저장합니다.
			argv2[0] = argv[0];
			argv2[1] = category;
			argv2[2] = order;
			while((c = getopt(argc, argv, "c:o:")) != -1)
			{
				switch(c)
				{
					case 'c':
						strcpy(category, optarg);
						break;
					case 'o':
						strcpy(order, optarg);
						break;
					case '?':
						fprintf(stderr, "Error: Wrong option\n");
						break;
				}
			}
			// 입력받은 옵션과 인자로 trash() 함수를 실행합니다.
			if(trash(argv2) < 0)
				continue;
		}
		else if (!strcmp(argv[0], "restore")){
			int index = 0;

			if(argv[1] == NULL) {
				fprintf(stderr, "Error: restore [RESTORE_INDEX]\n");
				continue;
			}

			// 입력 받은 인자로 restore() 함수를 실행합니다.
			restore(atoi(argv[1]));
		}

		else
			help();

	}

	printf("Prompt End\n");

	return 0;
}

int fsha1(int argc, char *argv[])
{
	char target_dir[PATHMAX];
	dirList *dirlist = (dirList *)malloc(sizeof(dirList));
	memset(dirlist, 0, sizeof(dirList));

	dups_list_h = (fileList *)malloc(sizeof(fileList));
	memset(dups_list_h, 0, sizeof(fileList));

	// 잘못된 입력이 들어올 경우 에러처리
	if (check_args(argc, argv))
		return 1;

	// ~ 이 들어간 경로를 절대 경로로 바꿔줍니다.
	if (strchr(argv[4], '~') != NULL)
		get_path_from_home(argv[4], target_dir);
	else
		realpath(argv[4], target_dir);

	// 같은 크기의 파일들을 저장하는 경로를 얻습니다.
	// 이 경로 밑에 같은 크기의 파일 이름들을 파일로 관리합니다.
	get_same_size_files_dir();

	struct timeval begin_t, end_t;

	gettimeofday(&begin_t, NULL);	// 시간 측정 시작

	dirlist_append(dirlist, target_dir);

	int tid[4];
	pthread_t thread[4];
	int thread_num = atoi(argv[5]);

	// 인자로 입력받은 쓰레드 개수만큼 쓰레드를 생성하여 탐색을 수행합니다.
	for(int i = 1; i < thread_num; i++) {
		tid[i] = pthread_create(&thread[i], NULL, thread_search, (void **)(&dirlist));
		pthread_join(thread[i], NULL);
	}

	dir_traverse(dirlist);		// 재귀 bfs로 탐색해서 같은 크기의 파일끼리 묶어서 경로를 파일로 만듭니다.
	find_duplicates();		// 같은 크기의 파일들 중 내용이 같은 파일끼리 묶어서 리스트를 만듭니다.
	remove_no_duplicates();

	gettimeofday(&end_t, NULL);		//시간 측정 종료

	end_t.tv_sec -= begin_t.tv_sec;

	if (end_t.tv_usec < begin_t.tv_usec){
		end_t.tv_sec--;
		end_t.tv_usec += 1000000;
	}

	end_t.tv_usec -= begin_t.tv_usec;

	if (filelist_print_format(dups_list_h) < 0)		// 중복 파일 리스트 출력
		printf("No duplicates in %s\n", target_dir);

	printf("Searching time: %ld:%06ld(sec:usec)\n\n", end_t.tv_sec, end_t.tv_usec);

	get_trash_path();	//쓰레기통 경로를 얻습니다.
	get_log_path();		//로그 파일 경로를 얻습니다.

	delete_prompt();	//삭제 옵션을 입력받아 수행합니다.
}

/* list 명령어 수행하는 함수 */
/* 아래 두 경우에는 따로 정렬을 수행하지 않고 출력합니다. */
/* 파일 세트를 정렬할 때 -c 옵션의 인자가 size가 아닌 경우 */
/* 파일 리스트를 정렬할 때 -c 옵션의 인자가 size인 경우 */
int list(char *argv[])
{
	int list_type, category, order;

	// -l 옵션의 인자 에러처리 */
	if(!strcmp(argv[1], "fileset"))
		list_type = FILESET;
	else if(!strcmp(argv[1], "filelist"))
		list_type = FILELIST;
	else {
		fprintf(stderr, "	list -l [LIST_TYPE] -c [CATEGORY] -o [ORDER]\n");
		fprintf(stderr, "		[LIST_TYPE]: fileset or filelist\n");
		return -1;
	}

	/* -c 옵션의 인자 에러처리 */
	if(!strcmp(argv[2], "filename"))
		category = FILENAME;
	else if(!strcmp(argv[2], "size"))
		category = SIZE;
	else if(!strcmp(argv[2], "uid"))
		category = UID;
	else if(!strcmp(argv[2], "gid"))
		category = GID;
	else if(!strcmp(argv[2], "mode"))
		category = MODE;
	else {
		fprintf(stderr, "	list -l [LIST_TYPE] -c [CATEGORY] -o [ORDER]\n");
		fprintf(stderr, "		[CATEGORY]: filename or size or uid or gid or mode\n");
		return -1;
	}

	/* -o 옵션의 인자 에러처리 */
	if(!strcmp(argv[3], "-1"))
		order = -1;
	else if(!strcmp(argv[3], "1"))
		order = 1;
	else {
		fprintf(stderr, "	list -l [LIST_TYPE] -c [CATEGORY] -o [ORDER]\n");
		fprintf(stderr, "		[ORDER]: -1 or 1\n");
		return -1;
	}

	// 리스트가 비어있을 경우 출력문
	if(dups_list_h == NULL) {
		printf("No duplicates in list\n");
		return 0;
	}

	// fileset를 정렬하여 출력할 때 size를 역순으로 출력할 경우 수행
	if(list_type == FILESET && category == SIZE && order == -1)
	{
		if(filelist_reverse_print(dups_list_h) < 0)
			printf("No duplicates in list\n");
	}
	// fileset를 정렬하여 출력할 때 size가 아닌 다른 것을 기준으로 출력할 경우 혹은 size 순으로 출력할 경우 리스트 그대로 출력합니다.
	else if(list_type == FILESET) {
		if (filelist_print_format(dups_list_h) < 0)
				printf("No duplicates in list\n");
	}
	// filelist를 정렬하여 출력할 경우 수행
	else {
		if(filelist_print_sort(dups_list_h, category, order) < 0)
			printf("No duplicates in list\n");
	}

	return 0;
}

/* trash 명령어를 수행하는 함수 */
int trash(char *argv[])
{
	int category, order;

	trash_list_h = (trashList *)malloc(sizeof(trashList));
	memset(trash_list_h, 0, sizeof(trashList));

	// -c 옵션의 인자 에러처리
	if(!strcmp(argv[1], "filename"))
		category = FILENAME;
	else if(!strcmp(argv[1], "size"))
		category = SIZE;
	else if(!strcmp(argv[1], "date"))
		category = DATE;
	else if(!strcmp(argv[1], "time"))
		category = TIME;
	else {
		fprintf(stderr, "	trash -c [CATEGORY] -o [ORDER]\n");
		fprintf(stderr, "		[CTEGORY]: filename or size or date or time\n");
		return -1;
	}

	// -o 옵션의 인자 에러처리
	if(!strcmp(argv[2], "-1"))
		order = -1;
	else if(!strcmp(argv[2], "1"))
		order = 1;
	else {
		fprintf(stderr, "	trash -c [CATEGORY] -o [ORDER]\n");
		fprintf(stderr, "		[ORDER]: -1 or 1\n");
		return -1;
	}

	// 옵션을에 따라 쓰레기통 리스트를 만들어 정렬합니다.
	if(trash_list(category, order) < 0){
		printf("\nNo file in trash\n\n");
		return 0;
	}
	else {
		printf("\n      ");
		printf("%-60s", "FILENAME");
		printf("%-15s", "SIZE");
		printf("%-15s", "DELETION DATE");
		printf("%-15s\n", "DELETION TIME");
		trashlist_print(trash_list_h);	// 쓰레기통 리스트를 모두 출력합니다.
		printf("\n");
	}

	return 0;
}

/* restore 명령어를 수행하는 함수 */
int restore(int index)
{
	trashList *deleted;
	char trash_to_move[PATHMAX], info_path[PATHMAX], buf[PATHMAX] = {0, };
	char hash[41];
	FILE *fp;
	fileList *search;

	// trash 명령어를 입력하지 않았을 경우 쓰레기통 리스트가 만들어지지 않았으므로 에러처리
	if(trash_list_h == NULL) {
		fprintf(stderr, "Input trash instruction first\n");
		return -1;
	}
	else if(index > trashlist_size(trash_list_h) || index < 1) {	// 쓰레기통 리스트의 인덱스를 벗어난 입력일 경우 에러처리
		fprintf(stderr, "Error: [RESTORE_INDEX] out of range\n");
		return -1;
	}

	deleted = trashlist_delete_node(trash_list_h, index);		// 복원할 인덱스에 해당하는 노드를 deleted로 가리킵니다.

	int i = 1;
	// 같은 이름의 파일이 여러 개일 경우 쓰레기통의 정보 파일을 읽어서 복원할 파일을 찾습니다.
	while(strcmp(buf, deleted->path)){
		char hash_tmp[41] = {0, };
		char *l, *k;

		// 변수 초기화
		memset(trash_to_move, 0, sizeof(trash_to_move));
		memset(info_path, 0, sizeof(info_path));
		memset(buf, 0, sizeof(buf));

		sprintf(trash_to_move, "%s%s", trash_path, strrchr(deleted->path, '/') + 1);
		sprintf(info_path, "%s%s", trash_info_path, strrchr(deleted->path, '/') + 1);
		if(i > 1) {		//같은 이름의 파일이 있는 경우 if문 수행
			char tmp[20];
			int j = 0;
			// 인덱스 j로 파일 이름의 확장자부분을 가리킵니다.
			for(j = strlen(trash_to_move); trash_to_move[j] != '.' && trash_to_move[j] != '/'; j--);
			if(trash_to_move[j] == '.') {	//확장자가 있을 경우
				l = get_extension(trash_to_move);
				k = get_extension(info_path);

				strcpy(tmp, l);

				//쓰레기통 경로 뒤에 파일 이름 뒤에 숫자를 붙여서 경로 이름을 만듭니다.
				sprintf(l, "%d", i);
				sprintf(k, "%d", i);

				strcat(trash_to_move, ".");
				strcat(trash_to_move, tmp);

				strcat(info_path, ".");
				strcat(info_path, tmp);
			}
			else {	//확장자가 없을 경우
				sprintf(tmp, "%d", i);

				//쓰레기통 경로 뒤에 파일 이름 뒤에 숫자를 붙여서 경로 이름을 만듭니다.
				strcat(trash_to_move, ".");
				strcat(info_path, ".");

				strcat(trash_to_move, tmp);
				strcat(info_path, tmp);

				printf("trash_to_move: %s\n", trash_to_move);
				printf("info_path: %s\n", info_path);
			}
		}

		strcat(info_path, ".trashinfo");	//쓰레기통 정보 파일의 이름을 만듭니다.

		i++;
		if((fp = fopen(info_path, "r")) == NULL)
			continue;

		fseek(fp, (off_t)18, SEEK_SET);
		fgets(buf, PATHMAX, fp);

		fgets(hash_tmp, PATHMAX, fp);
		fread(hash_tmp, sizeof(char), 40, fp);
		hash_tmp[40] = '\0';
		strcpy(hash, hash_tmp);		//정보 파일에 저장된 해시값을 hash에 저장합니다.

		buf[strlen(buf)-1] = '\0';		// 정보 파일에 저장된 경로를 buf에 저장합니다.

		fclose(fp);
	}

	if(unlink(info_path) < 0) {		// 정보 파일 삭제
		fprintf(stderr, "Error: Fail to remove info file\n");
		return -1;
	}

	if (rename(trash_to_move, deleted->path) == -1){	// 파일 복원
		printf("ERROR: Fail to restore in Trash\n");
		return -1;
	}
	write_log(RESTORE, deleted->path);	//로그 작성
	
	search = fileList_search(dups_list_h, hash);	// 중복 파일 리스트에서 해시에 해당하는 노드를 찾습니다.
	fileinfo_append(search->fileInfoList, deleted->path);	// 노드의 파일 리스트뒤에 추가합니다.
	fileinfolist_sort(search->fileInfoList);	//리스트를 파일 경로 이름을 기준으로 정렬합니다.

	free(deleted);

	// 복원이 완료된 후 쓰레기통 리스트를 출력합니다.
	if(trashlist_size(trash_list_h) < 1)
		printf("\nNo file in trash\n\n");
	else {
		printf("\n      ");
		printf("%-60s", "FILENAME");
		printf("%-15s", "SIZE");
		printf("%-15s", "DELETION DATE");
		printf("%-15s\n", "DELETION TIME");
		trashlist_print(trash_list_h);
		printf("\n");
	}
		
	return 0;
}

/* 잘못된 명령어나 help 명령어를 수행하는 함수 */
void help(void) {
	printf("Usage:\n > fsha1 –e [FILE_EXTENSION] -l [MINSIZE] -h [MAXSIZE] -d [TARGET_DIRECTORY] -t [THREAD_NUM]\n\t>> delete – l [SET_INDEX] -d [OPTARG] -i -f -t\n > trash -c [CATEGORY] -o [ORDER]\n > restore [RESTORE_INDEX]\n > help\n > exit\n\n");
}
