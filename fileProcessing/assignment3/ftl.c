#define PRINT_FOR_DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include "blkmap.h"

AddrMapTbl addrmaptbl;
extern FILE *devicefp;

/****************  prototypes ****************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void print_block(int pbn);
void print_addrmaptbl();

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다
//
void ftl_open()
{
	int i;

	// initialize the address mapping table
	for(i = 0; i < DATABLKS_PER_DEVICE; i++)
	{
		addrmaptbl.pbn[i] = -1;
	}

	//
	// 추가적으로 필요한 작업이 있으면 수행하면 되고 없으면 안해도 무방함
	//

	return;
}

//
// file system을 위한 FTL이 제공하는 write interface
// 'sectorbuf'가 가리키는 메모리의 크기는 'SECTOR_SIZE'이며, 호출하는 쪽에서 미리 메모리를 할당받아야 함
//
void ftl_write(int lsn, char *sectorbuf)
{
#ifdef PRINT_FOR_DEBUG			// 필요 시 현재의 block mapping table을 출력해 볼 수 있음
	print_addrmaptbl();
#endif

	//
	// block mapping 기법에서 overwrite가 발생하면 이를 해결하기 위해 반드시 하나의 empty block이
	// 필요하며, 초기값은 flash memory에서 맨 마지막 block number를 사용함
	// overwrite를 해결하고 난 후 당연히 reserved_empty_blk는 overwrite를 유발시킨 (invalid) block이 되어야 함
	// 따라서 reserved_empty_blk는 고정되어 있는 것이 아니라 상황에 따라 계속 바뀔 수 있음
	//
	static int reserved_empty_blk = DATABLKS_PER_DEVICE;
	int lbn = lsn / SECTORS_PER_BLOCK;
	int offset = lsn % SECTORS_PER_BLOCK;
	char readbuf[SECTOR_SIZE];
	char pagebuf[PAGE_SIZE];
	SpareData sparebuf;
	int i, j, k, check_space = 1;

	// 저장장치에서 lsn에 해당하는 데이터를 읽어서 readbuf에 저장합니다.
	// readbuf가 전부 1로 채워져있다면 저장장치에 lsn에 해당하는 곳이 비어있습니다.
	ftl_read(lsn, readbuf);
	for(i = 0; i < SECTOR_SIZE; i++)
	{
		if(readbuf[i] != (char)0xFF) {
			check_space = 0;
			break;
		}
	}

	// 저장장치에서 lsn에 해당하는 sector가 비어있는 경우
	if(check_space == 1) {

		// lsn이 해당하는 lbn에 mapping되는 pbn이 할당되어있지 않는 경우
		if(addrmaptbl.pbn[lbn] == -1) {
			fseek(devicefp, 0, SEEK_SET);
			for(i = 0; i < DATABLKS_PER_DEVICE; i++) {
				if(i == reserved_empty_blk)
					continue;
				check_space = 1;
				for(j = 0; j < PAGES_PER_BLOCK; j++) {
					dd_read(i*PAGES_PER_BLOCK + j, pagebuf);

					for(k = 0; k < SECTOR_SIZE; k++)
					{
						if(pagebuf[j] != (char)0xFF) {
							check_space = 0;
							break;
						}
					}
					if(check_space == 0)
						break;
				}
				if(check_space == 1)
					break;
			}

			// 테이블에 lbn에 mapping되는 pbn을 할당해줍니다.
			addrmaptbl.pbn[lbn] = i;
		}

		memset(&sparebuf, 0, SPARE_SIZE);
		sparebuf.lsn = lsn;

		strncpy(pagebuf, sectorbuf, SECTOR_SIZE);
		memcpy(&pagebuf[SECTOR_SIZE], &sparebuf, SPARE_SIZE);
		dd_write(addrmaptbl.pbn[lbn]*PAGES_PER_BLOCK + offset, pagebuf);

	}

	// 이미 lsn에 해당하는 psn이 할당되어 있는 경우 overwrite이므로
	// 처리해줘야한다.
	else {
		for(i = 0; i < PAGES_PER_BLOCK; i++) {
			if(i == offset) {
				memset(&sparebuf, 0, SPARE_SIZE);
				sparebuf.lsn = lsn;

				strncpy(pagebuf, sectorbuf, SECTOR_SIZE);
				memcpy(&pagebuf[SECTOR_SIZE], &sparebuf, SPARE_SIZE);
				dd_write(reserved_empty_blk*PAGES_PER_BLOCK + offset, pagebuf);

			}
			else {
				dd_read(addrmaptbl.pbn[lbn]*PAGES_PER_BLOCK + i, pagebuf);
				dd_write(reserved_empty_blk*PAGES_PER_BLOCK + i, pagebuf);
			}
		}

		int temp = addrmaptbl.pbn[lbn];
		addrmaptbl.pbn[lbn] = reserved_empty_blk;
		reserved_empty_blk = temp;

		dd_erase(reserved_empty_blk);
	}

	return;
}

//
// file system을 위한 FTL이 제공하는 read interface
// 'sectorbuf'가 가리키는 메모리의 크기는 'SECTOR_SIZE'이며, 호출하는 쪽에서 미리 메모리를 할당받아야 함
// 
void ftl_read(int lsn, char *sectorbuf)
{
#ifdef PRINT_FOR_DEBUG			// 필요 시 현재의 block mapping table을 출력해 볼 수 있음
	print_addrmaptbl();
#endif

	int i;
	int lbn = lsn / SECTORS_PER_BLOCK;
	int offset = lsn % SECTORS_PER_BLOCK;
	int ppn = addrmaptbl.pbn[lbn]*PAGES_PER_BLOCK;
	char pagebuf[PAGE_SIZE];
	SpareData sparebuf;

	// 테이블에 lbn에 mapping되는 값이 없는 경우
	// -1이면 없는 것이고, 인자로 받은 sectorbuf를 전부 1로 채웁니다.
	if(addrmaptbl.pbn[lbn] < 0) {
		memset(sectorbuf, 0xFF, SECTOR_SIZE);
	}

	// 테이블에 lbn에 mapping는 값이 있는 경우
	else {
		// pbn에서 offset에 해당되는 페이지를 읽어옵니다.
		// pbn을 읽어서 실패하면 전부 sectorbuf를 전부 1로 채웁니다.
		if(dd_read(ppn + offset, pagebuf) < 0) {
			fprintf(stderr, "dd_read error\n");
			exit(1);
		}

		//읽기 성공했으면 읽은 내용을 sectorbuf에 저장합니다.
		else {
			memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
			memcpy(&sparebuf, &pagebuf[SECTOR_SIZE], SPARE_SIZE);
			//	print_block(addrmaptbl.pbn[lbn]);

		}

	}


	return;
}

//
// for debugging
//
void print_block(int pbn)
{
	char *pagebuf;
	SpareData *sdata;
	int i;

	pagebuf = (char *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	printf("Physical Block Number: %d\n", pbn);

	for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
	{
		dd_read(i, pagebuf);
		memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);
		printf("\t   %5d-[%7d]\n", i, sdata->lsn);
	}

	free(pagebuf);
	free(sdata);

	return;
}

//
// for debugging
//
void print_addrmaptbl()
{
	int i;

	printf("Address Mapping Table: \n");
	for(i = 0; i < DATABLKS_PER_DEVICE; i++)
	{
		if(addrmaptbl.pbn[i] >= 0)
		{
			printf("[%d %d]\n", i, addrmaptbl.pbn[i]);
		}
	}
}
