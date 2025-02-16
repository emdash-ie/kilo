CFLAGS = -Wall -Wextra -pedantic -std=c17 -g

main-objects = source/kibi.o source/rope.o test/main.o

source-objects = $(filter-out $(main-objects),$(patsubst %.c,%.o,$(wildcard source/*.c)) \
	  $(patsubst %.c,%.o,$(wildcard source/lists/*.c)))

test-objects = test/main.o \
  test/munit/munit.o \
	  $(source-objects)

all-objects = $(main-objects) $(source-objects) $(test-objects)

test : $(test-objects) test/display.o
	cc $(CFLAGS) -o run-tests $(test-objects)

kibi : source/kibi.o $(source-objects)
	cc $(CFLAGS) -o kibi source/kibi.o $(source-objects)

test/main.o: test/munit/munit.h source/editorRow.h source/pane.h source/lists/PaneRow.h test/display.c
source/kibi.o: source/kibi.c source/editorRow.h source/fileData.h source/pane.h source/undo.h source/zipperBuffer.h source/display.h source/edit.h
source/zipperBuffer.o: source/zipperBuffer.c source/editorRow.h
source/pane.o: source/pane.c source/editorRow.h source/util.h source/zipperBuffer.h source/fileData.h
source/fileData.o: source/fileData.c source/undo.h source/zipperBuffer.h
source/display.o: source/display.c source/pane.h

.PHONY : clean
clean :
	rm kibi run-tests $(all-objects)
