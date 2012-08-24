SUBDIRS = icons

desktopdir = $(datadir)/applications
desktop_DATA = \
	rizoma-ventas.desktop	\
	rizoma-compras.desktop	\
	rizoma-admin.desktop	\
	rizoma-informes.desktop

UPDATE_DESKTOP = update-desktop-database $(datadir)/applications || :

install-data-hook: 
	$(UPDATE_DESKTOP)
uninstall-hook: 
	$(UPDATE_DESKTOP)
