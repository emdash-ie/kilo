CFLAGS = -Wall -Wextra -pedantic -std=c17

kibi: kibi.c editorRow.o zipperBuffer.o undo.o
zipperBuffer: zipperBuffer.c
