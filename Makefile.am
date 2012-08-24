SUBDIRS = src m4 db ui data
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	git archive --format=tar --prefix=$(distdir)/ v$(VERSION) | bzip2 > $(distdir).tar.bz2
	gpg -b --use-agent $(distdir).tar.bz2

	git archive --format=tar --prefix=$(distdir)/ v$(VERSION) | gzip > $(distdir).tar.gz
	gpg -b --use-agent $(distdir).tar.gz

	chmod 644 $(distdir).tar.*

