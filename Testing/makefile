# test_format: test_format.cpp test_util.h
# 	g++ -o test_format test_format.cpp test_util.c
test_format: test_format.cpp ../file_util.h test_util.h
	g++ -std=c++11 -o test_format test_format.cpp ../fs.cpp test_util.cpp 
test_open: test_open.cpp ../file_util.h test_util.h
	g++ -std=c++11 -o test_open test_open.cpp ../fs.cpp test_util.cpp 
test_helper: test_helper.cpp ../file_util.h test_util.h
	g++ -std=c++11 -o test_helper test_helper.cpp test_util.cpp
test_write: test_write.cpp ../file_util.h test_util.h
	g++ -std=c++11 -o test_write test_write.cpp ../fs.cpp test_util.cpp 
test_fseek: test_fseek.cpp ../file_util.h test_util.h
	g++ -std=c++11 -o test_fseek test_fseek.cpp ../fs.cpp test_util.cpp 
test_remove: test_remove.cpp ../file_util.h test_util.h
	g++ -std=c++11 -o test_remove test_remove.cpp ../fs.cpp test_util.cpp 
test_dir: test_dir.cpp ../file_util.h test_util.h
	g++ -std=c++11 -g -o test_dir test_dir.cpp ../fs.cpp test_util.cpp 

SHELL:=/bin/bash -O extglob
clean:
	@rm -f !(*.c|*.cpp|makefile|README.txt|*.h)