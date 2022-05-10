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
	// 아래 변수는 테스트할 때 필요하면 사용하기 바람
	char sectorbuf[SECTOR_SIZE], *blockbuf;
	int i;
    char arr[20] = "aertttt";
    memcpy(sectorbuf,arr,20);

	// flash memory의 파일명은 'flashmemory'을 가정함
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
	// ftl_write() 및 ftl_read() 테스트를 위한 코드를 자유자재로 만드세요.
	// 채점할 때 이 부분에 테스트 코드를 심어서 합니다. Flash memory에 대한 데이터의 읽기 및 쓰기가
	// 올바르게 동작하는지를 테스트하고, 필요하면 다른 부분도 검사를 합니다.
	//

	return 0;
}