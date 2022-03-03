(mapc 'org-babel-tangle-file
      (--map (concat "source/" it)
             (list "display.org"
                   "editorRow.org"
                   "edit.org"
                   "fileData.org"
                   "kibi.org"
                   "lists.org"
                   "makefile.org"
                   "pane.org"
                   "registers.org"
                   "string.org"
                   "test.txt"
                   "tuple.org"
                   "undo.org"
                   "util.org"
                   "zipperBuffer.org")))
