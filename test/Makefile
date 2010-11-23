# put down here since its only temporary and not a standardized build test
TEST_LIB   = .
_TEST_SRCS = sp_test.c 
TEST_SRCS  = $(patsubst %, $(TEST_LIB)/%,$(_TEST_SRCS))
TESTOBJ    = $(TEST_SRCS:.c=.o)
TESTBIN    = sp_test

CFLAGS = #-Wall -Werror
all: $(TESTBIN)

$(TESTBIN): $(TESTOBJ)
	@echo
	@echo Creating Test Bin $@
	gcc $(TESTOBJ) --output $(TESTBIN)

$(TESTOBJ): %.o : %.c
	@echo
	@echo Compiling Test File: $<
	gcc -c $(CFLAGS) $< -o $@ 

clean: 
	rm -f *.o 
	rm -f $(TEST_BIN)
