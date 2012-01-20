CFLAGS=-std=c99 -pedantic -g -Wall -Wextra -O2
TESTDIR=t
TESTSRC=$(wildcard $(TESTDIR)/*.c)
PREFIX=/usr/local
VERSION=1
BASENAME=libsimplegc.so
SONAME=$(BASENAME).$(VERSION)
.PHONY: test compile-tests all distclean install uninstall

all: CFLAGS+=-fPIC
all: gc.o
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(SONAME) gc.o -lc
distclean:
	@rm -f *.o >/dev/null
	@rm -f $(SONAME) >/dev/null
	@rm -f $(TESTDIR)/*[!c] >/dev/null
	@rm -f $(TESTDIR)/*.log	>/dev/null
	@rm -f example >/dev/null
	@rm -f example*[!c] >/dev/null

install: all
	@mkdir -p $(PREFIX)/lib/ \
		  $(PREFIX)/include/simplegc
	@cp -v *.h $(PREFIX)/include/simplegc/
	@cp -v $(SONAME) $(PREFIX)/lib/
	@unlink $(PREFIX)/lib/$(BASENAME) >/dev/null
	@ln -sv $(PREFIX)/lib/$(SONAME) $(PREFIX)/lib/$(BASENAME)

.ONESHELL:
uninstall:
	@rm -rvf $(PREFIX)/include/simplegc
	@rm -vf $(PREFIX)/lib/$(SONAME)
	@unlink $(PREFIX)/lib/$(BASENAME)

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