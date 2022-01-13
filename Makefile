
ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
else
    detected_OS := $(shell uname)  # same as "uname -s"
	ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
endif



.PHONY: prepare_mac
prepare_mac:
	mkdir -p third_party/poco/build_mac && \
	cd third_party/poco/build_mac && \
	cmake -G Xcode .. -DBUILD_SHARED_LIBS=NO

.PHONY: build_mac
build_mac:
	cd third_party/poco/build_mac && \
	cmake --build . --target Foundation --config Release && \
	cmake --build . --target Util --config Release && \
	cmake --build . --target Net --config Release


.PHONY: prepare_ios
prepare_ios:
	cd third_party/poco_pod && \
	./build-openssl.sh && \
	./build.sh && \
	mkdir -p third_party/poco_ios && \
	tar -xzf "third_party/poco_pod/standard/poco-1.11.1-standard-sdk15.0.tar.gz" -C "third_party/poco_ios"
