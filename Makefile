BUILD_ROOT = ..

include $(BUILD_ROOT)/build/make.defines

LIB = $(LIBTHROCKET_ARCHIVE)
all : $(LIB)

CPPFLAGS	+=									\

#				-fPIC							\

CCSOURCES	=									\
				./src/AlarmDebugLog.cc			\
				./src/Exception.cc				\
				./src/Socket.cc					\
				./src/ThreadMinimal.cc			\

CSOURCES	=									\

include $(BUILD_ROOT)/build/make.rules
