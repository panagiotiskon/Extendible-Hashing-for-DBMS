ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -o ./build/runner -O2 -lm
test_main1:
	@echo " Compile test_main1 ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/test_main1.c ./src/hash_file.c -lbf -o ./build/runner -O2 -lm
test_main2:
	@echo " Compile test_main2 ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/test_main2.c ./src/hash_file.c -lbf -o ./build/runner -O2 -lm
bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/runner -O2 -lm

clean:
	$(RM) ./build/runner 
	$(RM) data1.db
	$(RM) data.db
	$(RM) data3.db
	$(RM) data2.db