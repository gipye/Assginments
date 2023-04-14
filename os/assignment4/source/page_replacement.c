#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<time.h>

#define MIN_STREAM_LENGTH	500
#define MAX_TABLE_SIZE		10
#define MAX_INPUT_LENGTH	50
#define PRINT_BUFFER_LENGTH	100

enum Algorithm {
	OPTIMAL_NUM=1, FIFO_NUM=2, LIFO_NUM=3, LRU_NUM=4, LFU_NUM=5, SC_NUM=6, ESC_NUM=7, ALL_NUM=8
};

typedef unsigned char user_t;
typedef struct Clock {
	user_t R	: 1;
	user_t W	: 1;
	user_t number	: sizeof(user_t)*8-2;
} page_t;

/* 스택 자료구조 */
struct Stack {
	page_t stack[MAX_TABLE_SIZE];
	int top, size;
};

/* 큐 자료구조 */
struct Queue {
	page_t queue[MAX_TABLE_SIZE+1];
	int front, rear, size;
};

/* LRU, LFU를 위한 데이터를 저장할 자료구조 */
struct Least {
	unsigned int time;
	unsigned int count;
};

/* 전역변수들 */
char token[10];					// 입력 받은 토큰을 나눠서 반환할 변수
int missPointer, faultCounter;	// page fault 시 활용할 변수
int hitPointer, hitCounter;		// hit일 경우 활용할 변수
FILE *fp;
char *filename;
int streamLength;				// page stream의 총 길이를 저장할 변수
char **printBuffer;				// 테이블의 내용을 출력하기 위해 담아두는 변수

/* 페이지 교체 정책 알고리즘을 수행하는 함수 */
void Optimal(page_t *, int);
void FIFO(page_t *, int);
void LIFO(page_t *, int);
void LRU(page_t *, int);
void LFU(page_t *, int);
void SC(page_t *, int);
void ESC(page_t *, int);
void ALL(page_t *, int);

/* 스택 연산을 위한 함수들 */
void stackInit(struct Stack *s, int size) {
	memset(s, 0, sizeof(struct Stack));
	s->top = -1;
	s->size = size;
}
int isSFull(struct Stack *s) {
	if(s->size-1 <= s->top)
		return 1;
	return 0;
}
int push(struct Stack *s, page_t value) {
	if(isSFull(s))
		return 0;
	s->stack[++(s->top)] = value;
	return 1;
}
int pop(struct Stack *s) {
	if(s->top < 0)
		return 0;
	return s->stack[(s->top)--].number;
}

/* 큐 연산을 위한 함수들 */
void queueInit(struct Queue *q, int size) {
	memset(q, 0, sizeof(struct Queue));
	q->front = -1;
	q->rear = -1;
	q->size = size;
}
int isQFull(struct Queue *q) {
	int tmp = (q->rear+1)%(q->size+1);
	if(q->front == tmp)
		return 1;
	else return 0;
}
int add(struct Queue *q, page_t value) {
	if(isQFull(q))
		return 0;
	q->rear = (q->rear+1)%(q->size);
	q->queue[q->rear] = value;
	return 1;
}
int poll(struct Queue *q){
	if(q->front == q->rear)
		return 0;

	q->front = (q->front+1)%(q->size+1);
	return q->queue[q->front].number;
}

/* 출력할 내용을 저장하기 위한 함수들 */
void addTableInPrintBuffer(page_t *, int, int, struct Least *);
void addStringInPrintBuffer(int, char *);
void printBufferFlush(int);

/* 문자열을 separator로 나누어 반환하는 함수 */
char *stringTokenizer(char **string, char *separators){
	int i = -1, j = 0;
	int flag = 1;

	if((*string)[0] == 0)	// 토큰이 더 없으면 NULL을 반환합니다.
		return NULL;

	// separators로 지정된 문자들은 건너뜁니다.
	while(flag) {
		i++;
		flag = 0;
		for(j = 0; separators[j] != '\0'; j++) {
			if(separators[j] == (*string)[i]) {
				flag = 1;
				break;
			}
		}
	}

	// separators로 지정된 문자가 아닌 문자부터 전역변수 token에 문자열을 복사합니다.
	strcpy(token, &(*string)[i]);

	// separators로 지정된 문자가 처음 나올 때 까지 인덱스 i를 증가시킵니다.
	flag = 1;
	i--;
	while(flag) {
		i++;
		for(j = 0; separators[j] != '\0'; j++) {
			if(separators[j] == (*string)[i]) {
				flag = 0;
				break;
			}
		}
		if((*string)[i] == 0)
			break;
	}

	// 다음 이 함수를 호출할 때 그 다음 토큰을 읽을 수 있도록 *string가 가리키는 곳을
	// 다음 토큰의 시작으로 움직이고 separators로 지정된 문자가 나온 부분에
	// 널문자를 넣어 해당 토큰의 문자열만 얻을 수 있도록 합니다.
	if((*string)[i] == '\0')
		*string = &((*string)[i]);
	else {
		token[i] = 0;
		*string = &((*string)[i+1]);
	}

	return token;
}

/* 함수 포인터에 주소를 넣기 위해 만든 함수 */
/* 인자로 들어온 번호에 해당하는 알고리즘 수행 함수의 주소를 반환합니다. */
void *getAlgorithmFunc(int algorithm) {
	if(algorithm == OPTIMAL_NUM)
		return &Optimal;
	if(algorithm == FIFO_NUM)
		return &FIFO;
	if(algorithm == LIFO_NUM)
		return &LIFO;
	if(algorithm == LRU_NUM)
		return &LRU;
	if(algorithm == LFU_NUM)
		return &LFU;
	if(algorithm == SC_NUM)
		return &SC;
	if(algorithm == ESC_NUM)
		return &ESC;
	if(algorithm == ALL_NUM)
		return &ALL;
}

/* 입력값에 따라 페이지 스트림을 생성하고 선택된 페이지 교체 알고리즘 함수를 호출하는 함수 */
void simulator(int algorithm[], int framsNum, int dataInputType) {
	page_t *pageStream;		// 페이지 스트림을 담을 변수
	int i;					// for문을 위한 인덱스
	void (*algorithmFunc)(page_t *, int);	// 페이지 교체 알고리즘 함수 포인터
	int fd;					// 파일 디스크립터
	off_t fsize;			// 파일 전체 크기
	char readBuf[6];		// 파일 읽은 내용을 담을 버퍼, 페이지 하나 당 파일에는 6 개의 문자로 정보가 저장되어 있습니다.

	// 만약 페이지 넘버가 7, R비트가 1, W비트가 0일 경우
	// 파일에는 07R1W0 이렇게 6개의 문자로 저장되어 있습니다.

	if(dataInputType == 1) {	// 랜덤으로 생성을 선택한 경우
		streamLength = MIN_STREAM_LENGTH;
		pageStream = (page_t *)malloc(sizeof(page_t)*streamLength);

		srand((unsigned int)time(NULL));
		for(i = 0; i < streamLength; i++) {
			// R비트가 1이고 W비트가 0인 것을 허용하는 방식으로 했습니다.
			pageStream[i].number = rand()%30 + 1;	// 페이지 넘머 1부터 30 랜덤으로 생성
			pageStream[i].R = rand()%2;		// R비트 랜덤으로 생성
			pageStream[i].W = rand()%2;		// W비트 랜덤으로 생성
		}
	}
	else if(dataInputType == 2) {	// 파일에서 읽기를 선택한 경우

		// 전역변수에 저장된 파일 이름으로 파일을 열고 에러처리합니다.
		if((fd = open(filename, O_RDONLY)) < 0) {
			fprintf(stderr, "Error: %s file not exist\n", filename);
			exit(2);
		}

		// 파일 전체 크기를 저장합니다.
		if((fsize = lseek(fd, 0, SEEK_END)) < 0) {
			fprintf(stderr, "lseek error\n");
			exit(3);
		}

		// 페이지 하나 당 파일에는 6바이트로 저장되어 있기 때문에 페이지 스트림의 개수는 
		// 전체 파일 크기를 6으로 나눠서 구하고 페이지 스트림을 담을 변수의 메모리를
		// 동적할당합니다.
		streamLength = fsize / 6;
		pageStream = (page_t *)malloc(sizeof(page_t)*streamLength);

		// 오프셋을 다시 파일의 처음으로 설정합니다.
		if(lseek(fd, 0, SEEK_SET) < 0) {
			fprintf(stderr, "lseek error\n");
			exit(4);
		}

		// 파일에서 읽어서 페이지 정보를 저장합니다.
		i = 0;
		while(read(fd, readBuf, sizeof(readBuf)) > 0) {
			pageStream[i].number = (readBuf[0] - '0')*10 + readBuf[1] - '0';
			pageStream[i].R = readBuf[3] - '0';
			pageStream[i].W = readBuf[5] - '0';
			i++;
		}

		close(fd);
	}

	if((fp = fopen("result.txt", "w")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(5);
	}

	// 페이지 스트림 전체를 시뮬레이션 하기 전에 화면과 파일에 한번 출력합니다.
	// 페이지 번호가 6이고 R비트가 0, W비트가 1일 경우 6W,
	// 페이지 번호가 15이고 R비트가 0, W비트가 0일 경우 15,
	// 페이지 번호가 15이고 R비트가 1, W비트가 1일 경우 15RW,
	// 와 같은 형식으로 출력합니다.
	printf("page stream\n");
	fprintf(fp, "page stream\n");
	for(i = 0; i < streamLength-1; i++) {
		printf("%d", pageStream[i].number);
		fprintf(fp, "%d", pageStream[i].number);
		if(pageStream[i].R) {
			printf("R");
			fprintf(fp, "R");
		}
		if(pageStream[i].W) {
			printf("W");
			fprintf(fp, "W");
		}
		printf(", ");
		fprintf(fp, ", ");
	}
	printf("%d", pageStream[i].number);
	fprintf(fp, "%d", pageStream[i].number);
	if(pageStream[i].R) {
		printf("R");
		fprintf(fp, "R");
	}
	if(pageStream[i].W) {
		printf("W");
		fprintf(fp, "W");
	}
	printf("\n");
	fprintf(fp, "\n");

	// 시뮬레이션 과정을 출력할 내용을 담아 둘 변수를 동적할당합니다.
	printBuffer = (char **)malloc(sizeof(char *)*framsNum);
	for(i = 0; i < framsNum; i++) {
		printBuffer[i] = (char *)malloc(sizeof(char)*PRINT_BUFFER_LENGTH);
		memset(printBuffer[i], 0, sizeof(printBuffer[i]));
	}

	// 입력받은 알고리즘을 수행할 함수를 호출합니다.
	for(i = 0; algorithm[i] > 0; i++) {
		algorithmFunc = getAlgorithmFunc(algorithm[i]);
		algorithmFunc(pageStream, framsNum);

		// 페이지 스트림 전체 길이를 출력합니다.
		printf("streamLength: %d\n", streamLength);
		fprintf(fp, "streamLength: %d\n", streamLength);
	}

	// 파일을 닫습니다.
	fclose(fp);

	// 동적 할당된 메모리를 해제합니다.
	for(i = 0; i < framsNum; i++)
		free(printBuffer[i]);
	free(printBuffer);
	free(pageStream);
}

int main(int argc, char *argv[]){
	// 실행 시 인자가 2개 보다 많으면 에러처리합니다.
	if(argc > 2) {
		fprintf(stderr, "Usage: \"%s\" or \"%s <filename>\"\n", argv[0], argv[0]);
		exit(1);
	}

	filename = argv[1];		// 두 번째 인자는 페이지 스트림을 받을 파일 이름이므로 전역변수에 저장합니다.
	int algorithm[4], framsNum, dataInputType, i;		// 입력받기 위한 변수와 인덱스 변수
	char *input, *buf, c, error = 1;

	// 문자열 입력받을 변수 input을 동적할당합니다.
	// input가 가리키는 위치는 stringTokenizer() 함수를 호출하면 바뀌므로
	// buf 변수에 처음 할당된 메모리 위치를 저장해둡니다.
	input = (char *)malloc(sizeof(char) * MAX_INPUT_LENGTH);
	buf = input;

	printf("A. Page Replacement 알고리즘 시뮬레이터를 선택하시요 (최대 3개)\n");
	printf(" (1) Optimal (2) FIFO (3) LIFO (4) LRU (5) LFU (6) SC (7) ESC (8) ALL\n");

	// 알고리즘 시뮬레이터를 최대 3개까지 입력받고 잘못된 입력 시 다시 입력받습니다.
	while(error) {
		input = buf;	// 다시 입력받기 위해 buf에 저장된 값을 input에 대입합니다.
		error = 0;		// error을 0으로 만들어 입력값이 올바르면 반복문을 나갈 수 있도록 합니다.
		for(i = 0; i < 4; i++)	// 변수 초기화
			algorithm[i] = 0;

		scanf("%[^\n]", input);
		c = getchar();

		// 최대 3개까지의 입력만 저장하고 이후의 추가된 입력은 무시합니다.
		for(i = 0; i < 3; i++) {
			char *token;
			token = stringTokenizer(&input, " \t\n");	// 탭에나 공백으로 토큰을 구분합니다.
			if(token != NULL)	// 토큰이 있다면 정수로 그 값을 저장합니다.
				algorithm[i] = atoi(token);
			else if(algorithm[0] > 0)	// 만약 토큰이 NULL이지만 이전에 입력받은 토큰이 1개라도 있었다면 반복문을 탈출합니다.
				break;

			// 만약 1부터 8 사이의 값이 아니면 다시 입력받습니다.
			if(algorithm[i] < 1 || algorithm[i] > 8) {
				error = 1;
				printf("User input error: Usage 1 ~ 8\n");
				printf("다시 입력하세요. ");
				break;
			}
		}
	}

	// 페이지 프레임의 개수를 입력받습니다.
	error++;
	printf("B. 페이지 프레임의 개수를 입력하시오. (3~10)\n");

	do {
		input = buf;	// 변수 초기화
		input[0] = 0;
		scanf("%[^\n]", input);
		c = getchar();

		if(input[0] == 0) {
			printf("User input error: Usage 3 ~ 10\n");
			printf("다시 입력하세요. ");
			continue;
		}

		framsNum = atoi(input);

		// 만약 3부터 10 사이의 입력이 아니라면 다시 입력받습니다.
		if(framsNum >= 3 && framsNum <= 10)
			break;

		printf("User input error: Usage 3 ~ 10\n");
		printf("다시 입력하세요. ");
	}while(error);

	// 데이터 입력 방식을 입력받습니다.
	printf("C. 데이터 입력 방식을 선택하시오. (1, 2)\n");
	printf("(1) 랜덤하게 생성\t\t(2) 사용자 생성 파일 오픈\t\t(3) 프로그램 종료\n");

	do {
		input[0] = 0;		// 변수 초기화
		scanf("%[^\n]", input);
		c = getchar();

		if(input[0] == 0) {
			printf("User input error: Usage 1 ~ 3\n");
			printf("다시 입력하세요. ");
			continue;
		}

		dataInputType = atoi(input);

		if(dataInputType == 1 || dataInputType == 2)
			break;
		else if(dataInputType == 3)	// 3을 입력받았다면 프로그램을 종료합니다.
			exit(0);

		// 잘못된 입력 시 다시 입력받습니다.
		printf("User input error: Usage 1 ~ 3\n");
		printf("다시 입력하세요. ");
	}while(error);

	// 이제 제대로 된 입력값에 따른 수행을 위해 simulator() 함수를 호출합니다.
	simulator(algorithm, framsNum, dataInputType);

	// 메모리 해제
	free(buf);
	exit(0);
}

/* 해당 페이지 테이블에 대해 페이지가 페이지 폴트인지 확인하는 함수 */
/* 페이지 폴트가 아니라면 전역변수 hitPointer가 테이블에서 해당 페이지의 위치를 가리킵니다. */
int checkPageFault(page_t *table, int tableSize, page_t page) {
	for(int i = 0; i < tableSize; i++) {
		if(table[i].number == page.number) {
			hitPointer = i;
			return 0;
		}
	}
	return 1;
}

/* 페이지 테이블이 꽉 찼는지 확인하는 함수 */
/* 테이블이 아직 다 차지 않았다면 전역변수 missPointer가 비어있는 위치를 가리킵니다. */
/* 테이블이 꽉 찼다면 전역변수 missPointer은 변하지 않습니다. */
int isTableFull(page_t *table, int tableSize) {
	for(int i = 0; i < tableSize; i++) {
		if(table[missPointer].number == 0)
			return 0;
		missPointer = (missPointer+1) % tableSize;
	}

	// for문을 전부 다 돌고 나올 경우 전역변수 missPointer은 처음 값으로 돌아오게 됩니다.
	return 1;
}

/* optimal 알고리즘을 수행하는 함수 */
void Optimal(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;	// 변수 초기화

	page_t *table = (page_t *)malloc(sizeof(page_t)*tableSize);	// 페이지 테이블을 동적할당합니다
	int *metaInfoTable = (int *)malloc(sizeof(int)*tableSize);	// 알고리즘 수행을 위한 추가 정보를 저장할 변수도 동적할당하여 만듭니다.

	memset(table, 0, sizeof(table));

	printf("\n\nOptimal start\n");
	fprintf(fp, "\n\nOptimal start\n");

	// 처음 missPointer을 0으로 설정하여 테이블의 0번째 칸부터 값을 채웁니다.
	missPointer = 0;
	for(int i = 0; i < streamLength; i++) {

		// 페이지 폴트인지 확인합니다.
		if(checkPageFault(table, tableSize, pageStream[i])) {
			printf("%d. page %2d fault!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d fault!!\n", i+1, pageStream[i].number);
			faultCounter++;		// 페이지 폴트일 경우 faultCounter을 1 증가시킵니다.

			// 테이블이 변경되기 전 상태를 출력하기 전에 변수에 저장합니다.
			addTableInPrintBuffer(table, tableSize, OPTIMAL_NUM, NULL);
			// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");
			addStringInPrintBuffer(tableSize/2, "=>");
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");

			// 테이블이 꽉 찬 경우 missPointer이 바꿀 페이지의 위치를 가리키도록 합니다.
			if(isTableFull(table, tableSize)) {
				// 변수 초기화
				int k = i+1, j;
				for(j = 0; j < tableSize; j++)
					metaInfoTable[j] = 0;

				// 테이블 크기 - 1 만큼 반복합니다.
				for(j = 1; j < tableSize; j++) {
					// 페이지 스트림의 다음에 올 페이지들을 보며 페이지 테이블에 있는 페이지인지
					// 확인합니다.
					// checkPageFault() 함수의 반환값이 0일 경우 테이블에 있다는 의미이고
					// hitPointer는 테이블에서 해당 위치를 가리킵니다.
					while(k<streamLength && (checkPageFault(table, tableSize, pageStream[k++]) || metaInfoTable[hitPointer]));
					if(k > streamLength)
						break;

					// metaInfoTable 의 해당 위치에 값을 참으로 만듭니다.
					metaInfoTable[hitPointer]++;

					// 위 과정을 tableSize-1 번 반복하면 metaInfoTable에서
					// 페이지 테이블의 페이지들 중 가장 나중에 나올 페이지가 있는
					// 위치에 해당하는 칸만 0이 됩니다.
				}

				// metaInfoTable에서 0이 들어있는 칸을 missPointer이 가리키도록 합니다.
				for(k = 0; k < tableSize; k++)
					if(metaInfoTable[k] == 0) {
						missPointer = k;
						break;
					}

				// 이제 missPointer은 페이지 테이블에 있는 페이지 중에서 가장 나중에
				// 사용될 페이지가 있는 칸을 가리킵니다.
			}

			// 테이블이 꽉 차지 않았다면 missPointer은 비어있는 곳을 가리킵니다.

			// 이제 missPointer이 가리키는 곳에 해당 페이지를 넣습니다.
			table[missPointer] = pageStream[i];

		}
		else {	// 페이지 폴트가 아닐 경우 hitCounter을 1 증가시킵니다.
			printf("%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			hitCounter++;
		}

		// 페이지 교체 후 테이블의 상태를 출력하기 위해 저장하고 화면과 파일로 출력합니다.
		addTableInPrintBuffer(table, tableSize, OPTIMAL_NUM, NULL);
		printBufferFlush(tableSize);
	}

	// 메모리 해제
	free(metaInfoTable);
	free(table);

	// 수행 결과 출력
	printf("Optimal Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "Optimal Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* FIFO 알고리즘을 수행하는 함수 */
void FIFO(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;	// 변수 초기화

	struct Queue table;		// 선입선출을 위한 큐

	queueInit(&table, tableSize);

	printf("\n\nFIFO start\n");
	fprintf(fp, "\n\nFIFO start\n");

	for(int i = 0; i < streamLength; i++) {
		if(checkPageFault(table.queue, tableSize, pageStream[i])) {	// 페이지 폴트일 경우
			printf("%d. page %2d fault!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d fault!!!\n", i+1, pageStream[i].number);
			faultCounter++;		// faultCounter 증가

			// 테이블의 상태를 출력하기 위해 저장합니다.
			addTableInPrintBuffer(table.queue, tableSize, FIFO_NUM, NULL);
			// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");
			addStringInPrintBuffer(tableSize/2, "=>");
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");

			if(isQFull(&table))		// 큐가 다 찼다면 하나 빼줍니다.
				poll(&table);

			add(&table, pageStream[i]);	// 큐에 넣습니다.

		}
		else {		// 페이지 폴트가 아닐 경우
			printf("%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			hitCounter++;		// hitCount 증가
		}

		// 변경 후 테이블의 상태를 저장한 후 테이블 변화를 화면과 파일로 출력
		addTableInPrintBuffer(table.queue, tableSize, FIFO_NUM, NULL);
		printBufferFlush(tableSize);
	}

	// 수행 결과를 출력합니다.
	printf("FIFO Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "FIFO Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* LIFO 알고리즘을 수행하는 함수 */
void LIFO(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;

	struct Stack table;

	stackInit(&table, tableSize);

	printf("\n\nLIFO start\n");
	fprintf(fp, "\n\nLIFO start\n");

	for(int i = 0; i < streamLength; i++) {
		if(checkPageFault(table.stack, tableSize, pageStream[i])) {	// 페이지 폴트일 경우
			printf("%d. page %2d fault!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d fault!!\n", i+1, pageStream[i].number);
			faultCounter++;			// faultCounter 증가

			// 변경 전 테이블 상태 저장
			addTableInPrintBuffer(table.stack, tableSize, LIFO_NUM, NULL);
			// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");
			addStringInPrintBuffer(tableSize/2, "=>");
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");

			if(isSFull(&table))		// 테이블이 다 찾을 경우 하나 빼줍니다.
				pop(&table);

			push(&table, pageStream[i]);	// 페이지 교체

		}
		else {
			printf("%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			hitCounter++;			// hitCounter 증가
		}

		// 변경 후 테이블 상태 저장 후 테이블 변화를 화면과 파일로 출력
		addTableInPrintBuffer(table.stack, tableSize, LIFO_NUM, NULL);
		printBufferFlush(tableSize);
	}

	// 수행 결과 출력
	printf("LIFO Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "LIFO Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* LRU 알고리즘 수행하는 함수 */
void LRU(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;	// 변수 초기화

	// 참조 시간을 저장할 수 있는 struct Least 형으로 metaInfoTable을 만듭니다.
	page_t *table = (page_t *)malloc(sizeof(page_t)*tableSize);
	struct Least *metaInfoTable = (struct Least *)malloc(sizeof(struct Least)*tableSize);

	memset(table, 0, sizeof(table));
	memset(metaInfoTable, 0, sizeof(metaInfoTable));

	printf("\n\nLRU start\n");
	fprintf(fp, "\n\nLRU start\n");

	missPointer = 0;
	for(int i = 0; i < streamLength; i++) {
		if(checkPageFault(table, tableSize, pageStream[i])) {
			printf("%d. page %2d fault!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d fault!!\n", i+1, pageStream[i].number);
			faultCounter++;		//faultCounter 증가

			// 테이블 상태 저장
			addTableInPrintBuffer(table, tableSize, LRU_NUM, metaInfoTable);
			// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t\t");
			addStringInPrintBuffer(tableSize/2, "=>");
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");

			// 테이블이 찻을 경우 사용된 시간이 가장 오래된 것을 찾습니다.
			if(isTableFull(table, tableSize)) {
				missPointer = 0;

				for(int j = 1; j < tableSize; j++) {
					// 더 오래전에 사용됐다면 missPointer에 해당 위치를 저장
					if(metaInfoTable[missPointer].time > metaInfoTable[j].time)
						missPointer = j;
				}
			}

			// 이제 missPointer은 가장 오래 전에 사용된 페이지가 담긴 위치를 가리킵니다.

			// 페이지 교체 후 현재 시간을 참조시간으로 설정합니다.
			table[missPointer] = pageStream[i];
			metaInfoTable[missPointer].time = i;
		}
		else {		// 페이지 폴트가 아닐 경우
			// 현재 참조했으므로 참조 시간을 갱신합니다.
			metaInfoTable[hitPointer].time = i;
			printf("%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			hitCounter++;	// hitCounter 증가
		}

		// 변경 후 테이블 상태를 저장하고 테이블 변화를 출력합니다.
		addTableInPrintBuffer(table, tableSize, LRU_NUM, metaInfoTable);
		printBufferFlush(tableSize);
	}

	// 메모리 해제
	free(metaInfoTable);
	free(table);

	// 화면과 파일로 결과를 출력
	printf("LRU Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "LRU Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* LFU 알고리즘을 수행하는 함수 */
void LFU(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;	// 변수 초기화

	// 참조 시간과 참조 횟수를 저장하기 위해 struct Least 구조체로 테이블을 만듭니다.
	page_t *table = (page_t *)malloc(sizeof(page_t)*tableSize);
	struct Least *metaInfoTable = (struct Least *)malloc(sizeof(struct Least)*tableSize);

	memset(table, 0, sizeof(table));
	memset(metaInfoTable, 0, sizeof(metaInfoTable));

	printf("\n\nLFU start\n");
	fprintf(fp, "\n\nLFU start\n");

	missPointer = 0;
	for(int i = 0; i < streamLength; i++) {
		if(checkPageFault(table, tableSize, pageStream[i])) {	// 페이지 폴트일 경우
			printf("%d. page %2d fault!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d fault!!\n", i+1, pageStream[i].number);
			faultCounter++;		// faultCounter 증가

			// 변경 전 테이블 상태를 저장합니다.
			addTableInPrintBuffer(table, tableSize, LFU_NUM, metaInfoTable);
			// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t\t");
			addStringInPrintBuffer(tableSize/2, "=>");
			for(int j = 0; j < tableSize; j++)
				addStringInPrintBuffer(j, "\t");

			// 테이블이 찼을 경우 참조 횟수가 가장 작은 것을 찾고,
			// 같다면 더 오래전에 참조한 것을  찾습니다.
			if(isTableFull(table, tableSize)) {
				missPointer = 0;

				for(int j = 1; j < tableSize; j++) {
					if(metaInfoTable[missPointer].count > metaInfoTable[j].count)	// 참조 횟수가 더 적은 페이지가 담긴 위치를 missPointer에 저장합니다.
						missPointer = j;
					else if(metaInfoTable[missPointer].count == metaInfoTable[j].count && metaInfoTable[missPointer].time > metaInfoTable[j].time)	// 참조 횟수가 같다면 더 오래전에 사용된 페이지가 담긴 위치를 missPointer에 저장합니다.
						missPointer = j;
				}
			}

			// 페이지를 교체하고 현재 시간을 넣어주고 참조 횟수를 1로 넣어줍니다.
			table[missPointer] = pageStream[i];
			metaInfoTable[missPointer].time = i;
			metaInfoTable[missPointer].count = 1;

		}
		else {
			// 페이지 폴트가 아닐 경우 현재 참조 시간을 갱신하고 참조 횟수를 1 증가시킵니다.
			metaInfoTable[hitPointer].time = i;
			metaInfoTable[hitPointer].count++;
			printf("%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			hitCounter++;	// hitCounter 증가
		}

		// 현재 테이블 상태를 저장하고 파일과 화면에 테이블 변화를 출력합니다.
		addTableInPrintBuffer(table, tableSize, LFU_NUM, metaInfoTable);
		printBufferFlush(tableSize);
	}

	// 메모리 해제
	free(metaInfoTable);
	free(table);

	// 수행 결과 출력
	printf("LFU Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "LFU Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* SC 알고리즘 수행하는 함수 */
void SC(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;	// 변수 초기화

	// R 비트를 사용해야 하므로 struct Clock 구조체로 테이블을 만듭니다.
	struct Clock *table = (struct Clock *)malloc(sizeof(struct Clock)*tableSize);

	memset(table, 0, sizeof(table));

	printf("\n\nSC start\n");
	fprintf(fp, "\n\nSC start\n");

	missPointer = 0;
	for(int i = 0; i < streamLength; i++) {

		// 페이지 변화 전 상태 출력하기 전 저장합니다.
		addTableInPrintBuffer(table, tableSize, SC_NUM, NULL);
		// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
		for(int j = 0; j < tableSize; j++)
			addStringInPrintBuffer(j, "\t\t");
		addStringInPrintBuffer(tableSize/2, "=>");
		for(int j = 0; j < tableSize; j++)
			addStringInPrintBuffer(j, "\t");

		if(checkPageFault(table, tableSize, pageStream[i])) {	// 페이지 폴트일 경우
			printf("%d. page %2d fault!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d fault!!\n", i+1, pageStream[i].number);
			faultCounter++;		// faultCounter 증가

			if(isTableFull(table, tableSize)) {	// 페이지가 찼을 경우
				// R비트가 0인 것을 찾을 때까지 포인터를 움직입니다.
				while(table[missPointer].R) {
					table[missPointer].R = 0;	// R비트가 1이면 0으로 수정합니다.
					missPointer = (missPointer + 1) % tableSize;	// 모듈러 연산으로 포인터를 움직입니다.
				}
			}

			// 테이블 교체 하고 R비트를 1로 설정합니다.
			table[missPointer] = pageStream[i];
			table[missPointer].R = 1;
			missPointer = (missPointer+1)%tableSize;	// 모듈러 연산으로 포인터를 움직입니다.
		}
		else {
			// 페이지 폴트가 아닐 경우
			table[hitPointer].R = 1;	// R비트를 1로 갱신합니다.
			printf("%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			fprintf(fp, "%d. page %2d Hit!!!\n", i+1, pageStream[i].number);
			hitCounter++;	// hitCounter 증가
		}

		// 수행 후 테이블 상태를 저장한 후 테이블 변화를 출력합니다.
		addTableInPrintBuffer(table, tableSize, SC_NUM, NULL);
		printBufferFlush(tableSize);
	}

	free(table);

	printf("SC Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "SC Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* ESC 알고리즘에서 교체할 페이지를 찾아 해당 칸을 가리키는 포인터를 반환하는 함수 */
int search00Or01(page_t *table, int tableSize, int pointer) {
	int i;
	for(i = 0; i < tableSize; i++) {
		// 전체 테이블을 돌면서 RW 비트가 00인 것을 찾으면 바로 해당 위치를 반환합니다.
		if(!table[pointer].R && !table[pointer].W)
			return pointer;
		table[pointer].R = 0;
		pointer = (pointer+1)%tableSize;
	}

	while(1) {
		// 전체 테이블을 돌면서 RW 비트가 00인 것을 찾으면 바로 해당 위치를 반환합니다.
		if(!table[pointer].W)
			return pointer;
		table[pointer].W = 0;
		pointer = (pointer+1)%tableSize;
	}

	return -1;
}

/* ESC 알고리즘 수행하는 함수 */
void ESC(page_t *pageStream, int tableSize) {
	faultCounter = hitCounter = 0;	// 변수 초기화

	struct Clock *table = (struct Clock *)malloc(sizeof(struct Clock)*tableSize);

	memset(table, 0, sizeof(table));

	printf("\n\nESC start\n");
	fprintf(fp, "\n\nESC start\n");

	missPointer = 0;
	for(int i = 0; i < streamLength; i++) {

		// 변경 전 테이블 상태를 저장합니다.
		addTableInPrintBuffer(table, tableSize, ESC_NUM, NULL);
		// 보기 좋게 공백과 화살표를 출력하기 위해 저장합니다.
		for(int j = 0; j < tableSize; j++)
			addStringInPrintBuffer(j, "\t\t");
		addStringInPrintBuffer(tableSize/2, "=>");
		for(int j = 0; j < tableSize; j++)
			addStringInPrintBuffer(j, "\t");

		if(checkPageFault(table, tableSize, pageStream[i])) {	// 페이지 폴트일 경우
			printf("%d. page %2d(RW: %d%d) fault!!\n", i+1, pageStream[i].number, pageStream[i].R?1:0, pageStream[i].W?1:0);
			fprintf(fp, "%d. page %2d(RW: %d%d) fault!!\n", i+1, pageStream[i].number, pageStream[i].R?1:0, pageStream[i].W?1:0);
			faultCounter++;		// faultCounter 증가

			// 테이블이 찼는지 확인합니다.
			if(isTableFull(table, tableSize)) {
				// search00Or01() 함수를 호출하여 교체할 페이지의 위치를 missPointer에 저장합니다
				missPointer = search00Or01(table, tableSize, missPointer);
				if(missPointer < 0) {
					fprintf(stderr, "?????\n");
					exit(1);
				}
			}

			// 페이지 교체 후 모듈러 연산으로 포인터가 다음 칸을 가리키도록 합니다.
			table[missPointer] = pageStream[i];
			table[missPointer].R = 1;	// 페이지를 참조했으므로 R비트를 1로 바꿉니다.
			missPointer = (missPointer+1)%tableSize;

		}
		else {
			// 페이지 폴트가 아닐 경우
			table[hitPointer] = pageStream[i];	// R비트와 W비트를 읽은 페이지의 RW비트로 갱신합니다.
			table[hitPointer].R = 1;	// 페이지를 참조했으므로 R비트를 1로 바꿉니다.
			printf("%d. page %2d(RW: %d%d) Hit!!!\n", i+1, pageStream[i].number, pageStream[i].R?1:0, pageStream[i].W?1:0);
			fprintf(fp, "%d. page %2d(RW: %d%d) Hit!!!\n", i+1, pageStream[i].number, pageStream[i].R?1:0, pageStream[i].W?1:0);
			hitCounter++;		// hitCounter 증가
		}

		// 현재 페이지 상태를 저장하고 페이지 변화를 화면과 파일로 출력합니다.
		addTableInPrintBuffer(table, tableSize, ESC_NUM, NULL);
		printBufferFlush(tableSize);
	}

	free(table);	// 메모리 해제

	// 수행 결과를 화면과 파일에 출력합니다.
	printf("ESC Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
	fprintf(fp, "ESC Algorithm result\npage fault: %d번, hit: %d번\n", faultCounter, hitCounter);
}

/* 모든 알고리즘을 수행하는 함수 */
void ALL(page_t *pageStream, int tableSize) {
	Optimal(pageStream, tableSize);
	FIFO(pageStream, tableSize);
	LIFO(pageStream, tableSize);
	LRU(pageStream, tableSize);
	LFU(pageStream, tableSize);
	SC(pageStream, tableSize);
	ESC(pageStream, tableSize);
}

/* 현제 테이블의 상태를 출력하기 위해 저장하는 함수 */
// 보기 좋게 테이블의 변화를 한번에 출력하기 위해 교체 전 테이블 상태와 교체 후 테이블 상태를
// 다 저장한 후에 한번에 출력합니다.
void addTableInPrintBuffer(page_t *table, int tableSize, int algorithm, struct Least *data) {
	int i, offset = 0;

	// 어떤 알고리즘을 수행하는지에 따라 출력 형식을 다르게 만들었습니다.

	if(algorithm == OPTIMAL_NUM || algorithm == FIFO_NUM || algorithm == LIFO_NUM) {
		for(i = 0; i < tableSize; i++) {
			offset = 0;
			while(printBuffer[i][offset])	offset++;
			sprintf(&printBuffer[i][offset], "|%2d|", table[i].number);
		}
	}
	else if(algorithm == LRU_NUM) {		// LRU 알고리즘의 경우 언제 참조했는지도 출력합니다.
		for(i = 0; i < tableSize; i++) {
			offset = 0;
			while(printBuffer[i][offset])	offset++;
			sprintf(&printBuffer[i][offset], "|%2d, %d번 스트림에서 참조함|", table[i].number, data[i].time+1);
		}
	}
	else if(algorithm == LFU_NUM) {		// LFU 알고리즘의 경우 몇 번 참조했는지도 출력합니다.
		for(i = 0; i < tableSize; i++) {
			offset = 0;
			while(printBuffer[i][offset])	offset++;
			sprintf(&printBuffer[i][offset], "|%2d, %d번 참조함, %d에서 참조|", table[i].number, data[i].count, data[i].time);
		}
	}
	else if(algorithm == SC_NUM) {		// SC 알고리즘의 경우 현재 포인터가 가리키는 칸과 참조 비트를 각각 화살표와 '*'로 출력합니다.
		for(i = 0; i < tableSize; i++) {
			offset = 0;
			while(printBuffer[i][offset])	offset++;
			sprintf(&printBuffer[i][offset], "%s|%2d%c|", missPointer==i?"->":"  ", table[i].number, table[i].R?'*':' ');
		}
	}
	else if(algorithm == ESC_NUM) {		// ESC 알겨리즘의 경우 현재 포인터가 가리키는 칸과 RW 비트의 값도 같이 출력합니다.
		for(i = 0; i < tableSize; i++) {
			offset = 0;
			while(printBuffer[i][offset])	offset++;
			sprintf(&printBuffer[i][offset], "%s|%2d, RW:%c%c|", missPointer==i?"->":"  ", table[i].number, table[i].R?'1':'0', table[i].W?'1':'0');
		}
	}
}

/* 인자로 들어온 문자열을 인자로 들어온 인덱스 번째 printBuffer에 추가로 저장하는 함수 */
void addStringInPrintBuffer(int index, char *string) {
	int offset = 0;
	while(printBuffer[index][offset])	offset++;	// 널문자를 찾아 해당 인덱스를 offset에 저장
	sprintf(&printBuffer[index][offset], "%s", string);	// 해당 offset부터 문자열을 씁니다.
}

/* 출력을 위해 저장한 printBuffer변수에 저장된 내용을 화면과 파일로 출력합니다. */
void printBufferFlush(int tableSize) {
	for(int j = 0; j < tableSize; j++) {
		printf("%s\n", printBuffer[j]);
		fprintf(fp, "%s\n", printBuffer[j]);
		printBuffer[j][0] = 0;
	}
}
