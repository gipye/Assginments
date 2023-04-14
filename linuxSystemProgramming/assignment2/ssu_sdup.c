#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>


int main() {
	char input[1024] = {0, };
	char c;
	int i, input_len = 0, err = 1;
	char word[6] = {0, };


	while(1) {

		//변수 초기화
		err = 0;
		for(i = 0; i < input_len; i++)
			input[i] = '\0';
		for(i = 0; i < 6; i++)
			word[i] = '\0';

		printf("20182610> ");	//프롬프트 출력
		scanf("%[ \t]", input);	//입력 전 공백 없애기
		scanf("%[^\n]", input);	//입력 받기
		c = getchar();			//개행 문자를 없애줍니다.

		//아무것도 입력받지 않을 경우
		if(!strcmp(input, ""))
			continue;

		//인덱스 i를 내장 명령어 부분 바로 다음을 가리키게 합니다.
		i = 0;
		while(input[i] != ' ' && input[i] != '\t' && input[i] != '\0')
			i++;
		//가장 긴 명령어인 fsha1보다 길면 help 명령어를 실행합니다.
		if(i > 5)
			strcpy(word, "help");
		//아니라면 내장 명령어 부분만 뽑아냅니다.
		else {
			for(int j = 0; j < i; j++)
				word[j] = input[j];
			word[i] = '\0';
		}

		//fmd5나 fsha1 명령이일 경우
		if(!strcmp(word, "fmd5") || !strcmp(word, "fsha1")) {
			//printf("Enter fmd5, fsha1\n");
			int j = 0;
			char temp[1024];
			char file_extension[1024], target_directory[1024] = {0, };
			long long int min_size, max_size;

			//인덱스 i를 [FILE EXTENSIOIN]을 가리키도록 합니다.
			while(input[i] == ' ' || input[i] == '\t')
				i++;
			//파일 확장자 부분을 뽑아냅니다.
			j = 0;
			while(input[i] != ' ' && input[i] != '\t' && input[i] != '\0') {
				if(j >= 1024) {
					fprintf(stderr, "Error: Wrong [FILE_EXTENSION]\n");
					err = 1;
					break;
				}

				file_extension[j] = input[i];
				j++;
				i++;
			}
			if(file_extension[0] != '*' && file_extension[1] != '.') {
				fprintf(stderr, "Error: wrong [FILE_EXTENSION]\n");
				continue;
			}
			//잘못된 입력일 경우
			if(err == 1)
				continue;
			//아직 두번째 인자까지만 받았는데 입력이 끝나면 에러
			else if(input[i] == '\0') {
				fprintf(stderr, "fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
				continue;
			}
			//문자열로 만들기 위해 마지막에 널문자 삽입하면 [FILE_EXTENSION] 완성
			file_extension[j] = '\0';



			//인덱스 i를 [MINSIZE]를 가리키도록 합니다.
			while(input[i] == ' ' || input[i] == '\t')
				i++;
			//MINSIZE 부분을 뽑아냅니다.
			j = 0;
			while(input[i] != ' ' && input[i] != '\t' && input[i] != '\0') {
				if(j >= 1024) {
					fprintf(stderr, "Error: Wrong [MINSIZE]\n");
					err = 1;
					break;
				}
				//정수로 변환하기 전에 문자열로 저장합니다.
				temp[j] = input[i];
				j++;
				i++;
			}
			//잘못된 입력일 경우
			if(err == 1)
				continue;
			//아직 세 번째 인자까지만 받았는데 입력이 끝나면 에러)
			else if(input[i] == '\0') {
				fprintf(stderr, "fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
				continue;
			}
			//temp를 문자열로 만듭니다.
			temp[j] = '\0';
			//이제 [MINSIZE]에 정수로 값을 넣어줍니다.
			if(!strcmp(temp, "~") || !strcmp(temp, "0"))
				min_size = 0;
			else {

				//'.'이 두 개 이상인 경우 에러처리
				for(j = 0; (temp[j] >= '0' && temp[j] <= '9') || temp[j] == '.'; j++) {
					if(temp[j] == '.')
						err++;
				}
				if(err > 1) {
					fprintf(stderr, "Error: wrong [MINSIZE]\n");
					continue;
				}
				err = 0;

				//단위가 입력 없거나 바이트인 경우
				if(!strcmp(&temp[j], "b") || !strcmp(&temp[j], "B") || !strcmp(&temp[j], "")) {
					temp[j] = '\0';
					if((min_size = atol(temp)) < 0) {
						fprintf(stderr, "Error: wrong [MINSIZE]\n");
						continue;
					}
				}
				//단위가 kb인 경우
				else if(!strcmp(&temp[j], "kb") || !strcmp(&temp[j], "KB")) {
					temp[j] = '\0';
					if((min_size = (long long int)(atof(temp) * 1000.0)) < 0) {
						fprintf(stderr, "Error: wrong [MINSIZE]\n");
						continue;
					}
				}
				//단위가 mb인 경우
				else if(!strcmp(&temp[j], "mb") || !strcmp(&temp[j], "MB")) {
					temp[j] = '\0';
					if((min_size = (long long int)(atof(temp) * 1000000.0)) < 0) {
						fprintf(stderr, "Error: wrong [MINSIZE]\n");
						continue;
					}
				}
				//단위가 gb인 경우
				else if(!strcmp(&temp[j], "gb") || !strcmp(&temp[j], "GB")) {
					temp[j] = '\0';
					if((min_size = (long long int)(atof(temp) * 1000000000.0)) < 0) {
						fprintf(stderr, "Error: wrong [MINSIZE]\n");
						continue;
					}
				}
				//잘못된 단위일 경우
				else {
					fprintf(stderr, "Error: wrong [MINSIZE]\n");
					continue;
				}
			}



			//인덱스 i를 [MAXSIZE]를 가리키도록 합니다.
			while(input[i] == ' ' || input[i] == '\t')
				i++;
			//MAXSIZE 부분을 뽑아냅니다.
			j = 0;
			while(input[i] != ' ' && input[i] != '\t' && input[i] != '\0') {
				if(j >= 1024) {
					fprintf(stderr, "Error: Wrong [MAXSIZE]\n");
					err = 1;
					break;
				}
				//정수로 변환하기 전에 문자열로 저장합니다.
				temp[j] = input[i];
				j++;
				i++;
			}
			//잘못된 입력일 경우 (아직 네 번째 인자까지만 받았는데 입력이 끝나면 에러)
			if(err == 1)
				continue;
			else if(input[i] == '\0') {
				fprintf(stderr, "fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
				continue;
			}
			//temp를 문자열로 만듭니다.
			temp[j] = '\0';
			//이제 [MAXSIZE]에 정수로 값을 넣어줍니다.
			if(!strcmp(temp, "~"))
				max_size = 9223372036854775807;
			else if(!strcmp(temp, "0"))
				max_size = 0;
			else {
				for(j = 0; (temp[j] >= '0' && temp[j] <= '9') || temp[j] == '.'; j++) {
					if(temp[j] == '.')
						err++;
				}
				if(err > 1) {
					fprintf(stderr, "Error: wrong [MAXSIZE]\n");
					continue;
				}
				err = 0;
				//단위가 입력 없거나 바이트인 경우
				if(!strcmp(&temp[j], "b") || !strcmp(&temp[j], "B") || !strcmp(&temp[j], "")) {
					temp[j] = '\0';
					if((max_size = atol(temp)) < 0) {
						fprintf(stderr, "Error: wrong [MAXSIZE]\n");
						continue;
					}
				}
				//단위가 kb인 경우
				else if(!strcmp(&temp[j], "kb") || !strcmp(&temp[j], "KB")) {
					temp[j] = '\0';
					if((max_size = (long long int)(atof(temp) * 1000)) < 0) {
						fprintf(stderr, "Error: wrong [MAXSIZE]\n");
						continue;
					}
				}
				//단위가 mb인 경우
				else if(!strcmp(&temp[j], "mb") || !strcmp(&temp[j], "MB")) {
					temp[j] = '\0';
					if((max_size = (long long int)(atof(temp) * 1000000)) < 0) {
						fprintf(stderr, "Error: wrong [MAXSIZE]\n");
						continue;
					}
				}
				//단위가 gb인 경우
				else if(!strcmp(&temp[j], "gb") || !strcmp(&temp[j], "GB")) {
					temp[j] = '\0';
					if((max_size = (long long int)(atof(temp) * 1000000000)) < 0) {
						fprintf(stderr, "Error: wrong [MAXSIZE]\n");
						continue;
					}
				}
				//잘못된 단위일 경우
				else {
					fprintf(stderr, "Error: wrong [MAXSIZE]\n");
					continue;
				}
			}


			//인덱스 i를 [TARGET_DIRECTORY]를 가리키도록 합니다.
			while(input[i] == ' ' || input[i] == '\t')
				i++;
			//[TARGET_DIRECTORY] 부분을 뽑아냅니다.
			j = 0;
			while(input[i] != ' ' && input[i] != '\t' && input[i] != '\0') {
				if(j >= 1024) {
					fprintf(stderr, "Error: Wrong [TARGET_DIRECTORY]\n");
					err = 1;
					break;
				}
				target_directory[j] = input[i];
				j++;
				i++;
			}
			//잘못된 입력일 경우
			if(err == 1)
				continue;

			//문자열로 만들기 위해 끝에 널문자를 삽입합니다.
			target_directory[j] = '\0';
			//5개의 인자를 다 받았는데도 인자가 더 있으면 에러처리 합니다.
			while(input[i] == ' ' || input[i] == '\t')
				i++;
			if(input[i] != '\0') {
				fprintf(stderr, "fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
				continue;
			}


			pid_t pid;
			if((pid = fork()) < 0) {
				printf("fork error\n");
				continue;
			}
			//자식 프로세스 생성
			if(pid == 0) {
				char MINSIZE[20], MAXSIZE[20];
				sprintf(MINSIZE, "%lld", min_size);
				sprintf(MAXSIZE, "%lld", max_size);
				if(!strcmp(word, "fmd5")) {
					execl("./ssu_find-md5", "./ssu_find-md5", file_extension, MINSIZE, MAXSIZE, target_directory, NULL);
					exit(0);	//execl()이 실패했을 경우를 대비한 프로세스 종료
				}
				else {
					execl("./ssu_find-sha1", "./ssu_find-sha1", file_extension, MINSIZE, MAXSIZE, target_directory, NULL);
					exit(0);	//execl()이 실패했을 경우를 대비한 프로세스 종료
				}
			}
			else {
				wait(NULL);
				continue;
			}
		//fmd5/fsha1 명령어인 경우 끝
		}


		else if(!strcmp(word, "exit")){
			printf("Prompt End\n");
			exit(0);
		}


		//help 명령어나 잘못된 명령어일 경우
		else{
			//자식 프로세스 생성
			pid_t pid;
			if((pid = fork()) < 0) {
				printf("fork error\n");
				exit(1);
			}
			if(pid == 0) {
				//ssu_help 실행파일을 실행
				if(execl("./ssu_help", "./ssu_help", NULL) < 0) {
					fprintf(stderr, "execl error\n");
					exit(1);
				}
				exit(0);	//execl()이 실패했을 경우를 대비한 프로세스 종료
			}
			else {	//wait()로 ssu_help가 끝나고 continue 해준다.
				wait(0);
				continue;
			}

		}

	}



}
