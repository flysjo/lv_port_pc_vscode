
BUILDDIR = build

all: config
	@cmake --build $(BUILDDIR) --target all

reconfig:
	@cmake -G Ninja -B $(BUILDDIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) --fresh

config:
	mkdir -p $(BUILDDIR)
	@cmake -G Ninja -B $(BUILDDIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

clean:
	@cmake --build $(BUILDDIR) --target clean
	@rm -rf $(BUILDDIR)/*
