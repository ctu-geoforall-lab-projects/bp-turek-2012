if DEBUG
DBGFLAGS = -g -DDEBUG
endif
if SAGA_UNICODE
DEP_DEFS = -fPIC `wx-config --unicode=yes --static=no --cxxflags` -D_SAGA_UNICODE $(DBGFLAGS)
DEP_LFLG = -fPIC -shared `wx-config --unicode=yes --static=no --libs`
else
DEP_DEFS = -fPIC `wx-config --unicode=no --static=no --cxxflags` $(DBGFLAGS)
DEP_LFLG = -fPIC -shared `wx-config --unicode=no --static=no --libs`
endif
DEP_LIBS = `gdal-config --libs`
DEF_SAGA           = -D_SAGA_LINUX -D_TYPEDEF_BYTE -D_TYPEDEF_WORD
CXX_INCS           = -I$(top_srcdir)/src/saga_core `gdal-config --cflags`
AM_CXXFLAGS        = -fPIC $(CXX_INCS) $(DEF_SAGA) $(DEP_DEFS) $(DBGFLAGS)
AM_LDFLAGS         = -fPIC -shared -avoid-version $(DEP_LIBS)
pkglib_LTLIBRARIES = libs_webservices.la
libs_webservices_la_SOURCES =\
MLB_Interface.cpp\
wms_import.cpp\
wms_capabilities.cpp\
wms_base.cpp\
wms_gdal_drv.cpp\
wms_layer.cpp\
MLB_Interface.h\
wms_import.h

libs_webservices_la_LIBADD = $(top_srcdir)/src/saga_core/saga_api/libsaga_api.la
