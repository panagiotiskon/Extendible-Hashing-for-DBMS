#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "hash_file.h"

#define MAX_OPEN_FILES 20

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

HT_ErrorCode HT_Init()
{
  hash_file_array = malloc(MAX_OPEN_FILES * sizeof(HT_info)); // allocate enough space for opened hash files
  for (int i = 0; i < MAX_OPEN_FILES; i++)                    // initialise the ht_files table
    hash_file_array[i].file_desc = -1;
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  int file_desc;

  HT_info ht_info;

  // number of blocks will be 2 ^ depth

  int num_of_blocks = pow(2, depth);

  // create new file, open it and get a pointer to the file data

  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &file_desc));
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(file_desc, block)); // initialize and allocate first block that will contain HP_block
  void *data = BF_Block_GetData(block);

  if (data == NULL)
    return HT_ERROR;

  strcpy(ht_info.type, "hash");
  ht_info.file_desc = file_desc;
  ht_info.bucket_num = num_of_blocks;
  int *ht = malloc(num_of_blocks * sizeof(int));

  // first block contains metadata for the hash table, so skip it

  for (int i = 0; i < num_of_blocks; i++)
  {
    ht[i] = i + 1;
  }

  hash_file_array[file_desc] = ht_info;

  // save in the first block the ht_info

  memcpy(data, &ht_info, sizeof(HT_info));

  // also save the hash table in the first block

  data += sizeof(HT_info);

  memcpy(data, &ht, sizeof(int *));

  // set the block dirty

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  // asume that each block can fit one bucket

  int hash_key = -1;

  // for each of the created buckets (blocks) initialize and allocate space

  for (int i = 0; i < num_of_blocks; i++)
  {
    HT_Block_info bf_info;

    BF_Block *temp_block;
    BF_Block_Init(&temp_block);
    CALL_BF(BF_AllocateBlock(file_desc, temp_block));
    void *temp_data = BF_Block_GetData(temp_block);

    bf_info.max_records = floor((BF_BLOCK_SIZE - sizeof(HT_Block_info *)) / sizeof(Record));
    bf_info.records = 0;

    memcpy(temp_data, &bf_info, sizeof(HT_Block_info));

    BF_Block_SetDirty(temp_block); // set the block dirty
    CALL_BF(BF_UnpinBlock(temp_block));
    BF_Block_Destroy(&temp_block);
  }

  // close the hash file

  CALL_BF(BF_CloseFile(file_desc));

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{

  CALL_BF(BF_OpenFile(fileName, indexDesc));

  BF_Block *block;

  BF_Block_Init(&block);

  CALL_BF(BF_GetBlock(*indexDesc, 0, block));

  void *data = BF_Block_GetData(block);

  HT_info *ht_info = malloc(sizeof(HT_info));

  memcpy(ht_info, data, sizeof(HT_info));

  // check if the opened file is a hash file

  if (strcmp(ht_info->type, "hash") != 0)
    return HT_ERROR;

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{

  CALL_BF(BF_CloseFile(indexDesc));

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record)
{
  int total_buckets;
  int hash_key;
  int max_rec;
  int curr_rec;

  BF_Block *temp_block;
  BF_Block_Init(&temp_block);
  CALL_BF(BF_GetBlock(indexDesc, 0, temp_block));

  HT_info *ht_info = malloc(sizeof(HT_info));

  // get first block data

  void *data = BF_Block_GetData(temp_block);

  // copy the data of the ht_info struct

  memcpy(ht_info, data, sizeof(HT_info));

  total_buckets = ht_info->bucket_num;

  // get hash key

  hash_key = Hash_Function(record.id, total_buckets);

  printf("%d %d %d\n", hash_key, record.id, total_buckets);

  // calculate offset

  data += sizeof(HT_info);

  int *p = malloc(total_buckets * sizeof(int));

  memcpy(&p, data, sizeof(p));

  BF_Block *bucket;
  BF_Block_Init(&bucket);
  CALL_BF(BF_GetBlock(indexDesc, p[hash_key] + 1, bucket));
  void *bucket_data;
  bucket_data = BF_Block_GetData(bucket);
  HT_Block_info *ht_bucket_info = malloc(sizeof(HT_Block_info));
  memcpy(ht_bucket_info, bucket_data, sizeof(HT_Block_info));

  max_rec = ht_bucket_info->max_records;
  curr_rec = ht_bucket_info->records;

  printf("curr %d\n", curr_rec); 

  if (curr_rec < max_rec)
  {

    ht_bucket_info->records++;
    memcpy(bucket_data, ht_bucket_info, sizeof(HT_Block_info));
    // calculate offset to put new record
  
    bucket_data += sizeof(HT_Block_info)+(sizeof(Record)*curr_rec); 
    
    memcpy(bucket_data, &record, sizeof(Record));
    
    BF_Block_SetDirty(bucket);
    CALL_BF(BF_UnpinBlock(bucket));
    BF_Block_Destroy(&bucket);
  }

  BF_Block_SetDirty(temp_block);
  CALL_BF(BF_UnpinBlock(temp_block));
  BF_Block_Destroy(&temp_block);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{
  // insert code here
  return HT_OK;
}

HT_ErrorCode HT_HashStatistics(char *filename)
{
  // insert code here

  return HT_OK;
}

int Hash_Function(int record_id, int buckets_num)
{
  return record_id % buckets_num;
}