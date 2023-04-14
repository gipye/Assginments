#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <openssl/sha.h>

#define NAMEMAX 255
#define PATHMAX 4096

#ifdef HASH_SHA1
	#define HASHMAX 41
#else
	#define HASHMAX 33
#endif

#define STRMAX 10000 
#define ARGMAX 20

typedef struct fileInfo {
	char path[PATHMAX];
	struct stat statbuf;
	struct fileInfo *next;
} fileInfo;

typedef struct fileList {
	long long filesize;
	char hash[HASHMAX];
	fileInfo *fileInfoList;
	struct fileList *next;
} fileList;

typedef struct dirList {
	char dirpath[PATHMAX];
	struct dirList *next;
} dirList;

typedef struct trashList {
	char path[PATHMAX];
	int size;
	char date[11];
	char time[9];
	struct trashList *next;
} trashList;

#define DIRECTORY 1
#define REGFILE 2

#define REMOVE 5
#define DELETE 6
#define RESTORE 7

#define FILENAME 9
#define SIZE 10
#define DATE 11
#define TIME 12
#define UID 13
#define GID 14
#define MODE 15

#define FILESET 20
#define FILELIST 21

#define KB 1000
#define MB 1000000
#define GB 1000000000
#define KiB 1024
#define MiB 1048576
#define GiB 1073741824
#define SIZE_ERROR -2

char extension[10];
char same_size_files_dir[PATHMAX];
char trash_path[PATHMAX];
char trash_info_path[PATHMAX];
char log_path[PATHMAX];
long long minbsize;
long long maxbsize;
fileList *dups_list_h;
trashList *trash_list_h;

/* 파일 리스트에 노드 추가하는 함수 */
void fileinfo_append(fileInfo *head, char *path)
{
	fileInfo *fileinfo_cur;

	fileInfo *newinfo = (fileInfo *)malloc(sizeof(fileInfo));
	memset(newinfo, 0, sizeof(fileInfo));
	strcpy(newinfo->path, path);
	lstat(newinfo->path, &newinfo->statbuf);
	newinfo->next = NULL;

	if (head->next == NULL)
		head->next = newinfo;
	else {
		fileinfo_cur = head->next;
		while (fileinfo_cur->next != NULL)
			fileinfo_cur = fileinfo_cur->next;

		fileinfo_cur->next = newinfo;
	}

}

/* 파일 리스트에서 path에 해당하는 노드 삭제하는 함수 */
fileInfo *fileinfo_delete_node(fileInfo *head, char *path)
{
	fileInfo *deleted;

	if (!strcmp(head->next->path, path)){
		deleted = head->next;
		head->next = head->next->next;
		free(deleted);
		return head->next;
	}
	else {
		fileInfo *fileinfo_cur = head->next;

		while (fileinfo_cur->next != NULL){
			if (!strcmp(fileinfo_cur->next->path, path)){
				deleted = fileinfo_cur->next;

				fileinfo_cur->next = fileinfo_cur->next->next;
				free(deleted);
				return fileinfo_cur->next;
			}

			fileinfo_cur = fileinfo_cur->next;
		}
	}
}

/* 파일 리스트의 파일 개수 얻는 함수 */
int fileinfolist_size(fileInfo *head)
{
	fileInfo *cur = head->next;
	int size = 0;

	while (cur != NULL){
		size++;
		cur = cur->next;
	}

	return size;
}

/* 연결 리스트에서 두 노드의 위치를 바꾸는 함수 */
void exchange_fileinfonode(fileInfo **cur, fileInfo **cmp, fileInfo **cur_prev, fileInfo **cmp_prev)
{
	// 새 두 노드를 동적할당 합니다.
	fileInfo *tmp_cur = (fileInfo *)malloc(sizeof(fileInfo));
	fileInfo *tmp_cmp = (fileInfo *)malloc(sizeof(fileInfo));

	// 현재 노드와, (바꿀 노드의 이전 노드)가 같은 경우
	if(*cmp_prev == *cur) {
		// 새 두 노드에 바꿀 두 노드의 내용을 복사합니다.
		memcpy(tmp_cur, *cur, sizeof(fileInfo));
		memcpy(tmp_cmp, *cmp, sizeof(fileInfo));

		// 새 두 노드의 다음 노드를 각각 바꿀 두 노드가 가리키던 노드를 가리키도록 만듭니다.
		tmp_cur->next = (*cmp)->next;
		tmp_cmp->next = (*cur)->next;

		// 바꿀 두 노드를 가리키던 노드가 새 두 노드를 가리키도록 합니다.
		(*cur_prev)->next = tmp_cmp;
		tmp_cmp->next = tmp_cur;
	}
	// 현재 노드와, (바꿀 노드의 이전 노드)가 다른 경우
	else {
		// 새 두 노드에 바꿀 두 노드의 내용을 복사합니다.
		memcpy(tmp_cur, *cur, sizeof(fileInfo));
		memcpy(tmp_cmp, *cmp, sizeof(fileInfo));

		// 새 두 노드의 다음 노드를 각각 바꿀 두 노드가 가리키던 노드를 가리키도록 만듭니다.
		tmp_cur->next = (*cmp)->next;
		tmp_cmp->next = (*cur)->next;

		// 바꿀 두 노드를 가리키던 노드가 새 두 노드를 가리키도록 합니다.
		(*cur_prev)->next = tmp_cmp;
		(*cmp_prev)->next = tmp_cur;
	}

	// 교체되어 쓸모 없어진 노드는 메모리 해제합니다.
	free(*cur);
	free(*cmp);

	// 인자로 들어온 포인터가 새 두 노드를 가리키도록 합니다.
	*cur = tmp_cmp;
	*cmp = tmp_cur;
}

/* 경로 비교하는 함수 */
/* 깊이가 더 깊은지, 같은 깊이라면 사전 순으로 비교합니다. */
int pathcmp(char *ori, char *cmp)
{
	int ori_count = 0, cmp_count = 0;

	for(int i = strlen(ori); i > 0; i--){	//ori 경로의 깊이를 얻습니다.
		if(ori[i] == '/')
			ori_count++;
	}

	for(int i = strlen(cmp); i > 0; i--){	//cmp 경러의 깊이를 얻습니다.
		if(cmp[i] == '/')
			cmp_count++;
	}

	if(ori_count > cmp_count)
		return 1;
	else if(ori_count < cmp_count)
		return -1;
	else
		return strcmp(ori, cmp);	//깊이가 같다면 문자열로 비교합니다.
}

/* 파일 리스트의 연결리스트를 파일 경로 이름 순으로 정렬하는 함수 */
/* 경로가 얕은 것이 앞에 오도록, 같은 깊이라면 오름차순으로 정렬 */
void fileinfolist_sort(fileInfo *head)
{
	fileInfo *fileinfo_cur = head->next;
	fileInfo *fileinfo_cmp = fileinfo_cur->next;
	fileInfo *cur_prev = head;;
	fileInfo *cmp_prev = head->next;

	while (fileinfo_cur != NULL) {
		cmp_prev = fileinfo_cur;
		fileinfo_cmp = fileinfo_cur->next;

		while(fileinfo_cmp != NULL) {
			// 경로를 비교하여 앞에 경로가 더 크다면 자리를 바꿉니다.
			if(pathcmp(fileinfo_cur->path, fileinfo_cmp->path) > 0)
				exchange_fileinfonode(&fileinfo_cur, &fileinfo_cmp, &cur_prev, &cmp_prev);

			cmp_prev = fileinfo_cmp;
			fileinfo_cmp = fileinfo_cmp->next;
		}
		cur_prev = fileinfo_cur;
		fileinfo_cur = fileinfo_cur->next;
	}

}

// 파일 중복 세트 리스트에 세트를 추가하는 함수
void filelist_append(fileList *head, long long filesize, char *path, char *hash)
{
	fileList *newfile = (fileList *)malloc(sizeof(fileList));
	memset(newfile, 0, sizeof(fileList));

	newfile->filesize = filesize;
	strcpy(newfile->hash, hash);

	newfile->fileInfoList = (fileInfo *)malloc(sizeof(fileInfo));
	memset(newfile->fileInfoList, 0, sizeof(fileInfo));

	fileinfo_append(newfile->fileInfoList, path);
	newfile->next = NULL;

	if (head->next == NULL) {
		head->next = newfile;
	}    
	else {
		fileList *cur_node = head->next, *prev_node = head, *next_node;

		while (cur_node != NULL && cur_node->filesize < newfile->filesize) {
			prev_node = cur_node;
			cur_node = cur_node->next;
		}

		newfile->next = cur_node;
		prev_node->next = newfile;
	}    
}

// 파일 중복 세트 리스트에 hash에 해당하는 노드를 삭제하는 함수
void filelist_delete_node(fileList *head, char *hash)
{
	fileList *deleted;

	if (!strcmp(head->next->hash, hash)){
		deleted = head->next;
		head->next = head->next->next;
	}
	else {
		fileList *filelist_cur = head->next;

		while (filelist_cur->next != NULL){
			if (!strcmp(filelist_cur->next->hash, hash)){
				deleted = filelist_cur->next;

				filelist_cur->next = filelist_cur->next->next;

				break;
			}

			filelist_cur = filelist_cur->next;
		}
	}

	free(deleted);
}

// 파일 중복 세트 개수를 얻는 함수
int filelist_size(fileList *head)
{
	fileList *cur = head->next;
	int size = 0;

	while (cur != NULL){
		if(fileinfolist_size(cur->fileInfoList) < 2)
		{	cur = cur->next;	continue;	}
		size++;
		cur = cur->next;
	}

	return size;
}

// hash 와 같은 해시값을 갖는 파일 중복 세트의 인덱스를 얻는 함수
int filelist_search(fileList *head, char *hash)
{
	fileList *cur = head;
	int idx = 0;

	while (cur != NULL){
		if (!strcmp(cur->hash, hash))
			return idx;
		cur = cur->next;
		idx++;
	}

	return 0;
}

// hash 와 같은 해시값을 갖는 파일 중복 세트를 얻는 함수
fileList *fileList_search(fileList *head, char *hash)
{
	fileList *cur = head;
	int idx = 0;

	while (cur != NULL){
		if (!strcmp(cur->hash, hash))
			return cur;
		cur = cur->next;
		idx++;
	}

	return NULL;
}

// 디렉토리 연결 리스트에 노드 추가하는 함수
void dirlist_append(dirList *head, char *path)
{
	dirList *newFile = (dirList *)malloc(sizeof(dirList));

	strcpy(newFile->dirpath, path);
	newFile->next = NULL;

	if (head->next == NULL)
		head->next = newFile;
	else{
		dirList *cur = head->next;

		while(cur->next != NULL)
			cur = cur->next;

		cur->next = newFile;
	}
}

// 디렉토리 연결 리스트를 출력하는 함수
void dirlist_print(dirList *head, int index)
{
	dirList *cur = head->next;
	int i = 1;

	while (cur != NULL){
		if (index) 
			printf("[%d] ", i++);
		printf("%s\n", cur->dirpath);
		cur = cur->next;
	}
}

// 디렉토리 연결 리스트를 삭제하는 함수
void dirlist_delete_all(dirList *head)
{
	dirList *dirlist_cur = head->next;
	dirList *tmp;

	while (dirlist_cur != NULL){
		tmp = dirlist_cur->next;
		free(dirlist_cur);
		dirlist_cur = tmp;
	}

	head->next = NULL;
}

// 쓰레기통 연결 리스트에 노드 추가하는 함수
void trashlist_append(trashList *head, char *path, int size, char *date, char *time)
{
	trashList *trashlist_cur;

	// 인자로 들어온 값들을 새 노드에 저장합니다.
	trashList *newinfo = (trashList *)malloc(sizeof(trashList));
	memset(newinfo, 0, sizeof(trashList));
	strcpy(newinfo->path, path);
	newinfo->size = size;
	strcpy(newinfo->date, date);
	strcpy(newinfo->time, time);
	newinfo->next = NULL;

	// 노드의 마지막에 이어 붙입니다.
	if (head->next == NULL)
		head->next = newinfo;
	else {
		trashlist_cur = head->next;
		while (trashlist_cur->next != NULL)
			trashlist_cur = trashlist_cur->next;

		trashlist_cur->next = newinfo;
	}

}

// 쓰레기통 리스트를 출력하는 함수
void trashlist_print(trashList *head)
{
	trashList *cur = head->next;
	int i = 1;

	while (cur != NULL){
		printf("[%3d] ", i++);
		printf("%-60s", cur->path);
		printf("%-15d", cur->size);
		printf("%-15s", cur->date);
		printf("%-15s\n", cur->time);

		cur = cur->next;
	}
}

// 쓰레기통 연결 리스트에서 인덱스에 해당하는 노드를 연결리스트에서 제거하고 그 노드를 반환하는 함수
trashList *trashlist_delete_node(trashList *head, int idx)
{
	trashList *trashlist_cur = head;
	trashList *tmp;
	int i = 1;

	// 인덱스에 해당하는 노드 이전 노드를 가리키도록 합니다.
	while (i < idx) {
		trashlist_cur = trashlist_cur->next;
		i++;
	}
	//해당 노드를 연결 리스트에서 제거합니다.
	tmp = trashlist_cur;
	trashlist_cur = trashlist_cur->next;

	tmp->next = trashlist_cur->next;
	trashlist_cur->next = NULL;

	return trashlist_cur;	// 그 노드를 반환합니다.
}

// 쓰레기통 연결 리스트의 노드 개수를 얻는 함수
int trashlist_size(trashList *head)
{
	trashList *cur = head->next;
	int size = 0;

	while (cur != NULL){
		size++;
		cur = cur->next;
	}

	return size;
}

int tokenize(char *input, char *argv[])
{
	char *ptr = NULL;
	int argc = 0;
	ptr = strtok(input, " ");

	while (ptr != NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}
	return argc;
}

// 쓰레기통 연결 리스트에서 두 노드의 위치를 바꾸는 함수
void exchange_trashnode(trashList **cur, trashList **cmp, trashList **cur_prev, trashList **cmp_prev)
{
	trashList *tmp_cur = (trashList *)malloc(sizeof(trashList));
	trashList *tmp_cmp = (trashList *)malloc(sizeof(trashList));

	if(*cmp_prev == *cur) {
		memcpy(tmp_cur, *cur, sizeof(trashList));
		memcpy(tmp_cmp, *cmp, sizeof(trashList));

		tmp_cur->next = (*cmp)->next;
		tmp_cmp->next = (*cur)->next;

		(*cur_prev)->next = tmp_cmp;
		tmp_cmp->next = tmp_cur;
	}
	else {
		memcpy(tmp_cur, *cur, sizeof(trashList));
		memcpy(tmp_cmp, *cmp, sizeof(trashList));

		tmp_cur->next = (*cmp)->next;
		tmp_cmp->next = (*cur)->next;

		(*cur_prev)->next = tmp_cmp;
		(*cmp_prev)->next = tmp_cur;
	}

	free(*cur);
	free(*cmp);

	*cur = tmp_cmp;
	*cmp = tmp_cur;
}

// 쓰레기통 연결 리스트를 인자로 들어온 옵션에 따라 정렬하는 함수
void trashlist_sort(trashList *head, int category, int order)
{
	trashList *trashlist_cur = head->next;
	trashList *trashlist_cmp = trashlist_cur->next;
	trashList *cur_prev = head;
	trashList *cmp_prev = head->next;

	// 오름차순 정렬인 경우
	if(order == 1)
	{
		while (trashlist_cur != NULL) {
			cmp_prev = trashlist_cur;
			trashlist_cmp = trashlist_cur->next;

			while (trashlist_cmp != NULL) {
				if (category == SIZE && (trashlist_cur->size > trashlist_cmp->size))	// 옵션이 size인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);
				else if (category == DATE && strcmp(trashlist_cur->date, trashlist_cmp->date) > 0)	//옵션이 date인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);
				else if (category == TIME && strcmp(trashlist_cur->time, trashlist_cmp->time) > 0)	//옵션이 time인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);
				else if (category == FILENAME && pathcmp(trashlist_cur->path, trashlist_cmp->path) > 0)	//옵션이 filename인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);

				cmp_prev = trashlist_cmp;
				trashlist_cmp = trashlist_cmp->next;
			}
			cur_prev = trashlist_cur;
			trashlist_cur = trashlist_cur->next;
		}
	}
	// 내림차순 정렬인 경우
	else {
		while (trashlist_cur != NULL) {
			cmp_prev = trashlist_cur;
			trashlist_cmp = trashlist_cur->next;

			while (trashlist_cmp != NULL) {
				if (category == SIZE && (trashlist_cur->size < trashlist_cmp->size))	// 옵션이 size인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);
				else if (category == DATE && strcmp(trashlist_cur->date, trashlist_cmp->date) < 0)	//옵션이 date인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);
				else if (category == TIME && strcmp(trashlist_cur->time, trashlist_cmp->time) < 0)	//옵션이 time인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);
				else if (category == FILENAME && pathcmp(trashlist_cur->path, trashlist_cmp->path) < 0)	//옵션이 filename인 경우
					exchange_trashnode(&trashlist_cur, &trashlist_cmp, &cur_prev, &cmp_prev);

				cmp_prev = trashlist_cmp;
				trashlist_cmp = trashlist_cmp->next;
			}
			cur_prev = trashlist_cur;
			trashlist_cur = trashlist_cur->next;
		}
	}

}

// 홈 디렉토리 경로 얻는 함수
void get_path_from_home(char *path, char *path_from_home)
{
	char path_without_home[PATHMAX] = {0,};
	char *home_path;

	home_path = getenv("HOME");

	if (strlen(path) == 1){
		strncpy(path_from_home, home_path, strlen(home_path));
	}
	else {
		strncpy(path_without_home, path + 1, strlen(path)-1);
		sprintf(path_from_home, "%s%s", home_path, path_without_home);
	}
}

int is_dir(char *target_dir)
{
	struct stat statbuf;

	if (lstat(target_dir, &statbuf) < 0){
		printf("ERROR: lstat error for %s\n", target_dir);
		return 1;
	}
	return S_ISDIR(statbuf.st_mode) ? DIRECTORY : 0;

}

// 사이즈 단위를 변환하는 함수
long long get_size(char *filesize)
{
	double size_bytes = 0;
	char size[STRMAX] = {0, };
	char size_unit[4] = {0, };
	int size_idx = 0;

	if (!strcmp(filesize, "~"))
		size_bytes = -1;
	else {
		for (int i = 0; i < strlen(filesize); i++){
			if (isdigit(filesize[i]) || filesize[i] == '.'){
				size[size_idx++] = filesize[i];
				if (filesize[i] == '.' && !isdigit(filesize[i + 1]))
					return SIZE_ERROR;
			}
			else {
				strcpy(size_unit, filesize + i);
				break;
			}
		}

		size_bytes = atof(size);

		if (strlen(size_unit) != 0){
			if (!strcmp(size_unit, "kb") || !strcmp(size_unit, "KB"))
				size_bytes *= KB;
			else if (!strcmp(size_unit, "mb") || !strcmp(size_unit, "MB"))
				size_bytes *= MB;
			else if (!strcmp(size_unit, "gb") || !strcmp(size_unit, "GB"))
				size_bytes *= GB;
			else if (!strcmp(size_unit, "kib") || !strcmp(size_unit, "KiB"))
				size_bytes *= KiB;
			else if (!strcmp(size_unit, "mib") || !strcmp(size_unit, "MiB"))
				size_bytes *= MiB;
			else if (!strcmp(size_unit, "gib") || !strcmp(size_unit, "GiB"))
				size_bytes *= GiB;
			else
				return SIZE_ERROR;
		}
	}

	return (long long)size_bytes;
}

// 파일 종류 반환하는 함수
int get_file_mode(char *target_file, struct stat *statbuf)
{
	if (lstat(target_file, statbuf) < 0){
		printf("ERROR: lstat error for %s\n", target_file);
		return 0;
	}

	if (S_ISREG(statbuf->st_mode))
		return REGFILE;
	else if(S_ISDIR(statbuf->st_mode))
		return DIRECTORY;
	else
		return 0;
}

// 파일 절대 경로를 저장하는 함수
void get_fullpath(char *target_dir, char *target_file, char *fullpath)
{
	strcat(fullpath, target_dir);

	if(fullpath[strlen(target_dir) - 1] != '/')
		strcat(fullpath, "/");

	strcat(fullpath, target_file);
	fullpath[strlen(fullpath)] = '\0';
}

// 디렉토리에 하위 파일들을 namelist가 가리키는 구조체에 저장하는 함수
int get_dirlist(char *target_dir, struct dirent ***namelist)
{
	int cnt = 0;

	if ((cnt = scandir(target_dir, namelist, NULL, alphasort)) == -1){
		printf("ERROR: scandir error for %s\n", target_dir);
		return -1;
	}

	return cnt;
}

// 파일 이름에서 확장자를 얻는 함수 확장자가 없으면 NULL을 리턴
char *get_extension(char *filename)
{
	char *tmp_ext;

	if ((tmp_ext = strstr(filename, ".tar")) != NULL || (tmp_ext = strrchr(filename, '.')) != NULL)
		return tmp_ext + 1;
	else
		return NULL;
}

// 파일 이름 부분만 얻는 함수
void get_filename(char *path, char *filename)
{
	char tmp_name[NAMEMAX];
	char *pt = NULL;

	memset(tmp_name, 0, sizeof(tmp_name));

	if (strrchr(path, '/') != NULL)
		strcpy(tmp_name, strrchr(path, '/') + 1);
	else
		strcpy(tmp_name, path);

	if ((pt = get_extension(tmp_name)) != NULL)
		pt[-1] = '\0';

	if (strchr(path, '/') == NULL && (pt = strrchr(tmp_name, '.')) != NULL)
		pt[0] = '\0';

	strcpy(filename, tmp_name);
}

void get_new_file_name(char *org_filename, char *new_filename)
{
	char new_trash_path[PATHMAX];
	char tmp[NAMEMAX];
	struct dirent **namelist;
	int trashlist_cnt;
	int same_name_cnt = 1;

	get_filename(org_filename, new_filename);
	trashlist_cnt = get_dirlist(trash_path, &namelist);

	for (int i = 0; i < trashlist_cnt; i++){
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		memset(tmp, 0, sizeof(tmp));
		get_filename(namelist[i]->d_name, tmp);

		if (!strcmp(new_filename, tmp))
			same_name_cnt++;
	}

	sprintf(new_filename + strlen(new_filename), ".%d", same_name_cnt);

	if (get_extension(org_filename) != NULL)
		sprintf(new_filename + strlen(new_filename), ".%s", get_extension(org_filename));
}

void remove_files(char *dir)
{
	struct dirent **namelist;
	int listcnt = get_dirlist(dir, &namelist);

	for (int i = 0; i < listcnt; i++){
		char fullpath[PATHMAX] = {0, };

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		get_fullpath(dir, namelist[i]->d_name, fullpath);

		remove(fullpath);
	}
}

void get_same_size_files_dir(void)
{
	get_path_from_home("~/20182610", same_size_files_dir);

	if (access(same_size_files_dir, F_OK) == 0)
		remove_files(same_size_files_dir);
	else
		mkdir(same_size_files_dir, 0755);
}

// 휴지통 경로를 얻습니다.
void get_trash_path(void)
{
	char tmp[100];
	get_path_from_home("~/.Trash/", tmp);
	get_path_from_home("~/.Trash/files/", trash_path);

	if (access(trash_path, F_OK) == 0)
		remove_files(trash_path);
	else {
		mkdir(tmp, 0755);
		mkdir(trash_path, 0755);
	}

	get_path_from_home("~/.Trash/info/", trash_info_path);

	if (access(trash_info_path, F_OK) == 0)
		remove_files(trash_info_path);
	else
		mkdir(trash_info_path, 0755);
}

// 로그 파일을 만듭니다.
void get_log_path(void)
{
	int fd;
	get_path_from_home("~/.duplicate_20182610.log", log_path);

	if((fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		fprintf(stderr, "open error for %s\n", log_path);
		return;
	}
	close(fd);
}

//로그 파일을 작성합니다.
/* 모드와 파일 경로를 인자로 받아 어떤 작업을 수행했는지 작성합니다. */
int write_log(int mode, char *path)
{
	int fd;
	time_t t = time(NULL);
	struct tm *time;
	char ymdt[25] = {0, };
	char user_name[100], tmp[100];

	strcpy(tmp, getenv("HOME"));
	strcpy(user_name, strrchr(tmp, '/') + 1);

	if((fd = open(log_path, O_WRONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", log_path);
		return -1;
	}

	lseek(fd, 0, SEEK_END);

	// 어떤 작업을 수행했는지에 따라 다르게 저장합니다.
	if(mode == REMOVE)
		write(fd, "REMOVE ", strlen("REMOVE "));
	else if(mode == DELETE)
		write(fd, "DELETE ", strlen("DELETE "));
	else if(mode == RESTORE)
		write(fd, "RESTORE ", strlen("RESTORE "));

	// 작업을 수행한 파일의 경로를 저장합니다.
	write(fd, path, strlen(path));
	write(fd, " ", sizeof(char));

	// 파일 작업을 수행한 시간을 저장합니다.
	time = localtime(&t);
	sprintf(ymdt, "%04d-%02d-%02d %02d:%02d:%02d ", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

	write(fd, ymdt, strlen(ymdt));
	write(fd, user_name, strlen(user_name));
	write(fd, "\n", sizeof(char));

	close(fd);
	return 0;
}

// fsha1() 함수의 인자가 정상적으로 입력됐는지 검사하는 함수
int check_args(int argc, char *argv[])
{
	char target_dir[PATHMAX];
	int thread_num = atoi(argv[5]);

	if (strchr(argv[1], '*') == NULL){
		printf("ERROR: [FILE_EXTENSION] should be '*' or '*.extension'\n");
		return 1;
	}

	if (strchr(argv[1], '.') != NULL){
		strcpy(extension, get_extension(argv[1]));

		if (strlen(extension) == 0){
			printf("ERROR: There should be extension\n");
			return 1;
		}
	}

	minbsize = get_size(argv[2]);

	if (minbsize == -1)
		minbsize = 0;

	maxbsize = get_size(argv[3]);

	if (minbsize == SIZE_ERROR || maxbsize == SIZE_ERROR){
		printf("ERROR: Size wrong -min size : %s -max size : %s\n", argv[2], argv[3]);
		return 1;
	}

	if (maxbsize != -1 && minbsize > maxbsize){
		printf("ERROR: [MAXSIZE] should be bigger than [MINSIZE]\n");
		return 1;
	}

	if (strchr(argv[4], '~') != NULL)
		get_path_from_home(argv[4], target_dir);
	else{
		if (realpath(argv[4], target_dir) == NULL){
			printf("ERROR: [TARGET_DIRECTORY] should exist\n");
			return 1;
		}
	}

	if (access(target_dir, F_OK) == -1){
		printf("ERROR: %s directory doesn't exist\n", target_dir);
		return 1;
	}

	if (!is_dir(target_dir)){
		printf("ERROR: [TARGET_DIRECTORY] should be a directory\n");
		return 1;
	}

	if(thread_num < 1 || thread_num > 5){
		printf("ERROR: [THREAD_NUM] 1 to 5\n");
		return 1;
	}

	return 0;
}

//파일 크기를 세 자리 수마다 콤마를 찍어서 문자열로 저장합니다.
void filesize_with_comma(long long filesize, char *filesize_w_comma)
{
	char filesize_wo_comma[STRMAX] = {0, };
	int comma;
	int idx = 0;

	sprintf(filesize_wo_comma, "%lld", filesize);
	comma = strlen(filesize_wo_comma)%3;

	for (int i = 0 ; i < strlen(filesize_wo_comma); i++){
		if (i > 0 && (i%3) == comma)
			filesize_w_comma[idx++] = ',';

		filesize_w_comma[idx++] = filesize_wo_comma[i];
	}

	filesize_w_comma[idx] = '\0';
}

// 날짜 시간 정보를 문자열로 저장합니다.
void sec_to_ymdt(struct tm *time, char *ymdt)
{
	sprintf(ymdt, "%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
}

// 중복 파일 리스트 전체를 순서대로 출력합니다.
int filelist_print_format(fileList *head)
{
	fileList *filelist_cur = head->next;
	int set_idx = 1;	

	while (filelist_cur != NULL){
		fileInfo *fileinfolist_cur = filelist_cur->fileInfoList->next;
		char mtime[STRMAX];
		char atime[STRMAX];
		char filesize_w_comma[STRMAX] = {0, };
		int i = 1;

		if (fileinfolist_size(filelist_cur->fileInfoList) < 2)
		{	filelist_cur = filelist_cur->next;	continue;	}
		filesize_with_comma(filelist_cur->filesize, filesize_w_comma);

		printf("---- Identical files #%d (%s bytes - %s) ----\n", set_idx++, filesize_w_comma, filelist_cur->hash);

		while (fileinfolist_cur != NULL){
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_mtime), mtime);
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_atime), atime);
			printf("[%d] %s (mtime : %s) (atime : %s)", i++, fileinfolist_cur->path, mtime, atime);
			printf(" (uid : %d) (gid : %d) (mode : %o)\n", fileinfolist_cur->statbuf.st_uid, fileinfolist_cur->statbuf.st_gid, fileinfolist_cur->statbuf.st_mode);

			fileinfolist_cur = fileinfolist_cur->next;
		}

		printf("\n");

		filelist_cur = filelist_cur->next;
	}
	if (set_idx == 1) {
		return -1;
	}
	else
		return 0;
}

// 중복 파일 리스트를 반대로 출력합니다.
/* 파일 크기를 내림차순으로 정렬하여 출력하는 것과 동일합니다. */
int filelist_reverse_print(fileList *head)
{
	fileList *filelist_cur = head->next;
	int set_idx = 1, i = 0;
	int filelist_count = filelist_size(head);

	if(filelist_count < 1)
		return -1;

	// 중복 파일 세트 동적할당하여 배열을 만듭니다.
	fileList **filelist_arr = (fileList **)malloc(sizeof(fileList *) * filelist_count);

	while (filelist_cur != NULL){
		if (fileinfolist_size(filelist_cur->fileInfoList) < 2)
		{	filelist_cur = filelist_cur->next;	continue;	}

		filelist_arr[i] = filelist_cur;
		i++;
		filelist_cur = filelist_cur->next;
	}

	// 중복 파일 세트를 반대로 출력합니다.
	// 크기가 큰 세트부터 출력됩니다.
	i--;
	for(; i >= 0; i--) {
		fileInfo *fileinfolist_cur = filelist_arr[i]->fileInfoList->next;
		char mtime[STRMAX];
		char atime[STRMAX];
		char filesize_w_comma[STRMAX] = {0, };
		int j = 1;

		filesize_with_comma(filelist_arr[i]->filesize, filesize_w_comma);

		printf("---- Identical files #%d (%s bytes - %s) ----\n", set_idx++, filesize_w_comma, filelist_arr[i]->hash);

		while (fileinfolist_cur != NULL){
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_mtime), mtime);
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_atime), atime);
			printf("[%d] %s (mtime : %s) (atime : %s)", j++, fileinfolist_cur->path, mtime, atime);
			printf(" (uid : %d) (gid : %d) (mode : %o)\n", fileinfolist_cur->statbuf.st_uid, fileinfolist_cur->statbuf.st_gid, fileinfolist_cur->statbuf.st_mode);

			fileinfolist_cur = fileinfolist_cur->next;
		}
		printf("\n");
	}

	free(filelist_arr);
	return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_uid1(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	if(ori.statbuf.st_uid > cmp.statbuf.st_uid)
		return 1;
	else if(ori.statbuf.st_uid < cmp.statbuf.st_uid)
		return -1;
	else
		return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_uid2(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	if(ori.statbuf.st_uid < cmp.statbuf.st_uid)
		return 1;
	else if(ori.statbuf.st_uid > cmp.statbuf.st_uid)
		return -1;
	else
		return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_gid1(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	if(ori.statbuf.st_gid > cmp.statbuf.st_gid)
		return 1;
	else if(ori.statbuf.st_gid < cmp.statbuf.st_gid)
		return -1;
	else
		return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_gid2(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	if(ori.statbuf.st_gid < cmp.statbuf.st_gid)
		return 1;
	else if(ori.statbuf.st_gid > cmp.statbuf.st_gid)
		return -1;
	else
		return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_mode1(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	if(ori.statbuf.st_mode > cmp.statbuf.st_mode)
		return 1;
	else if(ori.statbuf.st_mode < cmp.statbuf.st_mode)
		return -1;
	else
		return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_mode2(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	if(ori.statbuf.st_mode < cmp.statbuf.st_mode)
		return 1;
	else if(ori.statbuf.st_mode > cmp.statbuf.st_mode)
		return -1;
	else
		return 0;
}

/* qsort에 사용할 비교 함수들 */
int compare_filename2(const void *a, const void *b) {
	fileInfo ori, cmp;

	memcpy(&ori, a, sizeof(fileInfo));
	memcpy(&cmp, b, sizeof(fileInfo));

	return 0 - pathcmp(ori.path, cmp.path);
}

//파일 리스트를 옵션에 따라 정렬하여 출력하는 함수
void fileinfo_print_sort(fileInfo *head, int category, int order)
{
	int fileinfo_count = fileinfolist_size(head);
	fileInfo *fileinfolist_cur = head->next;
	fileInfo *fileinfo_arr = (fileInfo *)malloc(sizeof(fileInfo) * fileinfo_count);
	int i = 0;

	while (fileinfolist_cur != NULL) {
		memcpy(&fileinfo_arr[i], fileinfolist_cur, sizeof(fileInfo));
		i++;
		fileinfolist_cur = fileinfolist_cur->next;
	}

	// 오름차순 정렬
	if(order > 0) {
		if(category == FILENAME);	// 파일 리스트는 애초에 파일 이름 기준으로 오름차순으로 정렬되어 있기 때문에 아무 작업도 하지 않습니다.
		else if(category == SIZE);	// 파일 리스트는 모두 같은 파일끼리 묶여있기 때문에 아무 작업도 하지 않습니다.
		else if(category == UID)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_uid1);	// uid 순으로 정렬합니다.
		else if(category == GID)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_gid1);	// gid 순으로 정렬합니다.
		else if(category == MODE)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_mode1);	//mode 순으로 정렬합니다.
	}
	// 내림차순 정렬
	else {
		if(category == FILENAME)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_filename2);	// 역순으로 정렬합니다.
		else if(category == SIZE);	//파일 리스트는 모두 같은 파일끼리 묶여있기 때문에 아무 작업도 하지 않습니다.
		else if(category == UID)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_uid2);	//uid 역순으로 정렬합니다.
		else if(category == GID)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_gid2);	//gid 역순으로 정렬합니다.
		else if(category == MODE)
			qsort(fileinfo_arr, fileinfo_count, sizeof(fileInfo), compare_mode2);	//mode 역순으로 정렬합니다.
	}

	// 옵션에 맞게 정렬된 배열을 순서대로 출력합니다.
	for(i = 0; i < fileinfo_count; i++) {
		char mtime[STRMAX];
		char atime[STRMAX];
		char filesize_w_comma[STRMAX] = {0, };

		sec_to_ymdt(localtime(&fileinfo_arr[i].statbuf.st_mtime), mtime);
		sec_to_ymdt(localtime(&fileinfo_arr[i].statbuf.st_atime), atime);
		printf("[%d] %s (mtime : %s) (atime : %s)", i+1, fileinfo_arr[i].path, mtime, atime);
		printf(" (uid : %d) (gid : %d) (mode : %o)\n", fileinfo_arr[i].statbuf.st_uid, fileinfo_arr[i].statbuf.st_gid, fileinfo_arr[i].statbuf.st_mode);
	}
	free(fileinfo_arr);
}

// 파일 세트를 옵션에 따라 정렬하여 출력하는 함수
int filelist_print_sort(fileList *head, int category, int order)
{
	fileList *filelist_cur = head->next;
	int set_idx = 1;	

	if(filelist_size(head) < 1)
		return -1;

	while (filelist_cur != NULL){
		char filesize_w_comma[STRMAX] = {0, };
		int i = 1;

		if (fileinfolist_size(filelist_cur->fileInfoList) < 2)
		{	filelist_cur = filelist_cur->next;	continue;	}
		filesize_with_comma(filelist_cur->filesize, filesize_w_comma);

		printf("---- Identical files #%d (%s bytes - %s) ----\n", set_idx++, filesize_w_comma, filelist_cur->hash);

		fileinfo_print_sort(filelist_cur->fileInfoList, category, order);

		printf("\n");

		filelist_cur = filelist_cur->next;
	}
}

// 해시값을 추출하는 함수
int sha1(char *target_path, char *hash_result)
{
	FILE *fp;
	unsigned char hash[SHA_DIGEST_LENGTH];
	unsigned char buffer[SHRT_MAX];
	int bytes = 0;
	SHA_CTX sha1;

	if ((fp = fopen(target_path, "rb")) == NULL){
		printf("ERROR: fopen error for %s\n", target_path);
		return 1;
	}

	SHA1_Init(&sha1);

	while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
		SHA1_Update(&sha1, buffer, bytes);

	SHA1_Final(hash, &sha1);

	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
		sprintf(hash_result + (i * 2), "%02x", hash[i]);
	hash_result[HASHMAX-1] = 0;

	fclose(fp);

	return 0;
}

void hash_func(char *path, char *hash)
{
#ifdef HASH_SHA1
	sha1(path, hash);
#endif
}

// 재귀 bfs로 탐색하는 함수
void dir_traverse(dirList *dirlist)
{
	dirList *cur = dirlist->next;
	dirList *subdirs = (dirList *)malloc(sizeof(dirList));

	memset(subdirs, 0 , sizeof(dirList));

	while (cur != NULL){
		struct dirent **namelist;
		int listcnt;

		listcnt = get_dirlist(cur->dirpath, &namelist);

		for (int i = 0; i < listcnt; i++){
			char fullpath[PATHMAX] = {0, };
			struct stat statbuf;
			int file_mode;
			long long filesize;

			if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
				continue;

			get_fullpath(cur->dirpath, namelist[i]->d_name, fullpath);

			if (!strcmp(fullpath,"/proc") || !strcmp(fullpath, "/run") || !strcmp(fullpath, "/sys") || !strcmp(fullpath, trash_path))
				continue;

			file_mode = get_file_mode(fullpath, &statbuf);

			if ((filesize = (long long)statbuf.st_size) == 0)
				continue;

			if (file_mode == DIRECTORY)
				dirlist_append(subdirs, fullpath);       
			else if (file_mode == REGFILE){
				FILE *fp;
				char filename[PATHMAX*2];
				char *path_extension;
				char hash[HASHMAX];

				if (filesize < minbsize)
					continue;

				if (maxbsize != -1 && filesize > maxbsize)
					continue;

				sprintf(filename, "%s/%lld", same_size_files_dir, filesize);

				memset(hash, 0, HASHMAX);
				hash_func(fullpath, hash);

				path_extension = get_extension(fullpath);

				if (strlen(extension) != 0){
					if (path_extension == NULL || strcmp(extension, path_extension))
						continue;
				}

				if ((fp = fopen(filename, "a")) == NULL){
					printf("ERROR: fopen error for %s\n", filename);
					return;
				}

				fprintf(fp, "%s %s\n", hash, fullpath);

				fclose(fp);
			}
		}

		cur = cur->next;
	}

	dirlist_delete_all(dirlist);

	if (subdirs->next != NULL)
			dir_traverse(subdirs);
}

// 쓰레드가 탐색을 수행할 함수
/* 하나의 노드만 탐색한 후 인자로 받은 헤더에 디렉토리 리스트를 붙입니다. */
void *thread_search(void *a)
{
	dirList **dirlist = (dirList **)a;
	if((*dirlist)->next == NULL) {
		(*dirlist)->next = NULL;
		pthread_exit(NULL);
		return NULL;
	}
	dirList *cur = (*dirlist)->next;
	dirList *subdirs = (dirList *)malloc(sizeof(dirList));

	memset(subdirs, 0 , sizeof(dirList));

	while (cur != NULL){
		struct dirent **namelist;
		int listcnt;

		listcnt = get_dirlist(cur->dirpath, &namelist);

		for (int i = 0; i < listcnt; i++){
			char fullpath[PATHMAX] = {0, };
			struct stat statbuf;
			int file_mode;
			long long filesize;

			if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
				continue;

			get_fullpath(cur->dirpath, namelist[i]->d_name, fullpath);

			if (!strcmp(fullpath,"/proc") || !strcmp(fullpath, "/run") || !strcmp(fullpath, "/sys") || !strcmp(fullpath, trash_path))
				continue;

			file_mode = get_file_mode(fullpath, &statbuf);

			if ((filesize = (long long)statbuf.st_size) == 0)
				continue;

			if (file_mode == DIRECTORY)
				dirlist_append(subdirs, fullpath);       
			else if (file_mode == REGFILE){
				FILE *fp;
				char filename[PATHMAX*2];
				char *path_extension;
				char hash[HASHMAX];

				if (filesize < minbsize)
					continue;

				if (maxbsize != -1 && filesize > maxbsize)
					continue;

				sprintf(filename, "%s/%lld", same_size_files_dir, filesize);

				memset(hash, 0, HASHMAX);
				hash_func(fullpath, hash);

				path_extension = get_extension(fullpath);

				if (strlen(extension) != 0){
					if (path_extension == NULL || strcmp(extension, path_extension))
						continue;
				}

				if ((fp = fopen(filename, "a")) == NULL){
					printf("ERROR: fopen error for %s\n", filename);
					return NULL;
				}

				fprintf(fp, "%s %s\n", hash, fullpath);

				fclose(fp);
			}
		}

		cur = cur->next;
	}

	dirlist_delete_all(*dirlist);

	if (subdirs->next != NULL)
		*dirlist = subdirs;
	else
		(*dirlist)->next = NULL;
	pthread_exit(NULL);
}

// 해시값을 비교하여 동일한 파일들을 찾는 함수
void find_duplicates(void)
{
	struct dirent **namelist;
	int listcnt;
	char hash[HASHMAX];
	FILE *fp;

	listcnt = get_dirlist(same_size_files_dir, &namelist);

	for (int i = 0; i < listcnt; i++){
		char filename[PATHMAX*2];
		long long filesize;
		char filepath[PATHMAX];
		char hash[HASHMAX];
		char line[STRMAX];

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		filesize = atoll(namelist[i]->d_name);
		sprintf(filename, "%s/%s", same_size_files_dir, namelist[i]->d_name);

		if ((fp = fopen(filename, "r")) == NULL){
			printf("ERROR: fopen error for %s\n", filename);
			continue;
		}

		while (fgets(line, sizeof(line), fp) != NULL){
			int idx;

			strncpy(hash, line, HASHMAX);
			hash[HASHMAX-1] = '\0';

			strcpy(filepath, line+HASHMAX);
			filepath[strlen(filepath)-1] = '\0';

			if ((idx = filelist_search(dups_list_h, hash)) == 0)
				filelist_append(dups_list_h, filesize, filepath, hash);
			else{
				fileList *filelist_cur = dups_list_h;
				while (idx--){
					filelist_cur = filelist_cur->next;
				}
				fileinfo_append(filelist_cur->fileInfoList, filepath);
			}
		}

		fclose(fp);
	}
}

// 리스트에서 중복이 없는 파일들을 제거하는 함수
void remove_no_duplicates(void)
{
	fileList *filelist_cur = dups_list_h->next;

	while (filelist_cur != NULL){
		fileInfo *fileinfo_cur = filelist_cur->fileInfoList;

		if (fileinfolist_size(fileinfo_cur) < 2)
			filelist_delete_node(dups_list_h, filelist_cur->hash);

		filelist_cur = filelist_cur->next;
	}
}

time_t get_recent_mtime(fileInfo *head, char *last_filepath)
{
	fileInfo *fileinfo_cur = head->next;
	time_t mtime = 0;

	while (fileinfo_cur != NULL){
		if (fileinfo_cur->statbuf.st_mtime > mtime){
			mtime = fileinfo_cur->statbuf.st_mtime;
			strcpy(last_filepath, fileinfo_cur->path);
		}
		fileinfo_cur = fileinfo_cur->next;
	}
	return mtime;
}

// fsha1 명령어 이후 삭제 옵션을 입력받아 수행하는 함수
void delete_prompt(void)
{
	while (filelist_size(dups_list_h) > 0){
		char input[STRMAX];
		char last_filepath[PATHMAX];
		char modifiedtime[STRMAX];
		char *argv[3], *argv2[ARGMAX];
		int argc;
		int set_idx;
		time_t mtime = 0;
		fileList *target_filelist_p;
		fileInfo *target_infolist_p;
		char c;

		optind = 1;
		printf(">> ");

		fgets(input, sizeof(input), stdin);

		if (!strcmp(input, "exit\n")){
			printf(">> Back to Prompt\n");
			break;
		}
		else if(!strcmp(input, "\n"))
			continue;

		argc = tokenize(input, argv2);

		if (strcmp(argv2[0], "delete"))
		{
			printf("Usage: delete -l [SET_IDX] -d [LIST_IDX] -i -f -t\n");
			continue;
		}

		// 인자를 getopt를 사용하여 옵션과 인자를 구분합니다.
		/* 입력받은 인자는 argv 배열에 저장합니다. */
		int list_idx = -1;
		char set_index[20] = {0, }, tmp[2] = {0, }, list_index[20] = {0, };
		argv[0] = set_index;
		argv[2] = list_index;
		argv[1] = tmp;
		while ((c = getopt(argc, argv2, "iftd:l:")) != -1)
		{
			switch (c)
			{
				case 'l':
					strcpy(set_index, optarg);
					break;
				case 'd':
					strcpy(list_index, optarg);
					strcpy(tmp, "d");
					break;
				case 'i':
					strcpy(tmp, "i");
					strcpy(argv[2], "");
					break;
				case 'f':
					strcpy(tmp, "f");
					strcpy(argv[2], "");
					break;
				case 't':
					strcpy(tmp, "t");
					strcpy(argv[2], "");
					break;
			}

		}

		if (!atoi(argv[0])){
			printf("ERROR: [SET_INDEX] should be a number\n");
			continue;
		}

		if (atoi(argv[0]) < 0 || atoi(argv[0]) > filelist_size(dups_list_h)){
			printf("ERROR: [SET_INDEX] out of range\n");
			continue;
		}

		target_filelist_p = dups_list_h->next;

		set_idx = atoi(argv[0]);

		if (target_filelist_p->fileInfoList->next->next == NULL)
			set_idx++;
		while (--set_idx)
		{
			target_filelist_p = target_filelist_p->next;
			if (target_filelist_p->fileInfoList->next->next == NULL)
			{	set_idx++;	continue;	}
		}

		target_infolist_p = target_filelist_p->fileInfoList;

		mtime = get_recent_mtime(target_infolist_p, last_filepath);
		sec_to_ymdt(localtime(&mtime), modifiedtime);

		set_idx = atoi(argv[0]);

		if (!strcmp(argv[1],"f")){
			fileInfo *tmp;				
			fileInfo *deleted = target_infolist_p->next;

			while (deleted != NULL){
				tmp = deleted->next;

				if (!strcmp(deleted->path, last_filepath)){
					deleted = tmp;
					continue;
				}
				remove(deleted->path);
				write_log(DELETE, deleted->path);	//삭제 후 로그를 작성합니다.
				fileinfo_delete_node(target_infolist_p, deleted->path);
				deleted = tmp;
			}

			filelist_delete_node(dups_list_h, target_filelist_p->hash);
			printf("Left file in #%d : %s (%s)\n\n", atoi(argv[0]), last_filepath, modifiedtime);
		}
		else if(!strcmp(argv[1],"t")){
			fileInfo *tmp;
			fileInfo *deleted = target_infolist_p->next;
			char move_to_trash[PATHMAX];
			char move_to_info[PATHMAX];
			char filename[PATHMAX];
			FILE *fp;

			while (deleted != NULL){
				tmp = deleted->next;

				if (!strcmp(deleted->path, last_filepath)){
					deleted = tmp;
					continue;
				}

				memset(move_to_trash, 0, sizeof(move_to_trash));
				memset(move_to_trash, 0, sizeof(move_to_info));
				memset(filename, 0, sizeof(filename));

				//쓰레기통 경로와 쓰레기통 정보 파일 경로를 만듭니다.
				sprintf(move_to_trash, "%s%s", trash_path, strrchr(deleted->path, '/') + 1);
				sprintf(move_to_info, "%s%s", trash_info_path, strrchr(deleted->path, '/') + 1);

				if (access(move_to_trash, F_OK) == 0){
					get_new_file_name(deleted->path, filename);

					strncpy(strrchr(move_to_trash, '/') + 1, filename, strlen(filename) + 1);
					strncpy(strrchr(move_to_info, '/') + 1, filename, strlen(filename) + 1);
				}
				else {
					strcpy(filename, strrchr(deleted->path, '/') + 1);
					strncpy(strrchr(move_to_trash, '/') + 1, filename, strlen(filename) + 1);
					strncpy(strrchr(move_to_info, '/') + 1, filename, strlen(filename) + 1);
				}

				// 파일을 쓰레기통으로 옮깁니다.
				if (rename(deleted->path, move_to_trash) == -1){
					printf("ERROR: Fail to move duplicates to Trash\n");
					deleted = tmp;
					continue;
				}
				write_log(REMOVE, deleted->path);	//삭제 후 로그를 작성합니다.

				strcat(move_to_info, ".trashinfo");

				//쓰레기통 정보 파일을 만들어서 엽니다.
				if((fp = fopen(move_to_info, "w")) == NULL) {
					fprintf(stderr, "fopen error for %s\n", move_to_info);
					continue;
				}

				time_t t = time(NULL);
				struct tm *time;
				time = localtime(&t);

				//쓰레기통 정보 파일에 파일 경로와 삭제 날짜, 시간과 파일의 해시값을 저장합니다.
				fprintf(fp, "[Trash Info]\npath=%s\nDeletionDate=", deleted->path);
				fprintf(fp, "%04d-%02d-%02dT%02d:%02d:%02d\n", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
				fprintf(fp, "%s", target_filelist_p->hash);

				fclose(fp);

				fileinfo_delete_node(target_infolist_p, deleted->path);
				deleted = tmp;
			}

			printf("All files in #%d have moved to Trash except \"%s\" (%s)\n\n", atoi(argv[0]), last_filepath, modifiedtime);
		}
		else if(!strcmp(argv[1],"i")){
			char ans[STRMAX];
			fileInfo *fileinfo_cur = target_infolist_p->next;
			fileInfo *deleted_list = (fileInfo *)malloc(sizeof(fileInfo));
			fileInfo *tmp;
			int listcnt = fileinfolist_size(target_infolist_p);


			while (fileinfo_cur != NULL && listcnt--){
				printf("Delete \"%s\"? [y/n] ", fileinfo_cur->path);
				memset(ans, 0, sizeof(ans));
				fgets(ans, sizeof(ans), stdin);

				if (!strcmp(ans, "y\n") || !strcmp(ans, "Y\n")){
					remove(fileinfo_cur->path);
					write_log(DELETE, fileinfo_cur->path);	//삭제 후 로그를 작성합니다.
					fileinfo_cur = fileinfo_delete_node(target_infolist_p, fileinfo_cur->path);				
				}
				else if (!strcmp(ans, "n\n") || !strcmp(ans, "N\n"))
					fileinfo_cur = fileinfo_cur->next;										
				else {
					printf("ERROR: Answer should be 'y/Y' or 'n/N'\n");
					break;
				}
			}
			printf("\n");

			if (fileinfolist_size(target_infolist_p) < 2)
				filelist_delete_node(dups_list_h, target_filelist_p->hash);
		}
		else if(!strcmp(argv[1], "d")){
			fileInfo *deleted;
			int list_idx;

			if (argv[2] == NULL || (list_idx = atoi(argv[2])) == 0){
				printf("ERROR: There should be an index\n");
				continue;
			}

			if (list_idx < 0 || list_idx > fileinfolist_size(target_infolist_p)){
				printf("ERROR: [LIST_IDX] out of range\n");
				continue;
			}

			deleted = target_infolist_p;

			while (list_idx--)
				deleted = deleted->next;

			printf("\"%s\" has been deleted in #%d\n\n", deleted->path, atoi(argv[0]));
			remove(deleted->path);
			write_log(DELETE, deleted->path);	//삭제 후 로그를 작성합니다.
			fileinfo_delete_node(target_infolist_p, deleted->path);

			if (fileinfolist_size(target_infolist_p) < 2)
				filelist_delete_node(dups_list_h, target_filelist_p->hash);
		}
		else {
			printf("ERROR: Only f, t, i, d options are available\n");
			continue;
		}

		filelist_print_format(dups_list_h);
	}
}

// 쓰레기통 연결리스트를 만듭니다.
int trash_list(int category, int order)
{
	FILE *fp;
	char trash_info_realpath[PATHMAX+257];
	struct dirent **namelist;
	char buf[PATHMAX];
	char path[PATHMAX], date[11], time[9];
	int size = 0;
	int i, j;

	int listcnt = get_dirlist(trash_info_path, &namelist);

	//반복문을 돌며 쓰레기통 디렉토리의 파일들을 확인하며 쓰레기통 연결 리스트에 추가합니다.
	for (i = 0; i < listcnt; i++) {

		memset(path, 0, sizeof(path));
		memset(date, 0, sizeof(date));
		memset(time, 0, sizeof(time));
		size = 0;

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		// 해당 파일의 쓰레기통 정보 파일 이름을 만들어서 엽니다.
		sprintf(trash_info_realpath, "%s%s", trash_info_path, namelist[i]->d_name);

		if((fp = fopen(trash_info_realpath, "r")) == NULL) {
			fprintf(stderr, "fopen error for %s\n", trash_info_realpath);
			continue;
		}

		// 원래 경로를 저장합니다.
		fseek(fp, (off_t)18, SEEK_SET);
		fgets(buf, PATHMAX, fp);
		buf[strlen(buf)-1] = '\0';
		strcpy(path, buf);

		//삭제된 날짜를 저장합니다.
		fread(buf, sizeof(char), 13, fp);
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), 10, fp);
		strcpy(date, buf);

		// 삭제된 시간을 저장합니다.
		fread(buf, sizeof(char), 1, fp);
		memset(buf, 0, sizeof(buf));
		fread(buf, sizeof(char), 8, fp);
		strcpy(time, buf);

		fclose(fp);

		/* 파일 크기를 얻기 위해 휴지통의 파일을 열어 ftell() 함수로 크기를 확인합니다. */
		sprintf(trash_info_realpath, "%s%s", trash_path, namelist[i]->d_name);
		for(j = strlen(trash_info_realpath); trash_info_realpath[j] != '.'; j--);
		trash_info_realpath[j] = 0;

		if((fp = fopen(trash_info_realpath, "r")) == NULL) {
			fprintf(stderr, "fopen error for %s\n", trash_info_realpath);
			continue;
		}

		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		/* 크기 확인 완료 */
		fclose(fp);

		trashlist_append(trash_list_h, path, size, date, time);	//쓰레기통 연결 리스트 끝에 추가합니다.
	}
	if(i < 3)
		return -1;	//쓰레기통이 비어있으면 -1을 반환합니다.

	else {
		trashlist_sort(trash_list_h, category, order);	//옵션에 맞게 쓰레기통을 정렬합니다.
		return 0;
	}
}
