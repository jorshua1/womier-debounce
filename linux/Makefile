CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
PKG_CONFIG ?= pkg-config

LIBEVDEV_CFLAGS := $(shell $(PKG_CONFIG) --cflags libevdev)
LIBEVDEV_LIBS := $(shell $(PKG_CONFIG) --libs libevdev)

TARGET = womier-debounce

all: $(TARGET)

$(TARGET): womier-debounce.c
	$(CC) $(CFLAGS) $(LIBEVDEV_CFLAGS) $< -o $@ $(LIBEVDEV_LIBS)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	install -d /etc/udev/rules.d
	install -m 644 99-womier.rules /etc/udev/rules.d/99-womier.rules
	install -d /etc/systemd/system
	install -m 644 womier-debounce.service /etc/systemd/system/womier-debounce.service
	udevadm control --reload-rules && udevadm trigger
	systemctl daemon-reload

clean:
	rm -f $(TARGET)

.PHONY: all install clean
