TARGET_PEERCAST = peercast-yt/peercast
TARGET_LICENSES = $(addprefix peercast-yt/,$(wildcard licenses/*.txt))

OS = $(shell uname -s | tr A-Z a-z)
ARCH = $(shell uname -p | tr A-Z a-z)
TAR = peercast-yt-$(OS)-$(ARCH).tar.gz

.PHONY: all clean $(TAR)

all: $(TAR)

$(TAR): peercast-yt $(TARGET_PEERCAST) $(TARGET_LICENSES) peercast-yt/LICENSE
	make -C ui all
	tar xf ui/html.tar -C peercast-yt/
	tar xf ui/public.tar -C peercast-yt/
	tar xf ui/assets.tar -C peercast-yt/
	tar czf $(TAR) peercast-yt
clean:
	rm -rf peercast-yt/
	make -C ui clean

peercast-yt:
	mkdir -p peercast-yt

peercast-yt/LICENSE: peercast-yt
	cp core/LICENSE peercast-yt

$(TARGET_PEERCAST): ui/linux/peercast
	$(MAKE) -C ui/linux peercast
	cp ui/linux/peercast $(TARGET_PEERCAST)
	strip -s $(TARGET_PEERCAST)

$(TARGET_LICENSES): $(wildcard licenses/*.txt)
	mkdir -p peercast-yt/licenses
	cp $(wildcard licenses/*.txt) peercast-yt/licenses
