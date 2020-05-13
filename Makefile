SUBMITTERS := 312238637_312332414
COMPILER := g++
COMPILER_FLAGS := --std=c++11 -Wall -g
SRCS := main.cpp Commands.cpp signals.cpp smash.cpp BuiltinCommands.cpp ExternalCommands.cpp SpecialCommands.cpp TimeOutCommand.cpp
OBJS=$(subst .cpp,.o,$(SRCS))
HDRS := Commands.h signals.h smash.h BuitinCommands.h ExternalCOmmands.h SpecialCommands.h TimeOutCommand.h
SMASH_BIN := smash

$(SMASH_BIN): $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) $^ -o $@

$(OBJS): %.o: %.cpp
	$(COMPILER) $(COMPILER_FLAGS) -c $^

zip: $(SRCS) $(HDRS)
	zip $(SUBMITTERS).zip $^ submitters.txt Makefile

clean:
	rm -rf $(SMASH_BIN) $(OBJS) $(TESTS_OUTPUTS) 
	rm -rf $(SUBMITTERS).zip
