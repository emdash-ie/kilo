CFLAGS = -Wall -Wextra -pedantic -std=c17

objects = kibi.o pane.o registers.o undo.o zipperBuffer.o editorRow.o \
          util.o fileData.o display.o

kibi : $(objects)
	cc -o kibi $(objects)

kibi.o: kibi.c editorRow.h fileData.h pane.h undo.h zipperBuffer.h display.h
zipperBuffer.o: zipperBuffer.c editorRow.h
pane.o: pane.c editorRow.h util.h zipperBuffer.h fileData.h linkedList.h
fileData.o: fileData.c undo.h zipperBuffer.h
display.o: display.c pane.h linkedList.h

.PHONY : clean
clean :
	rm kibi $(objects)
