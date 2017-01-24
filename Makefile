###############################################################################
# Path

DESTDIR		:= $(CURDIR)/out

PREFIX		:= /system
BINDIR		:= $(PREFIX)/sbin
INITRCDIR	:= /etc/init.d

###############################################################################
# Compile

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

ROOT_DIR = ./pre-compile

#CFLAGS	:= $(EXTRA_CFLAGS)
#LDFLAGS	:= $(EXTRA_LDFLAGS)

###############################################################################
CFLAGS  += -DBOARD_$(BOARD)
###############################################################################


#############################################################################
#AS		= $(CROSS_COMPILE)as
#CC		= $(CROSS_COMPILE)gcc
#LD		= $(CC) -nostdlib
#CPP		= $(CC) -E
#AR		= $(CROSS_COMPILE)ar
#NM		= $(CROSS_COMPILE)nm
#STRIP		= $(CROSS_COMPILE)strip
#OBJCOPY		= $(CROSS_COMPILE)objcopy
#OBJDUMP		= $(CROSS_COMPILE)objdump
#PKG_CONFIG	?= $(CROSS_COMPILE)pkg-config
#AWK		= awk
#############################################################################

LIB_ROOT = -L$(ROOT_DIR)/lib \
           -L$(ROOT_DIR)/usr/lib
INC_ROOT = $(ROOT_DIR)/include

GLIB_CFLAGS = -I$(INC_ROOT)/glib-2.0
GLIB_LIBS = -lglib-2.0  -lgthread-2.0 -lglib-2.0 

GTHREAD_CFLAGS = -pthread
GTHREAD_LIBS = -lgthread-2.0 -lglib-2.0 

LOCPLA_CFLAGS = -I$(INC_ROOT)/loc-pla 
LOCPLA_LIBS = -lloc_pla

LOC_CFLAGS = -I$(INC_ROOT)/loc-hal/utils \
             -I$(INC_ROOT)/loc-hal/core \
             -I$(INC_ROOT)/loc-hal 
LOC_LIBS = -lgps_utils_so -lloc_core -lloc_eng_so -lgps_default_so -lloc_ds_api -lloc_api_v02 -lgps_utils_so -ldsi_netctrl

LOCVZW_CFLAGS = -I$(INC_ROOT)/loc-vzw/vzwGpsLocationProvider/loc_ext 
LOCVZW_LIBS = -lloc_ext

GTI_CFLAGS = -I$(INC_ROOT)/garden-test-interfaces 

LOCXTRA_CFLAGS = -I$(INC_ROOT)/loc-xtra 
LOCXTRA_LIBS = -lloc_xtra

LOCATIONSERVICE_CFLAGS = -I$(INC_ROOT)/location-service 
LOCATIONSERVICE_LIBS = -llocationservice

EXT_CFLAGS = -I$(INC_ROOT)/hardware
EXT_LIB = -lloc_stub -llog -lqmi_common_so -lqmi_csi -lqmi_sap -lqmi_cci -lqmi_client_qmux -lconfigdb -lxml \
          -ldsutils -ldiag -ltime_genoff -lqmi_encdec -lqmiservices -lqdi -lqmi_client_helper -lqmi -lqmiidl \
          -lnetmgr -lrmnetctl -lcutils -llbs_core -lizat_core -lrt -ldl -lc -lm

AM_CFLAGS = -g -MD \
            --sysroot=$(ROOT_DIR) \
            -I$(INC_ROOT) \
            $(LOCPLA_CFLAGS) \
            $(LOC_CFLAGS) \
            $(LOCVZW_CFLAGS) \
            $(GTI_CFLAGS) \
            $(LOCXTRA_CFLAGS) \
            $(LOCATIONSERVICE_CFLAGS) \
            $(EXT_CFLAGS) 

REQUIRED_LIBS = $(LIB_ROOT) \
                $(LOCPLA_LIBS) \
                $(LOC_LIBS) \
                $(LOCVZW_LIBS) \
                $(LOCXTRA_LIBS) \
                $(LOCATIONSERVICE_LIBS) \
                $(EXT_LIB)


CFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(GLIB_CFLAGS) -I./src -I./garden-src
LDFLAGS = -lstdc++ -lpthread $(GLIB_LIBS) $(REQUIRED_LIBS)
CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) $(GLIB_CFLAGS)

################################################################################
TARGET = mds_gpsd3
################################################################################

GARDEN_CPP_SRC = \
    garden-src/test_android_gps.cpp \
    garden-src/fake_vzw_jni_bridage.cpp

GARDEN_CPP_OBJ = $(GARDEN_CPP_SRC:.cpp=.o)

MDS_C_SRC = \
    src/mds_logd.cpp \
    src/garden_tools.cpp \
    src/mds_udp_ipc_svr.cpp \
    src/mds_upd_ipc_clnt.cpp \
    src/stackdump.cpp

MDS_C_OBJ = $(MDS_C_SRC:.cpp=.o)

OBJS = $(GARDEN_CPP_OBJ) $(MDS_C_OBJ)
#################################################################################

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS)  -c $< -o $@

.cpp.o:
	$(CC) $(CFLAGS)  -c $< -o $@

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) -o $@ $^ $(LDFLAGS)

install:	install-binary

install-binary:	$(TARGET)
	$(Q)$(call check_install_dir, $(DESTDIR)$(BINDIR))
	$(Q)fakeroot cp -v $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	
clean :
	rm -rf $(OBJS)
	rm -rf $(TARGET)
	find -name "*.d" | xargs rm -rf

###############################################################################
# Functions

define check_install_dir
	if [ ! -d "$1" ]; then mkdir -p $1; fi
endef

define checkboard
ifeq ($(BOARD),)
$$(error BOARD is not found, BOARD=NEO_W100/NEO_W200)
endif
endef