SUBDIRS := $(patsubst %/,%,$(wildcard [0-9]/[0-9].[0-9]/))

FLAT_TASKS := $(notdir $(SUBDIRS))

get_path = $(filter %/$(1),$(SUBDIRS))

all:
	@for dir in $(SUBDIRS); do \
		echo "========================================"; \
		echo "Building task: $$dir"; \
		echo "========================================"; \
		$(MAKE) -C $$dir; \
		echo ""; \
	done

$(FLAT_TASKS):
	@path="$(call get_path,$@)"; \
	if [ -n "$$path" ]; then \
		echo "========================================"; \
		echo "Building task: $@ (path: $$path)"; \
		echo "========================================"; \
		$(MAKE) -C $$path; \
	else \
		echo "Error: Task $@ not found."; \
	fi

run-%:
	@path="$(call get_path,$*)"; \
	if [ -n "$$path" ]; then \
		echo "========================================"; \
		echo "Running task: $*"; \
		echo "========================================"; \
		$(MAKE) -C $$path run ARGS="$(ARGS)"; \
	else \
		echo "Error: Task $* not found."; \
	fi

test-%:
	@path="$(call get_path,$*)"; \
	if [ -n "$$path" ]; then \
		if grep -q '^test:' $$path/Makefile; then \
			echo "========================================"; \
			echo "Running tests for task: $*"; \
			echo "========================================"; \
			$(MAKE) -C $$path test; \
		else \
			echo "Info: Task $* does not have any tests configured in its Makefile."; \
		fi \
	else \
		echo "Error: Task $* not found."; \
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

.PHONY: all clean test $(FLAT_TASKS)