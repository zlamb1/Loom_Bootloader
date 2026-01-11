PODMAN ?= podman
MARKER := $(OUTDIR)/container.marker
TAG := loomos-bootloader

ifeq ($(BUILD_TYPE),podman)
RUN := $(PODMAN) run -it --rm -v ./:/src $(TAG)
endif

.PHONY: container-clean

container-clean:
	podman image rm $(TAG)
	rm -f $(MARKER)