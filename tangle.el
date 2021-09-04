(mapc 'org-babel-tangle-file
      (--map (concat "source/" it)
             (list "display.org"
                   "editorRow.org"
                   "fileData.org"
                   "kibi.org"
                   "lists.org"
                   "makefile.org"
                   "pane.org"
                   "registers.org"
                   "test.txt"
                   "tuple.org"
                   "undo.org"
                   "util.org"
                   "zipperBuffer.org")))
