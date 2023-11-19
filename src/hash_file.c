#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "hash_file.h"

#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}

HT_ErrorCode HT_Init() {
  ht_files = malloc(MAX_OPEN_FILES*sizeof(int)); // allocate enough space for opened hash files
  for(int i=0; i<MAX_OPEN_FILES; i++)             // initialise the ht_files table
    ht_files[i] =-1;
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  int file_desc;

  // number of blocks will be 2^depth

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

  memcpy(data, "hash", (5 * sizeof(char)));                                     // the first 5 bytes declare this is a heap file
  memcpy(data + (5 * sizeof(char)), &depth, sizeof(int));                       // save next the depth
  memcpy(data + (5 * sizeof(char)) + sizeof(int), &num_of_blocks, sizeof(int)); // save next the number of blocks in the file

  // set the block dirty

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  // asume that each block can fit one bucket

  int hash_key = -1;

  // for each of the created buckets (blocks) initialize and allocate space

  for (int i = 0; i < num_of_blocks; i++)
  {
    BF_Block *temp_block;
    BF_Block_Init(&temp_block);
    CALL_BF(BF_AllocateBlock(file_desc, temp_block));
    void *temp_data = BF_Block_GetData(temp_block);
    memcpy(temp_data, &hash_key, sizeof(int));
    BF_Block_SetDirty(temp_block); // set the block dirty
    CALL_BF(BF_UnpinBlock(temp_block));
    BF_Block_Destroy(&temp_block);

  }

  // close the heap file

  CALL_BF(BF_CloseFile(file_desc));

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){

  int file_desc;

  CALL_BF(BF_OpenFile(fileName, &file_desc));
  BF_Block *block;
  BF_Block_Init(&block);

  CALL_BF(BF_GetBlock(file_desc, 0, block)); 
  void * data = BF_Block_GetData(block);

  // check if the opened file is a hash file

  if(strcmp(data, "hash")!=0)
    return HT_ERROR;

  // find the first open index in the ht files table 

  for (int i = 0; i < MAX_OPEN_FILES; i++)
    if (ht_files[i] == -1)
    {
      ht_files[i] = file_desc;
      *indexDesc = i; 
    }

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {

  int file_desc; 
  file_desc = ht_files[indexDesc];

  CALL_BF(BF_CloseFile(file_desc)); 
  
  ht_files[indexDesc]=-1;
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

