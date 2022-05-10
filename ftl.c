#define PRINT_FOR_DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include "blkmap.h"
#include <stdbool.h>

AddrMapTbl addrmaptbl;
SpareData sparedata;
extern FILE *devicefp;
extern int dd_read(int, char *);
extern int dd_write(int, char *);
extern int dd_erase(int);
bool pbn_check[BLOCKS_PER_DEVICE] = { 0 };
int reserved_empty_blk = DATABLKS_PER_DEVICE;


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
	char page_buf[PAGE_SIZE];
	int lbn;

	// initialize the address mapping table
	for(i = 0; i < DATABLKS_PER_DEVICE; i++)
	{
		addrmaptbl.pbn[i] = -1;
	}

	//
	// 추가적으로 필요한 작업이 있으면 수행하면 되고 없으면 안해도 무방함
	//

	for(i = 0; i< BLOCKS_PER_DEVICE; i++){
		if(dd_read(i*PAGES_PER_BLOCK, page_buf) > 0){
			memcpy(&lbn,&page_buf[SECTOR_SIZE+4],4);
			if(lbn >= 0){
				addrmaptbl.pbn[lbn] = i;
				pbn_check[i]= TRUE;
			}
		}
	}
	// for(int i = 0; i <BLOCKS_PER_DEVICE; i++){
	// 	if(pbn_check[i] == FALSE){
	// 		reserved_empty_blk = i;
	// 		// addrmaptbl.pbn[DATABLKS_PER_DEVICE] = i;
	// 		// pbn_check[reserved_empty_blk]= TRUE;
	// 		break;
	// 	}
	// }
	
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
	int ppn, pbn, check;
	

	int lbn = lsn / PAGES_PER_BLOCK;
	int offset = lsn % PAGES_PER_BLOCK;

	char page_buf[PAGE_SIZE]; 
	memset(page_buf,0xff,PAGE_SIZE);
	sparedata.lsn = lsn;
	// memcpy(&sparedata.dummy,&lbn,12);

	if(addrmaptbl.pbn[lbn] >= 0){
		pbn = addrmaptbl.pbn[lbn];
		ppn = pbn * PAGES_PER_BLOCK + offset;
		if(dd_read(ppn,page_buf)> 0){
			memcpy(&check,&page_buf[SECTOR_SIZE],4);
			if(check >= 0){
				for(int i = 0; i< PAGES_PER_BLOCK ; i ++){
					if(i==offset)
						continue;
					if(dd_read(pbn*PAGES_PER_BLOCK +i, page_buf) < 0){
						fprintf(stderr,"read_error\n");
						exit(1);
					}
					else{
						if(dd_write(reserved_empty_blk*PAGES_PER_BLOCK+i,page_buf) < 0 ){
							fprintf(stderr,"write_error\n");
							exit(1);
						}
					}
				}

				memcpy(page_buf,sectorbuf,SECTOR_SIZE);
				memcpy(&page_buf[SECTOR_SIZE],&sparedata,16);
				if(dd_write(reserved_empty_blk*PAGES_PER_BLOCK + offset, page_buf) < 0){
					fprintf(stderr,"write_error\n");
					exit(1);
				}

				if(dd_erase(pbn) < 0){
					fprintf(stderr,"erase\n");
					exit(1);
				}

				addrmaptbl.pbn[lbn] = reserved_empty_blk;
				reserved_empty_blk = pbn;
			}
			else{
				memcpy(page_buf,sectorbuf,SECTOR_SIZE);
				memcpy(&page_buf[SECTOR_SIZE],&sparedata,16);
				dd_write(ppn,page_buf);
			}
		}
	}
	else{
		for(int i = 0 ; i < BLOCKS_PER_DEVICE; i++){
			// fprintf(stderr,"11->%d\n",i);
			// fprintf(stderr,"11->%d\n",pbn_check[i]);
			if(pbn_check[i] == false){
				addrmaptbl.pbn[lbn] = i;
				// fprintf(stderr,"22->%d\n",i);
				pbn_check[i] = true;
				break;
			}
		}
		pbn = addrmaptbl.pbn[lbn];
		ppn = pbn * PAGES_PER_BLOCK + offset;
		memcpy(&page_buf[SECTOR_SIZE+4],&lbn,4);
		dd_write(pbn * PAGES_PER_BLOCK, page_buf);

		memcpy(page_buf,sectorbuf,SECTOR_SIZE);
		memcpy(&page_buf[SECTOR_SIZE],&sparedata,16);
		dd_write(ppn,page_buf);
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
	int lbn = lsn / PAGES_PER_BLOCK;
	int pbn = addrmaptbl.pbn[lbn];
	int offset = lsn % PAGES_PER_BLOCK;

	int ppn = pbn*PAGES_PER_BLOCK + offset;

	char page_buf[PAGE_SIZE];

	if(dd_read( ppn, page_buf)==1){
		memcpy(sectorbuf,page_buf,SECTOR_SIZE);
	}
	else{
		fprintf(stderr,"read error");
	}

	return;
}
// void ftl_print(){
// 	printf("lbn pbn\n");
// 	for(int i = 0; i < DATABLKS_PER_DEVICE; i++){
// 		printf("%d %d\n",i, addrmaptbl.pbn[i]);
// 	}
// 	printf("free block %d\n",reserved_empty_blk);
// 	return;
// }

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
