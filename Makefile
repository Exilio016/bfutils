# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS =
LDLIBS =

# The test build injects failures, so the library's malloc/realloc/vsnprintf/free
# references are redirected to the wrappers in tests/test_framework.c.
WRAPFLAGS = -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=vsnprintf -Wl,--wrap=free

# Library versioning
LIB_NAME = bfutils
LIB_MAJOR = 0
LIB_MINOR = 1
LIB_PATCH = 0
LIB_VERSION = $(LIB_MAJOR).$(LIB_MINOR).$(LIB_PATCH)
LIB_SONAME = lib$(LIB_NAME).so.$(LIB_MAJOR)

# Project files
BUILD_DIR = build
TARGET = $(BUILD_DIR)/test

LIB_SRCS  = src/bfu_string.c src/bfu_array.c src/bfu_map.c
TEST_SRCS = tests/test_main.c tests/test_framework.c \
            tests/test_array.c tests/test_string.c tests/test_map.c

SRCS = $(LIB_SRCS) $(TEST_SRCS)
OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))

LIB_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(LIB_SRCS:.c=.o)))

# Install locations (overridable)
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
PKGCONFIGDIR = $(LIBDIR)/pkgconfig
CMAKEDIR = $(LIBDIR)/cmake/$(LIB_NAME)

all: $(TARGET) lib

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(WRAPFLAGS) $(LDLIBS)

# Library object files (position-independent for the shared build)
$(BUILD_DIR)/lib_%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

LIB_PIC_OBJS = $(addprefix $(BUILD_DIR)/lib_,$(notdir $(LIB_SRCS:.c=.o)))

# Static library
$(BUILD_DIR)/lib$(LIB_NAME).a: $(LIB_OBJS) | $(BUILD_DIR)
	ar rcs $@ $^

# Shared library
$(BUILD_DIR)/$(LIB_SONAME): $(LIB_PIC_OBJS) | $(BUILD_DIR)
	$(CC) -shared -Wl,-soname,$(LIB_SONAME) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/lib$(LIB_NAME).so: $(BUILD_DIR)/$(LIB_SONAME) | $(BUILD_DIR)
	ln -sf $(LIB_SONAME) $@

$(BUILD_DIR)/lib$(LIB_NAME).so.$(LIB_VERSION): $(BUILD_DIR)/$(LIB_SONAME) | $(BUILD_DIR)
	ln -sf $(LIB_SONAME) $@

lib: $(BUILD_DIR)/lib$(LIB_NAME).a $(BUILD_DIR)/lib$(LIB_NAME).so.$(LIB_VERSION) $(BUILD_DIR)/lib$(LIB_NAME).so

# pkg-config file
$(BUILD_DIR)/$(LIB_NAME).pc: Makefile | $(BUILD_DIR)
	@echo "prefix=$(PREFIX)" > $@
	@echo 'exec_prefix=$${prefix}' >> $@
	@echo "libdir=$(LIBDIR)" >> $@
	@echo "includedir=$(INCLUDEDIR)" >> $@
	@echo "" >> $@
	@echo "Name: $(LIB_NAME)" >> $@
	@echo "Description: bfutils - small C utility library" >> $@
	@echo "Version: $(LIB_VERSION)" >> $@
	@echo 'Libs: -L$${libdir} -l$(LIB_NAME)' >> $@
	@echo 'Cflags: -I$${includedir}' >> $@

pkgconfig: $(BUILD_DIR)/$(LIB_NAME).pc

# cmake package files
$(BUILD_DIR)/cmake/$(LIB_NAME)Config.cmake: Makefile | $(BUILD_DIR)/cmake
	@echo 'include("$${CMAKE_CURRENT_LIST_DIR}/$(LIB_NAME)Targets.cmake")' > $@

$(BUILD_DIR)/cmake/$(LIB_NAME)ConfigVersion.cmake: Makefile | $(BUILD_DIR)/cmake
	@echo "set(PACKAGE_VERSION \"$(LIB_VERSION)\")" > $@
	@echo "set(PACKAGE_VERSION_STRING \"$(LIB_VERSION)\")" >> $@

$(BUILD_DIR)/cmake/$(LIB_NAME)Targets.cmake: Makefile | $(BUILD_DIR)/cmake
	@echo "add_library($(LIB_NAME) SHARED IMPORTED)" > $@
	@echo "set_target_properties($(LIB_NAME) PROPERTIES" >> $@
	@echo "    IMPORTED_LOCATION \"$(LIBDIR)/lib$(LIB_NAME).so.$(LIB_MAJOR)\"" >> $@
	@echo "    INTERFACE_INCLUDE_DIRECTORIES \"$(INCLUDEDIR)\"" >> $@
	@echo ")" >> $@
	@echo "" >> $@
	@echo "add_library($(LIB_NAME)::$(LIB_NAME) ALIAS $(LIB_NAME))" >> $@

$(BUILD_DIR)/cmake:
	mkdir -p $@

cmake: $(BUILD_DIR)/cmake/$(LIB_NAME)Config.cmake \
       $(BUILD_DIR)/cmake/$(LIB_NAME)ConfigVersion.cmake \
       $(BUILD_DIR)/cmake/$(LIB_NAME)Targets.cmake

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: tests/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	@status=0; \
	for suite in string array map; do \
	    echo ""; \
	    echo "===== $$suite ====="; \
	    ./$(TARGET) $$suite; \
	    rc=$$?; \
	    if [ $$rc -ne 0 ]; then \
	        echo "  -> suite '$$suite' exited with status $$rc"; \
	        status=1; \
	    fi; \
	done; \
	exit $$status

coverage:
	$(MAKE) clean
	$(MAKE) BUILD_DIR=$(BUILD_DIR) \
	        CFLAGS="$(CFLAGS) --coverage" LDFLAGS="$(LDFLAGS) --coverage" all
	./$(BUILD_DIR)/test
	BUILD_DIR=$(BUILD_DIR) python3 tests/check_coverage.py

# Remove compiled artifacts
clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $@

# Install headers, libraries, pkg-config and cmake files so other projects can
# import this library (pkg-config: --cflags --libs; cmake: find_package(bfu)).
install: lib pkgconfig cmake
	install -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR) \
	       $(DESTDIR)$(PKGCONFIGDIR) $(DESTDIR)$(CMAKEDIR)
	install -m 0644 $(BUILD_DIR)/lib$(LIB_NAME).a $(DESTDIR)$(LIBDIR)/
	install -m 0755 $(BUILD_DIR)/$(LIB_SONAME) $(DESTDIR)$(LIBDIR)/
	ln -sf $(LIB_SONAME) $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).so.$(LIB_VERSION)
	ln -sf $(LIB_SONAME) $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).so
	install -m 0644 $(BUILD_DIR)/$(LIB_NAME).pc $(DESTDIR)$(PKGCONFIGDIR)/
	install -m 0644 $(BUILD_DIR)/cmake/$(LIB_NAME)Config.cmake \
	       $(DESTDIR)$(CMAKEDIR)/
	install -m 0644 $(BUILD_DIR)/cmake/$(LIB_NAME)ConfigVersion.cmake \
	       $(DESTDIR)$(CMAKEDIR)/
	install -m 0644 $(BUILD_DIR)/cmake/$(LIB_NAME)Targets.cmake \
	       $(DESTDIR)$(CMAKEDIR)/
	install -d $(DESTDIR)$(INCLUDEDIR)/
	install -m 0644 include/*.h $(DESTDIR)$(INCLUDEDIR)/

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).a
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).so.$(LIB_VERSION)
	rm -f $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).so
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/$(LIB_NAME).pc
	rm -f $(DESTDIR)$(CMAKEDIR)/$(LIB_NAME)Config.cmake
	rm -f $(DESTDIR)$(CMAKEDIR)/$(LIB_NAME)ConfigVersion.cmake
	rm -f $(DESTDIR)$(CMAKEDIR)/$(LIB_NAME)Targets.cmake
	rm -f $(DESTDIR)$(INCLUDEDIR)/bfu.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/bfu_types.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/bfu_common.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/bfu_string.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/bfu_array.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/bfu_map.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/rapidhash.h

.PHONY: all test coverage clean lib pkgconfig cmake install uninstall
