 # 👨‍💻 Extendible Hashing for DBMS

A low-level implementation of extendible hashing for database systems.  <br /> <br />
This method uses directories and buckets to hash data and is widely known for its 
flexibility and efficiency in computing time.

# Simple Example:

For instance, you have this table of records: 

ID | NAME      | SURNAME       | CITY
-- | --------- | ------------- | ----
26  | Maria     | Koronis       | Hong Kong
14  | Christoforos| Gaitanis      | Tokyo
16  | Marianna  | Karvounari    | Miami 
12  | Theofilos | Nikolopoulos  | London  
10  | Iosif     | Svingos       | Tokyo
21  | Theofilos | Michas        | Athens
17  | Giorgos   | Halatsis      | Munich

If each block of memory can have only 2 records the hash file after all insertions will look like this: <br /> <br />
![Έγγραφο χωρίς τίτλο (1)](https://github.com/panagiotiskon/Extendible-Hashing-for-DBMS/assets/48532935/9f394e0c-fd94-41d4-ad77-cf4f3d5b6a2a)

# How to run:

The program can be run by two different main functions. This first one inserts a large number of records in a file and the second one creates and inserts records into three different files simultaneously.


test_main1: 

    make main1
    ./build/runner
test_main2:
    
    make main2
    ./build/runner

### For a more detailed description of the project read here: 

[README (3).pdf](https://github.com/panagiotiskon/Extendible-Hashing-for-DBMS/files/13614287/README.3.pdf)
