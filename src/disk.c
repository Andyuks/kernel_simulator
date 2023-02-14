/* disk.c */
#include <stdlib.h>
#include <stdio.h>
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

/* adds an entry to the disk*/
int add_to_disk(struct disk_t * disk, int num, int data)
{
    int found = 0, blocknum=0, pagenum=0;    
    while( blocknum<MAX_BLOCKS && found == 0)
    {
        while( pagenum<BLOCK_PAGES && found==0)
        {
            if(disk->block[blocknum].page[pagenum].data==-1)
            {   
                disk->block[blocknum].page[pagenum].data = data;
                disk->block[blocknum].page[pagenum].num = num;
                disk->block[blocknum].page[pagenum].garbage = 0;
                printf("\033[1;35mdisk>>\033[0m Added page %d val %d to block %d page %d!!\n", num, data, blocknum, pagenum);
                found=1;
            }
            else
                pagenum ++;
        }

        if(found == 0)
        {
            blocknum++;
            pagenum=0;
        }     
    }
    if(found==0)
    {
        fprintf(stderr,"\033[1;31mError: could not allocate space!!\033[0m\n");
        return -1;
    }
    return BLOCK_PAGES*blocknum + pagenum;
}

/* collects garbage */
void garbage_collector(struct disk_t *  disk, struct ftl_t * ftl)
{
    int i;
    printf("\033[1;35mdisk_GC>>\033[0m victim block %d!!\n", disk->gc.oldest);
    /* write pages 1 by one */
    for(i=0; i<BLOCK_PAGES; i++)
    {
        /* copy entry if needed */
        if(disk->block[disk->gc.oldest].page[i].garbage == 0 && disk->block[disk->gc.oldest].page[i].num!=-1)
        {
            ftl->page[disk->block[disk->gc.oldest].page[i].num].dir = add_to_disk(disk,  disk->block[disk->gc.oldest].page[i].num,  disk->block[disk->gc.oldest].page[i].data); 
            printf("\033[1;35mdisk_GC>>\033[0m ftl page %d ->dir %d!!\n", disk->block[disk->gc.oldest].page[i].num, ftl->page[disk->block[disk->gc.oldest].page[i].num].dir);
        }
        else if(disk->block[disk->gc.oldest].page[i].garbage == 1)
            disk->gc.assigned --;   
    }

    /* bulk erase */
    for(i=0; i<BLOCK_PAGES; i++)
    {
        /* erase information of entry */
        disk->block[disk->gc.oldest].page[i].num=-1;
        disk->block[disk->gc.oldest].page[i].data=-1;
        disk->block[disk->gc.oldest].page[i].garbage=0;
    }
    disk->gc.oldest = (disk->gc.oldest + 1)%MAX_BLOCKS;

    if(disk->gc.assigned==(MAX_BLOCKS*BLOCK_PAGES - (BLOCK_PAGES+1)))
    {
        printf("\033[1;35mdisk_GC>>\033[0m assigned %d / %d -> GC!!\n", disk->gc.assigned, MAX_BLOCKS*BLOCK_PAGES);
        garbage_collector(disk, ftl);
    }
}

/* set a disk entry as garbage*/
void setgarbage(struct disk_t * disk, int totpage)
{
    int blocknum = totpage / BLOCK_PAGES;
    int pagenum = totpage % BLOCK_PAGES;
    disk->block[blocknum].page[pagenum].garbage = 1;
    printf("\033[1;35mdisk>>\033[0m set page %d val %d of block %d page %d as garbage!!\n", disk->block[blocknum].page[pagenum].num, disk->block[blocknum].page[pagenum].data, blocknum, pagenum);
}


/* provides data saved in disk*/
int get_data_disk(struct disk_t * disk, int totpage)
{
    int blocknum = totpage / BLOCK_PAGES;
    int pagenum = totpage % BLOCK_PAGES;
    printf("\033[1;35mdisk>>\033[0m  block %d page %d (pos %d) -> data %d!!\n", blocknum, pagenum, totpage, disk->block[blocknum].page[pagenum].data);
    return disk->block[blocknum].page[pagenum].data;
}

/* obtains data in disk if it is there*/
int get_from_disk(struct disk_t * disk, struct ftl_t * ftl,  int num)
{
    int data = -1;
    if (ftl->page[num].dir!= -1)
        data = get_data_disk(disk, ftl->page[num].dir);
    return data;
    
}

/* adds entry to ftl */
void add_to_ftl(struct disk_t * disk, struct ftl_t * ftl,  int num, int data)
{
    int ret;
    ret = add_to_disk(disk, num, data);
    if(ret!=-1)
    {
        if( ftl->page[num].dir!=-1)
        {
            setgarbage(disk, ftl->page[num].dir);
        }
        ftl->page[num].dir=ret;
        printf("\033[1;35mFTL>>\033[0m page %d dir %d \n", num, ftl->page[num].dir);
        disk->gc.assigned ++;
    }
    else
    {
        printf("\033[1;35mFTL>>\033[0m Nothing was updated, we are sorry, GC failed :)\n");
    }
    printf("\033[1;35mFTL>>\033[0m assigned %d / %d\n", disk->gc.assigned, MAX_BLOCKS*BLOCK_PAGES);
    if(disk->gc.assigned==(MAX_BLOCKS*BLOCK_PAGES - (BLOCK_PAGES+1)))
    {
        printf("\033[1;35mFTL>>\033[0m GC triggered!!\n");
        garbage_collector(disk, ftl);
    }
}



