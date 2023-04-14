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
// flash memory�� ó�� ����� �� �ʿ��� �ʱ�ȭ �۾�, ���� ��� address mapping table�� ����
// �ʱ�ȭ ���� �۾��� �����Ѵ�
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
	// �߰������� �ʿ��� �۾��� ������ �����ϸ� �ǰ� ������ ���ص� ������
	//

	return;
}

//
// file system�� ���� FTL�� �����ϴ� write interface
// 'sectorbuf'�� ����Ű�� �޸��� ũ��� 'SECTOR_SIZE'�̸�, ȣ���ϴ� �ʿ��� �̸� �޸𸮸� �Ҵ�޾ƾ� ��
//
void ftl_write(int lsn, char *sectorbuf)
{
#ifdef PRINT_FOR_DEBUG			// �ʿ� �� ������ block mapping table�� ����� �� �� ����
	print_addrmaptbl();
#endif

	//
	// block mapping ������� overwrite�� �߻��ϸ� �̸� �ذ��ϱ� ���� �ݵ�� �ϳ��� empty block��
	// �ʿ��ϸ�, �ʱⰪ�� flash memory���� �� ������ block number�� �����
	// overwrite�� �ذ��ϰ� �� �� �翬�� reserved_empty_blk�� overwrite�� ���߽�Ų (invalid) block�� �Ǿ�� ��
	// ���� reserved_empty_blk�� �����Ǿ� �ִ� ���� �ƴ϶� ��Ȳ�� ���� ��� �ٲ� �� ����
	//
	static int reserved_empty_blk = DATABLKS_PER_DEVICE;
	int lbn = lsn / SECTORS_PER_BLOCK;
	int offset = lsn % SECTORS_PER_BLOCK;
	char readbuf[SECTOR_SIZE];
	char pagebuf[PAGE_SIZE];
	SpareData sparebuf;
	int i, j, k, check_space = 1;

	// ������ġ���� lsn�� �ش��ϴ� �����͸� �о readbuf�� �����մϴ�.
	// readbuf�� ���� 1�� ä�����ִٸ� ������ġ�� lsn�� �ش��ϴ� ���� ����ֽ��ϴ�.
	ftl_read(lsn, readbuf);
	for(i = 0; i < SECTOR_SIZE; i++)
	{
		if(readbuf[i] != (char)0xFF) {
			check_space = 0;
			break;
		}
	}

	// ������ġ���� lsn�� �ش��ϴ� sector�� ����ִ� ���
	if(check_space == 1) {

		// lsn�� �ش��ϴ� lbn�� mapping�Ǵ� pbn�� �Ҵ�Ǿ����� �ʴ� ���
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

			// ���̺� lbn�� mapping�Ǵ� pbn�� �Ҵ����ݴϴ�.
			addrmaptbl.pbn[lbn] = i;
		}

		memset(&sparebuf, 0, SPARE_SIZE);
		sparebuf.lsn = lsn;

		strncpy(pagebuf, sectorbuf, SECTOR_SIZE);
		memcpy(&pagebuf[SECTOR_SIZE], &sparebuf, SPARE_SIZE);
		dd_write(addrmaptbl.pbn[lbn]*PAGES_PER_BLOCK + offset, pagebuf);

	}

	// �̹� lsn�� �ش��ϴ� psn�� �Ҵ�Ǿ� �ִ� ��� overwrite�̹Ƿ�
	// ó��������Ѵ�.
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
// file system�� ���� FTL�� �����ϴ� read interface
// 'sectorbuf'�� ����Ű�� �޸��� ũ��� 'SECTOR_SIZE'�̸�, ȣ���ϴ� �ʿ��� �̸� �޸𸮸� �Ҵ�޾ƾ� ��
// 
void ftl_read(int lsn, char *sectorbuf)
{
#ifdef PRINT_FOR_DEBUG			// �ʿ� �� ������ block mapping table�� ����� �� �� ����
	print_addrmaptbl();
#endif

	int i;
	int lbn = lsn / SECTORS_PER_BLOCK;
	int offset = lsn % SECTORS_PER_BLOCK;
	int ppn = addrmaptbl.pbn[lbn]*PAGES_PER_BLOCK;
	char pagebuf[PAGE_SIZE];
	SpareData sparebuf;

	// ���̺� lbn�� mapping�Ǵ� ���� ���� ���
	// -1�̸� ���� ���̰�, ���ڷ� ���� sectorbuf�� ���� 1�� ä��ϴ�.
	if(addrmaptbl.pbn[lbn] < 0) {
		memset(sectorbuf, 0xFF, SECTOR_SIZE);
	}

	// ���̺� lbn�� mapping�� ���� �ִ� ���
	else {
		// pbn���� offset�� �ش�Ǵ� �������� �о�ɴϴ�.
		// pbn�� �о �����ϸ� ���� sectorbuf�� ���� 1�� ä��ϴ�.
		if(dd_read(ppn + offset, pagebuf) < 0) {
			fprintf(stderr, "dd_read error\n");
			exit(1);
		}

		//�б� ���������� ���� ������ sectorbuf�� �����մϴ�.
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
