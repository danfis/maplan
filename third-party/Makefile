-include ../Makefile.local
-include ../Makefile.include

all: boruvka opts nanomsg protobuf translate val

submodule:
	git submodule init
	git submodule update

boruvka: submodule
	$(MAKE) -C boruvka

opts: submodule
	$(MAKE) -C opts

nanomsg: submodule
	cd nanomsg \
		&& if test ! -f build/lib/libnanomsg.a; then \
				./autogen.sh \
					&& ./configure --enable-static --disable-shared --prefix=`pwd`/build \
					&& make \
					&& make install; \
		fi;
	cd nanomsg4py \
		&& if test ! -f nanomsg2module.so; then \
				make third-party; \
				make; \
				ln -s ../nanomsg4py/nanomsg2module.so ../translate/nanomsg2module.so; \
		fi;

protobuf: submodule
	cd protobuf \
		&& if test ! -f build/lib/libprotobuf.a; then \
				./autogen.sh \
					&& ./configure --enable-static --disable-shared --prefix=`pwd`/build \
					&& make \
					&& make install; \
		fi;
	cd protobuf/python \
		&& if test ! -f build/lib/google/__init__.py; then \
				$(PYTHON2) setup.py build; \
				$(PYTHON2) setup.py bdist_egg --dist-dir build; \
		fi;

translate: submodule
	$(MAKE) -C translate

val: submodule
	$(MAKE) -C VAL

clean:
	$(MAKE) -C translate clean
	$(MAKE) -C boruvka clean
	$(MAKE) -C opts clean
	$(MAKE) -C nanomsg clean
	rm -rf nanomsg/build
	$(MAKE) -C protobuf clean
	rm -rf protobuf/build
	rm -rf protobuf/python/build
	$(MAKE) -C VAL clean

.PHONY: all submodule boruvka opts nanomsg protobuf translate val clean
