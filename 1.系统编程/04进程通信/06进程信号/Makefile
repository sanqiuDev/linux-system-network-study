.PHONY:all
all:testsig mykill

testsig:TestSig01.cc
	g++ -o $@ $^ -std=c++11
mykill:MyKill.cc
	g++ -o $@ $^ -std=c++11

.PHONY:clean
clean:
	rm -f testsig mykill 