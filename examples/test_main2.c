//////////////////////////////////////////////////////
// main that tests multiple opened hash table files //
//////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define GLOBAL_DEPT 2    // you can change it if you want

const char *names[] = {
    "Yannis",
    "Christofos",
    "Sofia",
    "Marianna",
    "Vagelis",
    "Maria",
    "Iosif",
    "Dionisis",
    "Konstantina",
    "Theofilos",
    "Giorgos",
    "Dimitris"};

const char *surnames[] = {
    "Ioannidis",
    "Svingos",
    "Karvounari",
    "Rezkalla",
    "Nikolopoulos",
    "Berreta",
    "Koronis",
    "Gaitanis",
    "Oikonomou",
    "Mailis",
    "Michas",
    "Halatsis"};

const char *cities[] = {
    "Athens",
    "San Francisco",
    "Los Angeles",
    "Amsterdam",
    "London",
    "New York",
    "Tokyo",
    "Hong Kong",
    "Munich",
    "Miami"};

#define CALL_OR_DIE(call)         \
    {                             \
        HT_ErrorCode code = call; \
        if (code != HT_OK)        \
        {                         \
            printf("Error\n");    \
            exit(code);           \
        }                         \
    }

const char *fnames[] = {"data1.db", "data2.db", "data3.db"};
int main()
{
    BF_Init(LRU);

    CALL_OR_DIE(HT_Init());

    int indexDesc1, indexDesc2, indexDesc3;
    CALL_OR_DIE(HT_CreateIndex(fnames[0], GLOBAL_DEPT));

    CALL_OR_DIE(HT_CreateIndex(fnames[1], GLOBAL_DEPT));

    CALL_OR_DIE(HT_CreateIndex(fnames[2], GLOBAL_DEPT));

    CALL_OR_DIE(HT_OpenIndex(fnames[0], &indexDesc1));

    CALL_OR_DIE(HT_OpenIndex(fnames[1], &indexDesc2));

    CALL_OR_DIE(HT_OpenIndex(fnames[2], &indexDesc3));

    Record record;
    srand(12569874);
    int r;
    printf("Insert Entries\n");
    for (int id = 0; id < RECORDS_NUM; ++id)
    {
        // create a record
        record.id = rand() % RECORDS_NUM;
        r = rand() % 12;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        r = rand() % 12;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        r = rand() % 10;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);

        CALL_OR_DIE(HT_InsertEntry(indexDesc1, record));

        record.id = rand() % RECORDS_NUM;
        r = rand() % 12;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        r = rand() % 12;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        r = rand() % 10;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);

        CALL_OR_DIE(HT_InsertEntry(indexDesc2, record));

        record.id = rand() % RECORDS_NUM;
        r = rand() % 12;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        r = rand() % 12;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        r = rand() % 10;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);

        CALL_OR_DIE(HT_InsertEntry(indexDesc3, record));
    }

    printf("RUN PrintAllEntries for 1st hash table\n");
    printf("\n\n");

    int id = rand() % RECORDS_NUM;
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc1, &id));
    printf("\n\n");
    printf("RUN PrintAllEntries for 2nd hash table\n");
    printf("\n\n");

    id = rand() % RECORDS_NUM;
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc1, &id));
    printf("\n\n");
    printf("RUN PrintAllEntries for 3d hash table\n");
    printf("\n\n");

    id = rand() % RECORDS_NUM;
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc1, &id));
    printf("\n\n");

    // you can also print entire hash table
    // CALL_OR_DIE(HT_PrintAllEntries(indexDesc1, NULL));
    // printf("\n\n");

    // CALL_OR_DIE(HT_PrintAllEntries(indexDesc2, NULL));
    // printf("\n\n");

    // CALL_OR_DIE(HT_PrintAllEntries(indexDesc3, NULL));
    // printf("\n\n");

    CALL_OR_DIE(HT_CloseFile(indexDesc1));

    CALL_OR_DIE(HT_CloseFile(indexDesc2));

    CALL_OR_DIE(HT_CloseFile(indexDesc3));

    BF_Close();

    remove("data1.db");
    remove("data2.db");
    remove("data3.db");
}