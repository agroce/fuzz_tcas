all: TCAS_driver.cpp
	${AFL_HOME}/afl-clang++ TCAS_driver.cpp -o TCAS_AFL -ldeepstate_AFL
	clang++ TCAS_driver.cpp -o TCAS_LF -ldeepstate_LF -fsanitize=fuzzer
	clang++ TCAS_driver.cpp -o TCAS -ldeepstate
	clang++ TCAS_driver.cpp -o TCAS_cov -ldeepstate --coverage
