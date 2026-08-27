MODULE_NAME := chardev_module
KDIR ?= /lib/modules/$(shell uname -r)/build
BUILD_DIR ?= $(PWD)/build

all:
	@mkdir -p $(BUILD_DIR)
	@cp $(MODULE_NAME).c $(BUILD_DIR)/
	@echo "obj-m := $(MODULE_NAME).o" > $(BUILD_DIR)/Makefile
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) modules
	@cp $(BUILD_DIR)/$(MODULE_NAME).ko $(PWD)/
	@echo "Done."

clean:
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) clean || true
	@rm -rf $(BUILD_DIR)
	@rm -f $(MODULE_NAME).ko
	@echo "Clean completed."