AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =		\
	bootstrap	\
	drm.spec	\
	drm.spec.in

SUBDIRS = linux

rpm:	Makefile
	make dist
	$(RPMBUILD) -ta $(PACKAGE)-$(VERSION).tar.gz
	rm $(PACKAGE)-$(VERSION).tar.gz

