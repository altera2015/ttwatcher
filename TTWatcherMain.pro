TEMPLATE = subdirs

SUBDIRS+=ttwatcher
SUBDIRS+=qhttpserver
SUBDIRS+=quazip

quazip.file = quazip/quazip/quazip.pro

qhttpserver.file=qhttpserver/src/src.pro

ttwatcher.file=src/ttwatcher.pro
ttwatcher.depends += qhttpserver
ttwatcher.depends += quazip


