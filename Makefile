CXX=clang++
# suppress all warnings :-(
CXXFLAGS=-std=c++11 -Iapi -w
TARGET=pict
TEST_OUTPUT = test/rel.log test/rel.log.failures test/dbg.log
TEST_OUTPUT += test/.stdout test/.stderr
OBJS = api/combination.o api/deriver.o api/exclusion.o
OBJS += api/model.o api/parameter.o api/pictapi.o
OBJS += api/task.o api/worklist.o cli/ccommon.o cli/cmdline.o
OBJS += cli/common.o cli/cparser.o cli/ctokenizer.o cli/gcd.o
OBJS += cli/gcdexcl.o cli/gcdmodel.o cli/model.o cli/mparser.o
OBJS += cli/pict.o cli/strings.o

pict: $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

test:
	cd test; perl test.pl ../$(TARGET) rel.log

clean:
	rm -f $(TARGET) $(TEST_OUTPUT) $(OBJS)

all: pict

.PHONY: all test clean
