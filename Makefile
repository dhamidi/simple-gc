CFLAGS=-std=c99 -pedantic -g -Wall -Wextra
TESTDIR=t
TESTSRC=$(wildcard $(TESTDIR)/*.c)
.PHONY: test compile-tests

compile-tests:
	$(foreach TARGET,$(basename $(TESTSRC)),$(MAKE) $(TARGET);)
.ONESHELL:
test: compile-tests
	@$(foreach TEST,$(basename $(TESTSRC)),$(TEST) > $(TEST).log;)
	@if $$(grep "Failed" t/*.log)
	@then 
		@echo "Tests failed."
	@else 
		@echo "Tests passed."
	@fi
$(TESTDIR)/%: $(TESTDIR)/%.c gc.o
	$(CC) -o $@ $(TESTDIR)/$*.c gc.o