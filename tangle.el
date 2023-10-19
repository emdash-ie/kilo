(mapc 'org-babel-tangle-file
      (-concat (--map (concat "source/" it)
                      (list "display.org"
                            "editorRow.org"
                            "edit.org"
                            "fileData.org"
                            "kibi.org"
                            "lists.org"
                            "pane.org"
                            "registers.org"
                            "string.org"
                            "tuple.org"
                            "undo.org"
                            "util.org"
                            "zipperBuffer.org"))
               (list "makefile.org")))
