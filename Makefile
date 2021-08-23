# To use another compiler, such clang++, set the CXX variable
# CXX=clang++
# variables used to generate a source snapshot of the GIT repo
COMMIT=$(shell git log --pretty=format:'%H' -n 1)
SHORT_COMMIT=$(shell git log --pretty=format:'%h' -n 1)
CXXFLAGS=-fPIC -pipe -std=c++11 -O2 -Iapi
TARGET=pict
TARGET_LIB_SO=libpict.so
TEST_OUTPUT = test/rel.log test/rel.log.failures test/dbg.log
TEST_OUTPUT += test/.stdout test/.stderr
OBJS = $(OBJS_API) $(OBJS_CLI)
OBJS_API = api/combination.o api/deriver.o api/exclusion.o
OBJS_API += api/model.o api/parameter.o api/pictapi.o
OBJS_API += api/task.o api/worklist.o
OBJS_CLI = cli/ccommon.o cli/cmdline.o
OBJS_CLI += cli/common.o cli/cparser.o cli/ctokenizer.o cli/gcd.o
OBJS_CLI += cli/gcdexcl.o cli/gcdmodel.o cli/model.o cli/mparser.o
OBJS_CLI += cli/pict.o cli/strings.o

pict: $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

$(TARGET_LIB_SO): $(OBJS)
	$(CXX) -fPIC -shared $(OBJS) -o $(TARGET_LIB_SO)

test: $(TARGET)
	cd test; perl test.pl ../$(TARGET) rel.log

clean:
	rm -f $(TARGET) $(TARGET_LIB_SO) $(TEST_OUTPUT) $(OBJS)

all: pict $(TARGET_LIB_SO)

source: clean
	git archive --prefix="pict-$(COMMIT)/" -o "pict-$(SHORT_COMMIT).tar.gz" $(COMMIT)

.PHONY: all test clean source
