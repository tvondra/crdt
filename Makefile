MODULE_big = bdr_crdt
OBJS = bdr_crdt.o

EXTENSION = bdr_crdt
EXTVERSION = 0.0.1
EXTSQL = $(EXTENSION)--$(EXTVERSION).sql

MODULES = bdr_crdt
OBJS = bdr_crdt.o
DATA = uninstall_bdr_crdt.sql bdr_crdt--0.0.1.sql
REGRESS = bdr_crdt

CFLAGS =`pg_config --includedir-server`

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

$(SQL_IN): bdr_crdt.sql.in.c
	$(CC) -E -P $(CPPFLAGS) $< > $@
