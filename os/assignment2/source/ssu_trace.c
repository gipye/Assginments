#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf(2, "usage: ssu_trace [mask] [command]\n");
		exit();
	}

	trace(atoi(argv[1]));		// 마스크 값 설정

	exec(argv[2], &argv[2]);	// 프로세스 실행

	printf(2, "exec() error\n");
	exit();
}
