CFLAGS = -Wall -Wextra -pedantic -std=c17

objects = kibi.o pane.o registers.o undo.o zipperBuffer.o editorRow.o util.o fileData.o

kibi : $(objects)
	cc -o kibi $(objects)

kibi.o: editorRow.h fileData.h pane.h undo.h zipperBuffer.h
zipperBuffer.o: editorRow.h
pane.o: editorRow.h util.h zipperBuffer.h
fileData.o: undo.h zipperBuffer.h

.PHONY : clean
clean :
	rm kibi $(objects)
