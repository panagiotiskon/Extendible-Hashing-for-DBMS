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
HT_info *hash_file_array;

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
  ht_info.global_depth = depth;

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
  // at start we have two buckets with local depth of 1
  // for each of the created buckets (blocks) initialize and allocate space

  for (int i = 0; i < num_of_blocks; i++)
  {
    createBucket(file_desc, 1); 
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
  int global_depth;
  int local_depth;

  BF_Block *first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(indexDesc, 0, first_block));

  HT_info *ht_info = malloc(sizeof(HT_info));

  // get first block data

  void *data = BF_Block_GetData(first_block);

  // copy the data of the ht_info struct

  memcpy(ht_info, data, sizeof(HT_info));

  total_buckets = ht_info->bucket_num;
  global_depth = ht_info->global_depth;

  // get hash key

  hash_key = reverseBits(bitExtracted(record.id, global_depth, 1), global_depth);

  printf("eeee %d %d %d %d\n", hash_key, record.id, total_buckets, global_depth);

  // calculate offset

  data += sizeof(HT_info);
  printf("aoaoaoaoaoaoa");
  int *p = malloc(total_buckets * sizeof(int));

  memcpy(&p, data, sizeof(p));

  BF_Block *bucket;
  BF_Block_Init(&bucket);
  CALL_BF(BF_GetBlock(indexDesc, p[hash_key], bucket));
  void *bucket_data;
  bucket_data = BF_Block_GetData(bucket);
  HT_Block_info *ht_bucket_info = malloc(sizeof(HT_Block_info));
  memcpy(ht_bucket_info, bucket_data, sizeof(HT_Block_info));

  max_rec = ht_bucket_info->max_records;
  curr_rec = ht_bucket_info->records;
  local_depth = ht_bucket_info->local_depth;
  printf("curr %d\n", curr_rec);

  if (curr_rec < max_rec) // if record can fit into the bucket
  {
    ht_bucket_info->records++;
    memcpy(bucket_data, ht_bucket_info, sizeof(HT_Block_info));
    // calculate offset to put new record

    bucket_data += sizeof(HT_Block_info) + (sizeof(Record) * curr_rec);

    memcpy(bucket_data, &record, sizeof(Record));

    BF_Block_SetDirty(bucket);
    CALL_BF(BF_UnpinBlock(bucket));
    BF_Block_Destroy(&bucket);
  }

  else if (curr_rec == max_rec) // if record cant fit into bucket
  {
    if (local_depth == global_depth) // then we need to double array's size
    {
      printf("eoeoeoeoe");
      global_depth++; // global depth is growing by 1

      int new_size = pow(2, global_depth); // compute new hash table size
      int *new_hashtable = expandingHashTable(p, new_size);

      // update global depth
      data -= sizeof(HT_info);
      ht_info->global_depth = global_depth;
      memcpy(data, ht_info, sizeof(HT_info));

      // update new hashtable
      data += sizeof(HT_info);
      memcpy(data, new_hashtable, sizeof(new_hashtable));
      
      // free old hash table 
      free(p); 
      
      HT_InsertEntry(indexDesc, record);

    }
    else if (global_depth > local_depth) // we need to split the current bucket to two buckets
    { 
      printf("aaaaaaaaaa");
      // create an array that will store all the previous records
      Record* array_of_record = malloc(curr_rec*sizeof(Record)); 
      bucket_data += sizeof(HT_Block_info); 
      for(int i=0; i<curr_rec;i++){
        memcpy(array_of_record, bucket_data, sizeof(Record));
      }

      // update local depth and record count

      local_depth++;
      curr_rec=0;
      // copy it to the bucket info 
      memcpy(bucket_data, ht_bucket_info, sizeof(HT_Block_info));

      // create new bucket       
      createBucket(indexDesc, local_depth);

      // update total buckets in file 
      total_buckets++;
      void* new_data = BF_Block_GetData(first_block);
      memcpy(new_data, ht_info, sizeof(HT_info));

      // update hashtable to show to both buckets
      correct_hashtable(p, hash_key, global_depth); 

      for(int i=0; i<curr_rec; i++){
        HT_InsertEntry(indexDesc, array_of_record[i]); 
      }

    }

    bucket_data += sizeof(HT_Block_info) + (sizeof(Record) * curr_rec);
  }

  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));
  BF_Block_Destroy(&first_block);

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

int bitExtracted(int number, int k, int p)
{
  return (((1 << k) - 1) & (number >> (p - 1)));
}
int reverseBits(int number, int bits)
{
  int reversedNumber = 0;
  for (int i = 0; i < bits; i++)
  {
    reversedNumber = (reversedNumber << 1) | (number & 1);
    number >>= 1;
  }
  return reversedNumber;
}

// create new hash table and copy the existing information to the new one
// return the new hash table

int *expandingHashTable(int *hashtable, int size)
{
  int pos = -1;
  int *new_hashtable = malloc(size * sizeof(int));
  for (int i = 0; i < size; i++)
  {
    pos = extract_bits(i);
    new_hashtable[i] = hashtable[pos];
  }

  for(int i =0; i<size; i++){
    printf("new %d\n", new_hashtable[i]);
  }

  return new_hashtable;
}

// when a new bucket is created correct properly the hashtable

HT_ErrorCode correct_hashtable(int *hashtable, int hashkey, int depth)
{
  int pointers_num, index;
  int size = pow(2, depth);
  index = hashtable[hashkey]; 
  for (int i = 0; i < size; i++)
  {
    if(hashtable[i]==index)
      pointers_num++;
  }
  printf("pointers num %d\n", pointers_num);
  return HT_OK;
}

// extract k-1 bits from k bit number

int extract_bits(int k)
{
  return k >> 1;
}


// Initializes and creates an empty Block(Bucket)

HT_ErrorCode createBucket(int file_desc, int local_depth)
{

  HT_Block_info bf_info;
  BF_Block *new_bucket;
  BF_Block_Init(&new_bucket);
  CALL_BF(BF_AllocateBlock(file_desc, new_bucket));
  void *data = BF_Block_GetData(new_bucket);

  if (data == NULL)
    return HT_ERROR;

  bf_info.max_records = floor((BF_BLOCK_SIZE - sizeof(HT_Block_info *)) / sizeof(Record));
  bf_info.records = 0;
  bf_info.local_depth = local_depth;

  memcpy(data, &bf_info, sizeof(HT_Block_info));

  BF_Block_SetDirty(new_bucket); // set the block dirty
  CALL_BF(BF_UnpinBlock(new_bucket));
  BF_Block_Destroy(&new_bucket);

  return HT_OK; 

}