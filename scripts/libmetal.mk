
#############################################################
# Compiles an instance of Metal targeted at $(TARGET)
#############################################################
METAL_SOURCE_PATH ?= freedom-metal
METAL_LDSCRIPT	   = $(BSP_DIR)/metal.$(LINK_TARGET).lds
METAL_HEADER	   = $(BSP_DIR)/metal.h
METAL_INLINE       = $(BSP_DIR)/metal-inline.h
PLATFORM_HEADER	   = $(BSP_DIR)/metal-platform.h

METAL_PREFIX       = $(abspath $(BSP_DIR)/install)
METAL_BUILD_DIR    = $(abspath $(BSP_DIR)/build/$(CONFIGURATION))
METAL_LIB_DIR	   = $(abspath $(BSP_DIR)/install/lib/$(CONFIGURATION))

METAL_HEADER_GENERATOR = freedom-metal_header-generator
LDSCRIPT_GENERATOR = freedom-ldscript-generator
MAKEATTRIB_GENERATOR = freedom-makeattributes-generator
BARE_HEADER_GENERATOR = freedom-bare_header-generator

# Metal BSP file generation
#
# Requires devicetree compiler (dtc) and freedom-devicetree-tools to be in the
# PATH, otherwise the existing files are used.
#
# This allows user changes to the devicetree in $(BSP_DIR)/design.dts to be
# propagated through to the end application with a single invocation of Make

ifeq ($(findstring spike,$(TARGET)),spike)
$(BSP_DIR)/spike_options.sh:
	echo "export SPIKE_OPTIONS=\"\"" > $@

ifneq ($(shell which spike),)
$(BSP_DIR)/design.dts: $(BSP_DIR)/spike_options.sh
	. $< && scripts/spikedts $(dir $@)
endif # which spike
endif # findstring spike,$(TARGET)


ifneq ($(shell which dtc),)
ifneq ($(shell which $(METAL_HEADER_GENERATOR)),)

$(BSP_DIR)/design.dtb: $(BSP_DIR)/design.dts
	./scripts/fixup-dts --dts $<
	cd $(dir $@) && dtc -I dts -O dtb -o $(notdir $@) $(notdir $<)

$(BSP_DIR)/metal.default.lds: $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(LDSCRIPT_GENERATOR) -d $(notdir $<) -l $(notdir $@)

$(BSP_DIR)/metal.ramrodata.lds: $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(LDSCRIPT_GENERATOR) -d $(notdir $<) -l $(notdir $@) --ramrodata

$(BSP_DIR)/metal.scratchpad.lds: $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(LDSCRIPT_GENERATOR) -d $(notdir $<) -l $(notdir $@) --scratchpad

$(METAL_INLINE): $(BSP_DIR)/design.dtb
$(METAL_HEADER): $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(METAL_HEADER_GENERATOR) -d $(notdir $<) -o $(notdir $@)

$(PLATFORM_HEADER): $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(BARE_HEADER_GENERATOR) -d $(notdir $<) -o $(notdir $@)

$(BSP_DIR)/settings.mk: $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(MAKEATTRIB_GENERATOR) -d $(notdir $<) -o $(notdir $@) -b $(TARGET)

.PHONY: bsp
metal-bsp:\
	   $(METAL_HEADER) $(METAL_INLINE) $(PLATFORM_HEADER) \
	   $(BSP_DIR)/metal.default.lds \
	   $(BSP_DIR)/metal.ramrodata.lds \
	   $(BSP_DIR)/metal.scratchpad.lds \
	   $(BSP_DIR)/settings.mk
else
.PHONY: bsp
metal-bsp:
	@echo "Make cannot generate a BSP because it cannot find freedom-devicetree-tools"
	@exit 1
endif # which $(METAL_HEADER_GENERATOR)
else
.PHONY: bsp
metal-bsp:
	@echo "Make cannot generate a BSP because it cannot find dtc"
	@exit 1
endif # which dtc


.PHONY: metal
metal: $(METAL_LIB_DIR)/stamp

$(METAL_BUILD_DIR)/Makefile: \
	   $(METAL_HEADER) $(METAL_INLINE) $(PLATFORM_HEADER) \
	   $(BSP_DIR)/settings.mk
	@rm -rf $(dir $@)
	@mkdir -p $(dir $@)
	cd $(dir $@) && \
		CFLAGS="$(RISCV_CFLAGS)" \
		$(abspath $(METAL_SOURCE_PATH)/configure) \
		--host=$(CROSS_COMPILE) \
		--prefix=$(METAL_PREFIX) \
		--libdir=$(METAL_LIB_DIR) \
		--disable-maintainer-mode \
		--with-preconfigured \
		--with-machine-name=$(TARGET) \
		--with-machine-header=$(abspath $(METAL_HEADER)) \
                --with-machine-inline=$(abspath $(METAL_INLINE)) \
		--with-platform-header=$(abspath $(PLATFORM_HEADER)) \
		--with-machine-ldscript=$(abspath $(METAL_LDSCRIPT)) \
		--with-builtin-libgloss
	touch -c $@

$(METAL_LIB_DIR)/stamp: $(METAL_BUILD_DIR)/Makefile
	$(MAKE) -C $(METAL_BUILD_DIR) install
	date > $@

$(METAL_LIB_DIR)/libriscv%.a: $(METAL_LIB_DIR)/stamp ;@:

$(METAL_LIB_DIR)/libmetal.a: $(METAL_LIB_DIR)/libriscv__mmachine__$(TARGET).a
	cp $< $@

$(METAL_LIB_DIR)/libmetal-gloss.a: $(METAL_LIB_DIR)/libriscv__menv__metal.a
	cp $< $@

# If we're cleaning the last Metal library for a TARGET, then remove
# the install directory, otherwise just remove the built libs for that
# CONFIGURATION.
ifeq ($(words $(wildcard $(METAL_PREFIX)/lib/*)),1)
METAL_CLEAN = $(METAL_PREFIX)
else
METAL_CLEAN = $(METAL_LIB_DIR)
endif

.PHONY: clean-metal
clean-metal:
	rm -rf $(METAL_CLEAN)
	rm -rf $(METAL_BUILD_DIR)
clean: clean-metal

metal_install: metal
	$(MAKE) -C $(METAL_SOURCE_PATH) install

