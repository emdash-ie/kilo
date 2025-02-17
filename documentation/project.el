;;; project.el --- An org-mode publishing project for the docs for this app -*- lexical-binding: t; -*-

(let* ((docs-project (list "kibi-docs"
                           :base-directory "source"
                           :publishing-directory "build"
                           :recursive t
                           :publishing-function 'kibi-publish-project)))
  (defun kibi-publish-project (plist file dest)
    ;; currently only used on org-mode files, so equivalent to org-html-publish-to-html
    (let* ((base-directory (plist-get plist :base-directory))
           (relative-file-name (f-relative file base-directory))
           (dest-file-name (concat dest "/" relative-file-name)))
      (if (s-suffix-p ".c" file)
          (copy-file file dest-file-name t)
        (org-html-publish-to-html plist file dest))))
  (org-publish docs-project t))
