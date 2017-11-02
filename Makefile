CXX=clang++
# suppress all warnings :-(
CXXFLAGS=-std=c++11 -Iapi -w
TARGET=pict
TEST_OUTPUT = test/rel.log test/rel.log.failures test/dbg.log
TEST_OUTPUT += test/.stdout test/.stderr

all:
	$(CXX) $(CXXFLAGS) api/*cpp cli/*cpp -o $(TARGET)

test:
	cd test; perl test.pl ../$(TARGET) rel.log

clean:
	rm -f $(TARGET) $(TEST_OUTPUT)

.PHONY: all test clean
