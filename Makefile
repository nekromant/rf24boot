SRCDIR=antares
GITURL=https://nekromant@github.com/nekromant/antares.git
OBJDIR=.
TMPDIR=tmp
TOPDIR=.
project_sources=src
ANTARES_DIR:=./antares

ifeq ($(ANTARES_INSTALL_DIR),)
antares:
	git clone $(GITURL) $(ANTARES_DIR)
	@echo "I have fetched the antares sources for you to $(ANTARES_DIR)"
	@echo "Please, re-run make"
else
antares:
	ln -sf $(ANTARES_INSTALL_DIR) $(ANTARES_DIR)
	@echo "Using global antares installation: $(ANTARES_INSTALL_DIR)"
	@echo "Symlinking done, please re-run make"
endif

-include antares/Makefile

define PKG_CONFIG
CFLAGS  += $$(shell pkg-config --cflags  $(1))
LDFLAGS += $$(shell pkg-config --libs $(1))
endef

ifeq ($(CONFIG_ROLE_USERSPACE),y)
$(eval $(call PKG_CONFIG,libusb-1.0))
endif

# You can define any custom make rules right here!
# They will can be later hooked as default make target
# in menuconfig 

rf24tool:
	cd rf24tool/ && make all

rf24tool-%:
	cd rf24tool/ && make $(*)
