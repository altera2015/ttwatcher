TEMPLATE = subdirs

SUBDIRS+=ttwatcher
SUBDIRS+=qhttpserver

qhttpserver.file=qhttpserver/src/src.pro

ttwatcher.file=src/ttwatcher.pro
ttwatcher.depends += qhttpserver
