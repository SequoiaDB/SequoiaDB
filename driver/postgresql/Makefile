

CURRENT_PATH = .
MD5_PATH = $(CURRENT_PATH)/external/sdbbson
MD5_HEADER_PATH = $(MD5_PATH)/lib

EXTERNAL_PATH = $(CURRENT_PATH)/external/client
BSON_PATH = $(EXTERNAL_PATH)/sdbbson
ENGINE_PATH = $(CURRENT_PATH)/../../SequoiaDB/engine

BSON_SOURCE = $(BSON_PATH)/numbers.c $(BSON_PATH)/bson.c $(BSON_PATH)/encoding.c $(BSON_PATH)/common_decimal.c

MD5_SOURCE = $(MD5_PATH)/md5.c
MD5_HEADER = $(MD5_HEADER_PATH)/md5.h

EXTERNAL_SOURCE = $(EXTERNAL_PATH)/client.c  $(EXTERNAL_PATH)/oss.c     $(EXTERNAL_PATH)/ossUtil.c \
                  $(EXTERNAL_PATH)/common.c  $(EXTERNAL_PATH)/network.c $(EXTERNAL_PATH)/ossMem.c \
                  $(EXTERNAL_PATH)/base64c.c $(EXTERNAL_PATH)/cJSON.c   $(EXTERNAL_PATH)/jstobs.c \
                  $(EXTERNAL_PATH)/timestampParse.c                     $(EXTERNAL_PATH)/timestampTm.c \
                  $(EXTERNAL_PATH)/timestampValid.c                     $(EXTERNAL_PATH)/cJSON_ext.c \
                  $(EXTERNAL_PATH)/cJSON_iterator.c

BSON_HEADER = $(BSON_PATH)/sdbbson.h $(BSON_PATH)/encoding.h $(BSON_PATH)/common_decimal.h $(BSON_PATH)/common_decimal_fun.h $(BSON_PATH)/common_decimal_type.h 

EXTERNAL_HEADER = $(EXTERNAL_PATH)/client.h   $(EXTERNAL_PATH)/jstobs.h  $(EXTERNAL_PATH)/base64c.h \
                  $(EXTERNAL_PATH)/core.h     $(EXTERNAL_PATH)/ossErr.h  $(EXTERNAL_PATH)/ossFeat.h \
                  $(EXTERNAL_PATH)/ossTypes.h $(EXTERNAL_PATH)/spd.h     $(EXTERNAL_PATH)/client_internal.h \
                  $(EXTERNAL_PATH)/ossUtil.h  $(EXTERNAL_PATH)/ossMem.h  $(EXTERNAL_PATH)/msg.h \
                  $(EXTERNAL_PATH)/msgDef.h   $(EXTERNAL_PATH)/network.h $(EXTERNAL_PATH)/common.h \
                  $(EXTERNAL_PATH)/pmdOptions.h                          $(EXTERNAL_PATH)/msgCatalogDef.h \
                  $(EXTERNAL_PATH)/fmpDef.h   $(EXTERNAL_PATH)/oss.h     $(EXTERNAL_PATH)/cJSON.h \
                  $(EXTERNAL_PATH)/timestamp.h                           $(EXTERNAL_PATH)/clientDef.h \
                  $(EXTERNAL_PATH)/cJSON_ext.h \

$(MD5_PATH)/md5.c:                  $(ENGINE_PATH)/bson/md5.c
$(MD5_HEADER_PATH)/md5.h:           $(ENGINE_PATH)/bson/lib/md5.h

$(BSON_PATH)/numbers.c:             $(ENGINE_PATH)/client/bson/numbers.c
$(BSON_PATH)/bson.c:                $(ENGINE_PATH)/client/bson/bson.c
$(BSON_PATH)/common_decimal.c:      $(ENGINE_PATH)/client/bson/common_decimal.c
$(BSON_PATH)/encoding.c:            $(ENGINE_PATH)/client/bson/encoding.c
$(EXTERNAL_PATH)/client.c:          $(ENGINE_PATH)/client/client.c
$(EXTERNAL_PATH)/oss.c:             $(ENGINE_PATH)/oss/oss.c
$(EXTERNAL_PATH)/ossUtil.c:         $(ENGINE_PATH)/oss/ossUtil.c
$(EXTERNAL_PATH)/ossMem.c:          $(ENGINE_PATH)/oss/ossMem.c
$(EXTERNAL_PATH)/common.c:          $(ENGINE_PATH)/client/common.c
$(EXTERNAL_PATH)/network.c:         $(ENGINE_PATH)/client/network.c
$(EXTERNAL_PATH)/base64c.c:         $(ENGINE_PATH)/client/base64c.c
$(EXTERNAL_PATH)/cJSON.c:           $(ENGINE_PATH)/client/cJSON.c
$(EXTERNAL_PATH)/cJSON_ext.c:       $(ENGINE_PATH)/client/cJSON_ext.c
$(EXTERNAL_PATH)/cJSON_iterator.c:  $(ENGINE_PATH)/client/cJSON_iterator.c
$(EXTERNAL_PATH)/jstobs.c:          $(ENGINE_PATH)/client/jstobs.c
$(EXTERNAL_PATH)/timestampParse.c:  $(ENGINE_PATH)/client/timestampParse.c
$(EXTERNAL_PATH)/timestampTm.c:     $(ENGINE_PATH)/client/timestampTm.c
$(EXTERNAL_PATH)/timestampValid.c:  $(ENGINE_PATH)/client/timestampValid.c

$(BSON_PATH)/sdbbson.h:             $(ENGINE_PATH)/client/bson/bson.h
$(BSON_PATH)/common_decimal.h:      $(ENGINE_PATH)/include/common_decimal.h
$(BSON_PATH)/common_decimal_fun.h:  $(ENGINE_PATH)/include/common_decimal_fun.h
$(BSON_PATH)/common_decimal_type.h: $(ENGINE_PATH)/include/common_decimal_type.h
$(BSON_PATH)/encoding.h:            $(ENGINE_PATH)/client/bson/encoding.h
$(EXTERNAL_PATH)/client.h:          $(ENGINE_PATH)/client/client.h
$(EXTERNAL_PATH)/jstobs.h:          $(ENGINE_PATH)/client/jstobs.h
$(EXTERNAL_PATH)/base64c.h:         $(ENGINE_PATH)/client/base64c.h
$(EXTERNAL_PATH)/network.h:         $(ENGINE_PATH)/client/network.h
$(EXTERNAL_PATH)/common.h:          $(ENGINE_PATH)/client/common.h
$(EXTERNAL_PATH)/client_internal.h: $(ENGINE_PATH)/client/client_internal.h
$(EXTERNAL_PATH)/cJSON.h:           $(ENGINE_PATH)/client/cJSON.h
$(EXTERNAL_PATH)/cJSON_ext.h:       $(ENGINE_PATH)/client/cJSON_ext.h
$(EXTERNAL_PATH)/timestamp.h:       $(ENGINE_PATH)/client/timestamp.h
$(EXTERNAL_PATH)/spd.h:             $(ENGINE_PATH)/include/spd.h
$(EXTERNAL_PATH)/core.h:            $(ENGINE_PATH)/include/core.h
$(EXTERNAL_PATH)/ossErr.h:          $(ENGINE_PATH)/include/ossErr.h
$(EXTERNAL_PATH)/ossFeat.h:         $(ENGINE_PATH)/include/ossFeat.h
$(EXTERNAL_PATH)/ossTypes.h:        $(ENGINE_PATH)/include/ossTypes.h
$(EXTERNAL_PATH)/ossUtil.h:         $(ENGINE_PATH)/include/ossUtil.h
$(EXTERNAL_PATH)/ossMem.h:          $(ENGINE_PATH)/include/ossMem.h
$(EXTERNAL_PATH)/oss.h:             $(ENGINE_PATH)/include/oss.h
$(EXTERNAL_PATH)/msg.h:             $(ENGINE_PATH)/include/msg.h
$(EXTERNAL_PATH)/msgDef.h:          $(ENGINE_PATH)/include/msgDef.h
$(EXTERNAL_PATH)/fmpDef.h:          $(ENGINE_PATH)/include/fmpDef.h
$(EXTERNAL_PATH)/pmdOptions.h:      $(ENGINE_PATH)/include/pmdOptions.h
$(EXTERNAL_PATH)/msgCatalogDef.h:   $(ENGINE_PATH)/include/msgCatalogDef.h
$(EXTERNAL_PATH)/clientDef.h:       $(ENGINE_PATH)/client/clientDef.h

# Compile flags
ALL_DEFINES=$(DEFINES)
DYN_FLAGS:=-fPIC -DSDB_DLL_BUILD -pthread
# Endianess Check
endian := $(shell sh -c 'echo "ab" | od -x | grep "6261" >/dev/null && echo little || echo big')
ifeq ($(endian),big)
    ALL_DEFINES+=-DSDB_BIG_ENDIAN
endif
OPTIMIZATION?=-O3
WARNINGS?=-Wall -Wno-unknown-pragmas -Wno-missing-prototypes -Wno-missing-format-attribute
#DEBUG?=-ggdb -DSDB_USE_OWN_POSTGRES
DEBUG?=-ggdb 
INCLUDEPATH=-I$(EXTERNAL_PATH) -I$(BSON_PATH) -I$(MD5_PATH)
ALL_CFLAGS=$(CFLAGS) $(OPTIMIZATION) $(WARNINGS) $(DEBUG) $(INCLUDEPATH) $(ALL_DEFINES) $(DYN_FLAGS)


# Files declare
COPY_FILES = $(MD5_SOURCE) $(EXTERNAL_SOURCE) $(BSON_SOURCE) $(BSON_HEADER) $(EXTERNAL_HEADER) $(MD5_HEADER)
OBJECTS := $(foreach filename,$(EXTERNAL_SOURCE) $(BSON_SOURCE) $(MD5_SOURCE),$(filename:%.c=%.os))
COMPILEOBJECTS = $(OBJECTS)

# PG Usage
PG_CPPFLAGS = $(ALL_CFLAGS)
EXTENSION = sdb_fdw
DATA = sdb_fdw--1.0.sql
MODULE_big = sdb_fdw
OBJS = sdb_fdw_util.o sdb_fdw.o $(OBJECTS)
#SHLIB_LINK = -lcrypto
SHLIB_LINK = -pthread

# Procedures
local: $(BSON_PATH) $(MD5_HEADER_PATH) $(MD5_PATH) $(COPY_FILES) $(COMPILEOBJECTS) 

$(BSON_PATH):
	mkdir -p $@

$(BSON_PATH)/%.c:
	cp -f $< $@
	sed -i "s/bson/sdbbson/g" $@
	
$(MD5_HEADER_PATH):
	mkdir -p $@
	
$(MD5_HEADER_PATH)/md5.h:
	cp -f $< $@

$(MD5_PATH):
	mkdir -p $@
	
$(MD5_PATH)/md5.c:
	cp -f $< $@

$(EXTERNAL_PATH)/%.c:
	cp -f $< $@
	sed -i "s/bson/sdbbson/g" $@

$(BSON_PATH)/%.h:
	cp -f $< $@
	sed -i "s/bson/sdbbson/g" $@

$(EXTERNAL_PATH)/%.h:
	cp -f $< $@
	sed -i "s/bson/sdbbson/g" $@

$(MD5_PATH)/%.o: %.c
	$(CC) -o $@ -c $(ALL_CFLAGS) $<

$(EXTERNAL_PATH)/%.o: %.c
	$(CC) -o $@ -c $(ALL_CFLAGS) $<

$(BSON_PATH)/%.o: %.c
	$(CC) -o $@ -c $(ALL_CFLAGS) $<

%.os: %.c
	$(CC) -o $@ -c $(ALL_CFLAGS) $<

# Users need to specify their Postgres installation path through pg_config. For
# example: /usr/local/pgsql/bin/pg_config or /usr/lib/postgresql/9.1/bin/pg_config

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
