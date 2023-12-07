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

HT_ErrorCode HT_Close()
{
  free(hash_file_array);
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

  // hash_file_array[file_desc] = ht_info;   <-----------------------------------

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

  if (strcmp(ht_info->type, "hash") != 0)
    return HT_ERROR;

  else
  {
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
      if (hash_file_array[i].file_desc == -1)
      {
        memcpy(&hash_file_array[i], data, sizeof(HT_info));
        break;
      }
    }
  }

  free(ht_info);

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{

  // remove the file from the Open Files Array

  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (hash_file_array[i].file_desc == indexDesc)
    {
      hash_file_array[i].file_desc = -1;
    }
  }

  // Close the file

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

  // calculate offset

  data += sizeof(HT_info);
  int *p = malloc(pow(2, global_depth) * sizeof(int)); // hash table is size of global depth^2

  memcpy(&p, data, sizeof(p));

  free(ht_info);

  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));
  BF_Block_Destroy(&first_block);

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
  // printf("current records into current bucket %d and current bucket %d\n\n", curr_rec, p[hash_key]);

  if (curr_rec < max_rec) // if record can fit into the bucket
  {
    ht_bucket_info->records++;
    memcpy(bucket_data, ht_bucket_info, sizeof(HT_Block_info));
    // calculate offset to put new record

    bucket_data += sizeof(HT_Block_info) + (sizeof(Record) * curr_rec);

    memcpy(bucket_data, &record, sizeof(Record));
  }

  else if (curr_rec == max_rec) // if record cant fit into bucket
  {
    if (local_depth == global_depth) // then we need to double array's size
    {
      BF_Block *new_block;
      BF_Block_Init(&new_block);
      CALL_BF(BF_GetBlock(indexDesc, 0, new_block));

      HT_info *new_ht_info = malloc(sizeof(HT_info));

      void *new_data = BF_Block_GetData(new_block);

      memcpy(new_ht_info, new_data, sizeof(HT_info));

      global_depth++; // global depth is growing by 1

      new_ht_info->global_depth++;

      int new_size = pow(2, global_depth); // compute new hash table size

      memcpy(new_data, new_ht_info, sizeof(HT_info)); // update global depth

      // printf("new global depth %d \n\n", global_depth);

      int *new_hashtable = expandingHashTable(p, new_size); // create new array

      new_data += sizeof(HT_info);

      memcpy(new_data, &new_hashtable, sizeof(int *));

      // free old hash table
      free(new_ht_info);
      BF_Block_SetDirty(new_block);
      CALL_BF(BF_UnpinBlock(new_block));
      BF_Block_Destroy(&new_block);
      HT_InsertEntry(indexDesc, record);
    }
    else if (global_depth > local_depth) // we need to split the current bucket to two buckets
    {
      // update local depth and record count
      ht_bucket_info->local_depth++;
      ht_bucket_info->records = 0;
      // copy it to the bucket info
      memcpy(bucket_data, ht_bucket_info, sizeof(HT_Block_info));
      // create an array that will store all the previous records
      Record *array_of_record = malloc(max_rec * sizeof(Record)); // bucket is full so max records
      bucket_data += sizeof(HT_Block_info);
      for (int i = 0; i < max_rec; i++)
      {

        memcpy(&array_of_record[i], bucket_data, sizeof(Record)); // copy each record from the bucket to the array of records
        bucket_data = bucket_data + sizeof(Record);
      }


      // create new bucket
      createBucket(indexDesc, local_depth + 1);

      BF_Block *new_block;
      BF_Block_Init(&new_block);
      CALL_BF(BF_GetBlock(indexDesc, 0, new_block)); // get the first block again to update info about total buckets

      HT_info *new_ht_info = malloc(sizeof(HT_info));

      void *new_data = BF_Block_GetData(new_block);

      memcpy(new_ht_info, new_data, sizeof(HT_info));

      new_ht_info->bucket_num++; // update number of buckets

      memcpy(new_data, new_ht_info, sizeof(HT_info)); // copy the updated info
      int counter = 0;
      for (int i = 0; i < pow(2, global_depth); i++)
      {
        if (p[i] == p[hash_key])
        { // run the hash table,if u find "pointer" pointing to the same bucket as the full bucket then store its cell number to an array
          counter++;
        }
      }

      int *position_array = malloc(sizeof(int) * counter);
      int j = 0;
      for (int i = 0; i < pow(2, global_depth); i++)
      {
        if (p[i] == p[hash_key])
        { // run the hash table,if u find "pointer" pointing to the same bucket as the full bucket then store its cell number to an array
          position_array[j] = i;
          j++;
        }
      }

      for (int i = counter / 2; i < counter; i++)
      { // the first half pointers will stay the same,the second half pointers will point to the new bucket created
        p[position_array[i]] = new_ht_info->bucket_num;
      }
      new_data += sizeof(HT_info);         // go to the position where the hash table is stored
      memcpy(new_data, &p, sizeof(int *)); // copy the updated array
      HT_InsertEntry(indexDesc, record);   // first insert the record id given
      for (int i = 0; i < max_rec; i++)
      {
        HT_InsertEntry(indexDesc, array_of_record[i]); // rehash all records from the splited bucket
      }

      free(new_ht_info);
      free(array_of_record);
      free(position_array); 
      BF_Block_SetDirty(new_block);
      CALL_BF(BF_UnpinBlock(new_block));
      BF_Block_Destroy(&new_block);
    }
  }

  //free(p);
  free(ht_bucket_info);

  BF_Block_SetDirty(bucket);
  CALL_BF(BF_UnpinBlock(bucket));
  BF_Block_Destroy(&bucket);
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{

  BF_Block *first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(indexDesc, 0, first_block));

  HT_info *ht_info = malloc(sizeof(HT_info));

  // get first block data

  void *data = BF_Block_GetData(first_block);

  memcpy(ht_info, data, sizeof(HT_info));

  int global_depth = ht_info->global_depth;

  int hash_table_size = pow(2, global_depth);

  int *hash_table = malloc(hash_table_size * sizeof(int));

  data += sizeof(HT_info);

  memcpy(&hash_table, data, sizeof(int *)); // get hash table from first block
  if (id != NULL)
  {

    int hash_key = reverseBits(bitExtracted(*id, global_depth, 1), global_depth); // find hash key for this record id

    // get the block(bucket) number that exists in this hash key array id
    BF_Block *bucket;
    BF_Block_Init(&bucket);
    CALL_BF(BF_GetBlock(indexDesc, hash_table[hash_key], bucket));

    HT_Block_info *ht_b_info = malloc(sizeof(HT_Block_info));

    void *bucket_data = BF_Block_GetData(bucket);

    memcpy(ht_b_info, bucket_data, sizeof(ht_b_info));

    int records_in_bucket = ht_b_info->records;

    bucket_data += sizeof(HT_Block_info);
    int flag = 0;
    for (int i = 0; i < records_in_bucket; i++)
    {
      Record rec;
      memcpy(&rec, bucket_data, sizeof(Record)); // get each record from bucket
      if (rec.id == *id)
      {
        printf("%s %d %s %s \n", rec.city, rec.id, rec.name, rec.surname);
        flag = 1;
      }
      bucket_data = bucket_data + sizeof(Record);
    }
    if (flag == 0)
    {
      printf("ID %d does not exist in the hash table\n\n", *id);
    }
    free(ht_b_info);
    BF_Block_SetDirty(bucket);
    CALL_BF(BF_UnpinBlock(bucket));
    BF_Block_Destroy(&bucket);
  }
  else
  {
    free(hash_table);
    // if id == null print every element of the hash table
    int num_of_buckets = ht_info->bucket_num; // get number of buckets
    int counter = 0;
    for (int i = 0; i < num_of_buckets; i++)
    {
      BF_Block *bucket;
      BF_Block_Init(&bucket);
      CALL_BF(BF_GetBlock(indexDesc, i + 1, bucket)); // get each bucket

      HT_Block_info *ht_b_info = malloc(sizeof(HT_Block_info));

      void *bucket_data = BF_Block_GetData(bucket);

      memcpy(ht_b_info, bucket_data, sizeof(ht_b_info));

      int num_of_records = ht_b_info->records; // get number of records in current bucket

      bucket_data += sizeof(HT_Block_info); // go to records

      for (int j = 0; j < num_of_records; j++)
      {
        Record rec;
        memcpy(&rec, bucket_data, sizeof(Record));
        printf("%s %d %s %s \n", rec.city, rec.id, rec.name, rec.surname);
        bucket_data += sizeof(Record);
        counter++;
      }

      free(ht_b_info);

      BF_Block_SetDirty(bucket);
      CALL_BF(BF_UnpinBlock(bucket));
      BF_Block_Destroy(&bucket);
    }
    printf("Total records in hash table %d\n", counter);
  }
  free(ht_info);
  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));
  BF_Block_Destroy(&first_block);

  return HT_OK;
}

HT_ErrorCode HT_HashStatistics(char *filename, int indexDesc)
{
  int max = -1;
  int min = 9999;
  int counter;
  int total_records = 0;

  BF_Block *first_block;
  BF_Block_Init(&first_block);
  CALL_BF(BF_GetBlock(indexDesc, 0, first_block));

  HT_info *ht_info = malloc(sizeof(HT_info));

  // get first block data

  void *data = BF_Block_GetData(first_block);

  memcpy(ht_info, data, sizeof(HT_info));

  int buckets_num = ht_info->bucket_num; // get bucket number

  printf("File with name %s, has %d blocks(buckets) and 1 more block for the hash table and its info,total %d blocks\n", filename, buckets_num, buckets_num + 1);

  for (int i = 0; i < buckets_num; i++)
  {
    counter = 0;
    BF_Block *bucket;
    BF_Block_Init(&bucket);
    CALL_BF(BF_GetBlock(indexDesc, i + 1, bucket)); // get each bucket

    HT_Block_info *ht_b_info = malloc(sizeof(HT_Block_info));

    void *bucket_data = BF_Block_GetData(bucket);

    memcpy(ht_b_info, bucket_data, sizeof(ht_b_info));

    int num_of_records = ht_b_info->records; // get number of records in current bucket

    bucket_data += sizeof(HT_Block_info); // go to records

    for (int j = 0; j < num_of_records; j++)
    {
      Record rec;
      memcpy(&rec, bucket_data, sizeof(Record));
      bucket_data += sizeof(Record);
      counter++;
    }
    if (counter < min)
    {
      min = counter;
    }
    if (counter > max)
    {
      max = counter;
    }
    free(ht_b_info);
    total_records = total_records + counter;
    BF_Block_SetDirty(bucket);
    CALL_BF(BF_UnpinBlock(bucket));
    BF_Block_Destroy(&bucket);
  }
  free(ht_info);
  BF_Block_SetDirty(first_block);
  CALL_BF(BF_UnpinBlock(first_block));
  BF_Block_Destroy(&first_block);
  printf("\n");
  printf("The minimum number of records in a block is %d\nThe maximum number of records in a block is %d\nThe average number of records in a block is %d \n", min, max, total_records / buckets_num);
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
    //memcpy(&new_hashtable[i], &hashtable[pos], sizeof(int));
    new_hashtable[i] = hashtable[pos];
  } 

  return new_hashtable;
}

// extract k-1 bits from k bit number

int extract_bits(int k)
{
  return k >> 1;
}

// Initializes and creates an empty Block(Bucket)

HT_ErrorCode createBucket(int file_desc, int local_depth)
{

  HT_Block_info *bf_info = malloc(sizeof(HT_Block_info));
  BF_Block *new_bucket;
  BF_Block_Init(&new_bucket);
  CALL_BF(BF_AllocateBlock(file_desc, new_bucket));
  void *data = BF_Block_GetData(new_bucket);

  if (data == NULL)
    return HT_ERROR;

  bf_info->max_records = floor((BF_BLOCK_SIZE - sizeof(HT_Block_info *)) / sizeof(Record));
  bf_info->records = 0;
  bf_info->local_depth = local_depth;

  memcpy(data, bf_info, sizeof(HT_Block_info));
  free(bf_info);

  BF_Block_SetDirty(new_bucket); // set the block dirty
  CALL_BF(BF_UnpinBlock(new_bucket));
  BF_Block_Destroy(&new_bucket);

  return HT_OK;
}

