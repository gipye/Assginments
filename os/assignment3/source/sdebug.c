#include "types.h"
#include "stat.h"
#include "user.h"

#define PNUM 5
#define PRINT_CYCLE		10000000
#define TOTAL_COUNTER	500000000

void sdebug_func(void)
{
	int n, pid;

	printf(1, "start sdebug command\n");

	for(n = 0; n < PNUM; n++)	// PNUM 개의 프로세스 생성
	{
		pid = fork();
		if(pid < 0)		// 에러처리
			break;
		if(pid == 0)
		{
			long long print_counter = PRINT_CYCLE, counter = 0;
			int start_ticks = uptime(), end_ticks;

			if(weightset(n+1) < 0)	// 프로세스의 weight 값을 1 부터 PNUM 까지 생성된 순서대로 부여합니다.
			{
				printf(1, "weightset error\n");
				exit();
			}

			int first = 1;

			while (counter <= TOTAL_COUNTER)	// counter 값을 증가시키며 TOTAL_COUNTER이 되면 프로세스를 종료하기 위해 while문을 빠져나옵니다.
			{
				print_counter--;	// PRINT_CYCLE 만큼 지나면 프로세스 정보를 출력하기 위해 값을 감소시킵니다.
				counter++;		// TOTAL_COUNTER 만큼 지나면 프로세스를 종료하기 위해 증가시킵니다.

				if(print_counter == 0)
				{
					if(first)	// 한번도 출력하지 않았다면 출력합니다.
					{
						end_ticks = uptime();
						printf(1, "PID: %d, WEIGHT: %d, ", getpid(), n+1);
						printf(1, "TIMES: %d ms\n", (end_ticks - start_ticks)*10);
						first = 0;
					}
					print_counter = PRINT_CYCLE;
				}
			}

			printf(1, "PID: %d terminated\n", getpid());
			exit();
		}
	}

	// PNUM개의 프로세스가 끝나는 것을 기다리기 위해 PNUM번의 wait()을 호출하는데 PNUM번 호출되기 전에 wait()의 반환값이 -1이면 에러처리합니다.
	for(; n > 0; n--) {
		if(wait() < 0) {
			printf(1, "wait stopped early\n");
			exit();
		}
	}

	// PNUM번의 wait()을 호출 후에도 wait()이 -1을 리턴하지 않는다면 에러처리합니다.
	if(wait() != -1) {
		printf(1, "wait got too many\n");
		exit();
	}

	printf(1, "end of sdebug command\n");
}

int main(void)
{
	sdebug_func();
	exit();
}
