ht:
	@echo "Compile ht_main ..."
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -lm -o ./build/ht_main -O2

bf:
	@echo "Compile bf_main ..."
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/bf_main -O2

clean:
	$(RM) ./build/ht_main ./build/bf_main
	$(RM) data1.db
	$(RM) data.db
