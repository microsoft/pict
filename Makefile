CXX=clang++
# suppress all warnings :-(
CXXFLAGS=-std=c++11 -stdlib=libc++ -Iapi -w
TARGET=pict
all:
	$(CXX) $(CXXFLAGS) api/*cpp cli/*cpp -o $(TARGET)

test:
	cd test; perl test.pl ../$(TARGET) rel.log

.PHONY: all test
