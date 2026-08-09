SUBDIRS := $(patsubst %/Makefile,%,$(wildcard */Makefile))

all:
	@for dir in $(SUBDIRS); do \
		echo "========================================"; \
		echo "Building task: $$dir"; \
		echo "========================================"; \
		$(MAKE) -C $$dir; \
		echo ""; \
	done

$(SUBDIRS):
	@echo "========================================"
	@echo "Building task: $@"
	@echo "========================================"
	@$(MAKE) -C $@

run-%:
	@if [ -d "$*" ]; then \
		echo "========================================"; \
		echo "Running task: $*"; \
		echo "========================================"; \
		$(MAKE) -C $* run; \
	else \
		echo "Error: Directory $* not found."; \
	fi

test-%:
	@if [ -d "$*" ]; then \
		if grep -q '^test:' $*/Makefile; then \
			echo "========================================"; \
			echo "Running tests for task: $*"; \
			echo "========================================"; \
			$(MAKE) -C $* test; \
		else \
			echo "Info: Task $* does not have any tests configured."; \
		fi; \
	else \
		echo "Error: Directory $* not found."; \
	fi

test:
	@any_tests=0; \
	for dir in $(SUBDIRS); do \
		if grep -q '^test:' $$dir/Makefile; then \
			any_tests=1; \
			echo "========================================"; \
			echo "Running tests in: $$dir"; \
			echo "========================================"; \
			$(MAKE) -C $$dir test; \
			echo ""; \
		fi; \
	done; \
	if [ $$any_tests -eq 0 ]; then \
		echo "No tests found in any of the tasks."; \
	fi

clean:
	@for dir in $(SUBDIRS); do \
		echo "Cleaning task: $$dir"; \
		$(MAKE) -C $$dir clean; \
		echo ""; \
	done

.PHONY: all clean test $(SUBDIRS)