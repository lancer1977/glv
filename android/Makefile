GLV_LIB = libglv.a
GLV_OBJ = gesture.o glv_activity.o glv_asset.o glv.o rqueue.o

CFLAGS += -I. -fPIC -Wall -Wextra

CFLAGS += -O3 -DNDEBUG
#CFLAGS += -g -DDEBUG

.PHONY: clean

$(GLV_LIB): $(GLV_OBJ)
	$(AR) rc $@ $^

clean:
	-rm -f $(GLV_LIB) $(GLV_OBJ)
