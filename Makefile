all: build
	cd build; make -j4

build:
	mkdir build; (cd build; cmake ..)

build-debug:
	mkdir build; (cd build; cmake .. -DCMAKE_BUILD_TYPE=Debug)

clean:
	-rm -rf build
