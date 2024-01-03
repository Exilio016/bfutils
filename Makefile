CC = gcc
CFLAGS = -O3 -ggdb
PREFIX = /usr/local
objects = bfstring.o
test_objects = test.o
major = 1
minor = 0

all: $(objects)
	mkdir -p lib
	gcc $(CFLAGS) -Iinclude/ -shared -o lib/libbfutils.so.$(major).$(minor) -Wl,-soname,libbfutils.so.$(major) $^

$(objects): %.o : src/%.c
	gcc $(CFLAGS) -Iinclude/ -c -fPIC $^

$(test_objects): %.o : src/%.c
	gcc $(CFLAGS) -Iinclude/ -c $^

clean:
	find -name "*.o" -exec rm -f {} +;
	find -name "*.so.*" -exec rm -f {} +;
	find -name test -exec rm -r {} +;

test_compile: $(objects) $(test_objects)
	gcc $(CFLAGS) -Iinclude/ $^ -o test

test: test_compile
	./test

install:
	cp include/*.h $(PREFIX)/include/
	cp lib/*.so.* $(PREFIX)/lib/
	ln -sf $(PREFIX)/lib/libbfutils.so.$(major).$(minor) $(PREFIX)/lib/libbfutils.so.$(major)
	ln -sf $(PREFIX)/lib/libbfutils.so.$(major) $(PREFIX)/lib/libbfutils.so

