CFLAGS=-Wall -O2 -Wno-unused
INCLUDES=-I include
LIB=libDivaS.a

# -----------------------------------------------
# Detect available system libraries
# -----------------------------------------------
LIBGPP   := $(shell echo "int main(int argc,char** argv) {return 0;}" > tmp.c 2>/dev/null && gcc tmp.c -o tmp -lg++     > /dev/null 2>&1 && echo -lg++)
LIBSTDCPP:= $(shell echo "int main(int argc, char** argv) {return 0;}" > tmp.c 2>/dev/null && gcc tmp.c -o tmp -lstdc++ > /dev/null 2>&1 && echo -lstdc++)
$(shell rm tmp.c tmp)

# -----------------------------------------------
# Detect location of DivaS library
# -----------------------------------------------
LIBDIVA=$(shell test -f ../$(LIB) && echo ../$(LIB))
ifeq (,${LIBDIVA})
LIBDIVA=$(shell test -f ../../lib/$(LIB) && echo ../../lib/$(LIB))
endif
ifeq (,${LIBDIVA})
LIBDIVA=-lDivaS
endif

# -----------------------------------------------
#  Libraries to be used
# -----------------------------------------------
LIBS=$(LIBDIVA) -lpthread $(LIBGPP) $(LIBSTDCPP)

# -----------------------------------------------
#  Extra includes to be used
# -----------------------------------------------
EXTRA_INCLUDES=$(shell test -f /usr/include/string.h && echo -include /usr/include/string.h)
EXTRA_INCLUDES+=$(shell test -f /usr/include/strings.h && echo -include /usr/include/strings.h)
EXTRA_INCLUDES+=$(shell test -f /usr/include/memory.h && echo -include /usr/include/memory.h)

# -------------------------------------------------------------------------

TARGET=divaconf
SRC=divaconf.c

# -------------------------------------------------------------------------

$(TARGET): $(patsubst ./%.c,./%.o,$(SRC))
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LIBS) -o $(TARGET)

./%.o:./%.c
	$(CC) $(EXTRA_INCLUDES) -c $(CFLAGS) $(INCLUDES) $< -o $@

./%.o:./%.cpp
	$(CC) $(EXTRA_INCLUDES) -c $(CFLAGS) $(INCLUDES) $< -o $@

# -------------------------------------------------------------------------

clean:
	@rm -f ./*.o $(TARGET) ./.depend ./tmp.o ./tmp

# -------------------------------------------------------------------------

depend:
	@$(CC) $(INCLUDES) $(CFLAGS) -M $(SRC) | \
          sed -e "s/^.*:/.\/&/" - > ./.depend

-include ./.depend

