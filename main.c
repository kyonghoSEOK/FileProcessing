#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "blkmap.h"

FILE *devicefp;

void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
// void ftl_print();
void print_addrmaptbl();

int main( )
{
	// �Ʒ� ������ �׽�Ʈ�� �� �ʿ��ϸ� ����ϱ� �ٶ�
	char sectorbuf[SECTOR_SIZE], *blockbuf;
	int i;
    char arr[20] = "aertttt";
    memcpy(sectorbuf,arr,20);

	// flash memory�� ���ϸ��� 'flashmemory'�� ������
	devicefp = fopen("flashmemory", "w+b");

	if(devicefp == NULL)
	{
		printf("file open error\n");
		exit(1);
	}

	//
	// initialize flash memory where each byte are set to '0xFF'
	// 
	blockbuf = (char *)malloc(BLOCK_SIZE);
	memset(blockbuf, 0xFF, BLOCK_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		fwrite(blockbuf, BLOCK_SIZE, 1, devicefp);
	}

	ftl_open();

	ftl_write(505,sectorbuf);
	ftl_read(505,sectorbuf);
	printf("%s\n",sectorbuf);
	strcpy(sectorbuf,"abc");

	ftl_write(505,sectorbuf);
	// strcpy(sectorbuf,"abaac");

	// ftl_write(8,sectorbuf);
	// strcpy(sectorbuf,"aadffffbc");

	// ftl_write(10,sectorbuf);

	// ftl_write(10,sectorbuf);

	ftl_read(505,sectorbuf);
	printf("%s\n",sectorbuf);

	// ftl_write(5,sectorbuf);

	// ftl_read(5,sectorbuf);

	// ftl_print();

	//
	// ftl_write() �� ftl_read() �׽�Ʈ�� ���� �ڵ带 ��������� ���弼��.
	// ä���� �� �� �κп� �׽�Ʈ �ڵ带 �ɾ �մϴ�. Flash memory�� ���� �������� �б� �� ���Ⱑ
	// �ùٸ��� �����ϴ����� �׽�Ʈ�ϰ�, �ʿ��ϸ� �ٸ� �κе� �˻縦 �մϴ�.
	//

	return 0;
}