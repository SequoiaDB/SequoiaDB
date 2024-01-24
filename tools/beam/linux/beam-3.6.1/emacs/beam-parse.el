;; Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
;;
;; IBM Confidential - OCO Source Materials
;;
;; Copyright (C) 2002-2010 IBM Corporation. All rights reserved.
;;
;; US Government Users Restricted Rights - Use, duplication or disclosure
;; restricted by GSA ADP Schedule Contract with IBM Corp
;;
;; The source code for this program is not published or otherwise divested
;; of its trade secrets, irrespective of what has been deposited within
;; the U.S. Copyright Office.
;; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Usage:
;; These two functions are for a file loaded normally, but containing beam
;;  complaint messages (like loading build_errors.beam.yyy)
;;
;;   (beam-parse-buffer)  -  This will take a buffer and enable it to be 
;;                           the target of the parsing
;;   (beam-unparse-buffer)-  This undoes (beam-parse-buffer)
;;
;; The following work on the current beam buffer, set up with (beam-parse-buffer)
;;
;;  (beam-next-error) -      This will move the position in the target error
;;                           log to the next beam error, and bring up the
;;                           source file that contains the error, moving
;;                           to the line in the source where the error is
;;  (beam-next-line-error) - This is like (beam-next-error), but only moves
;;                           to the next line in the current error's path
;;                           instead of moving to the next error.
;;  (beam-next-any-error) -  This will go through the beam errors like
;;                           beam-next-error does, unless there is no beam
;;                           buffer. In that case, it just calls next-error
;;                           from compile.el.
;;  (beam-revisit-error)  -  Goes back to the current error, useful if you
;;                           changed buffers and lost it
;;  (beam-mark-innocent-and-next-error)
;;                        - This makes the current error "innocent", marking
;;                          it so beam will skip it on the next run. It also
;;                          moves to the next error.
;;  (beam-mark-innocent)  - Marks the current error as innocent without moving
;;                          to the next error
;;  (beam-mark-innocent-...)
;;                        - There are variations on the above two functions.
;;                          One allows comments to be added to the innocents
;;                          file, and the other writes the appropriate
;;                          comment in the actual source (/*uninitialized*/)
;;                          instead of writing the innocents file.
;;  (beam-color-path)     - Turns on or off path-coloring.
;;                          If an error has a path, it will be colored when
;;                          this is on. Path coloring defaults to on.
;;  (beam-erase-path)     - Turns off path coloring.
;;
;;  (beam-extended-help)  - Toggles the extended help window.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Changelog
;;
;; Date       Changer    Version  Comment
;; --------------------------------------------------------------------------
;; 9/27/2005  fwalling   3.2.4    - Fixed up beam-color-path so that if the
;;                                  coloring fails, coloring is turned back off.
;; 3/3/2005   fwalling   3.2.3    - Added support for earlier Emacs (20.1)
;;                                  and XEmacs (20.2)
;; 9/16/2004  fwalling   3.2.2    - Added support for multiple BEAM roots
;; 2/17/2004  fwalling   3.2.1    - Added support for multiple colored paths
;; 6/3/2003   fwalling   3.2      - Added support for extended help
;; 5/16/2003  fwalling   3.1.2    - Minor changes to the opening of files
;; 4/24/2003  fwalling   3.1.1    - Minor changes to the display
;; 3/20/2003  fwalling   3.1      - Fixing up minor things, mainly all line 
;;                                  numbers will be jump-to-able
;;
;; 2/10/2003  fwalling   3.0      - Fixing up major things, rewriting the
;;                                  error parsing and walking
;;                                - Removed compilation dependency; doing
;;                                  everything ourselves
;;
;; 5/01/2002  fwalling   2.3      - Added the ability to give reasons for
;;                                  innocents; more functions and keybindings
;;
;; 3/27/2002  fwalling   2.2      - Now setting local variable
;;                                  'backup-inhibited' in innocent buffer
;;                                  so no ~ files are created
;;
;; 3/20/2002  fwalling   2.1      - Added support for innocent and errors file
;;                                  suffixes
;;
;; 2/2002     fwalling   2.0      First real release. Most functionality present
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Developer's notes:
;;
;;  After the beam buffer is parsed, two lists are created and maintained to
;;  traverse the errors: one is the "error-list", and is used to track each
;;  beam error. There is one entry in this list for each beam error in the
;;  beam buffer, and this list is used to traverse from one beam error
;;  to the next (via "(beam-next-error)")
;;
;;  The other list is the "line-list", and is used to track each source line
;;  that beam has given an error about. Each beam error in "error-list" can
;;  span more than one source line, and this list allows this package to
;;  provide for moving from one source line error to the next
;;  (via "(beam-next-line-error)") and for jumping straight to a line in
;;  a source file that is part of a beam error's path.
;;
;;  The "error-list" contains:
;;   (line-list-index marker extent color hash-string innocent-comment)
;;
;;  The "line-list" contains:
;;   (error-list-index file line marker extent beam-error-marker)
;;
;;  The colors and extents used:
;;
;;  - All of the errors are colored according to their status
;;   (extents stored in the error-list)
;;     * Red    = Not seen   (face: beam-error-face)
;;     * Orange = Seen       (face: beam-visited-error-face)
;;     * Green  = Innocent   (face: beam-innocent-face)
;;  - The current path is colored when it is requested (extents stored in
;;    the global beam-path-color-list)
;;      Face: beam-path-face
;;  - The current error (extents stored in beam-error-color-list)
;;      Face: beam-highlight-face
;;  - The color to use when mousing-over a line to jump to (stored in each
;;    line-list extent)
;;      Face: beam-hover-face
;;
;; TODO: Make path coloring smarter.
;;       Today it's a hack: turning it on and off sets a variable,
;;       but still does the coloring (find-and-execute has a check
;;       to see if it's on an error, which should not be needed).
;;       This is what should happen: beam-redisplay should also do
;;       coloring. Turning on and off colors should toggle the global
;;       variable, and just call beam-redisplay.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(provide 'beam-parse)

(defconst beam-parse-version '3.2.4 "Beam-parse version")

;;========================================
;; Key Bindings
;;========================================

(defun global-set-key-if-not-set (key symbol)
  "Sets the global key if the global key map does not have it yet"
  (if (or (equal (lookup-key global-map key) nil)
	  (integerp (lookup-key global-map key)))
      (global-set-key key symbol)))

;; My prefix was Control-c b
(global-set-key-if-not-set "\C-cbc"    'beam-color-path)
(global-set-key-if-not-set "\C-cbe"    'beam-erase-path)
(global-set-key-if-not-set "\C-cbp"    'beam-parse-buffer)
(global-set-key-if-not-set "\C-cbu"    'beam-unparse-buffer)
(global-set-key-if-not-set "\C-cbn"    'beam-next-error)
(global-set-key-if-not-set "\C-cbl"    'beam-next-line-error)
(global-set-key-if-not-set "\C-cbr"    'beam-revisit-error)
(global-set-key-if-not-set "\C-cbi"    'beam-mark-innocent)
(global-set-key-if-not-set "\C-cb\C-i" 'beam-mark-innocent-and-next-error)
(global-set-key-if-not-set "\C-cbk"    'beam-mark-innocent-in-source)
(global-set-key-if-not-set "\C-cb\C-k"
			   'beam-mark-innocent-in-source-and-next-error)
(global-set-key-if-not-set "\C-cbs"    'beam-mark-innocent-and-comment)
(global-set-key-if-not-set "\C-cb\C-s"
			   'beam-mark-innocent-and-comment-and-next-error)
(global-set-key-if-not-set "\C-cbh"    'beam-extended-help)

;; This one I'll set all the time, because emacs sets it by default and we
;; want to override it.
;; This way, if a beam buffer is being parsed, C-x ` will work as expected.
;; If not, it'll just do what it normally does.
(global-set-key "\C-x`"  'beam-next-any-error)

;;========================================
;; Setup
;;========================================

;; Only needed for beam-next-any-error
(require 'compile)

;; This is used to find supporting files that come with the beam
;; installation, like the extended help file.

(defvar beam-install-dir nil
  "Directory (absolute) where beam is installed. This is
used to find supporting files that come with beam, like
the extended help file.

This is set automatically if beam-parse.el was loaded from
the emacs load-path and it is still inside the beam install
point.

To set it manually (if beam-parse.el can't find it, or if
you want to override it), add this to your emacs startup
file:

 (setq beam-install-dir \"/opt/beam/2.0\")

Do not append a trailing slash to the directory name.

See http://w3.eda.ibm.com/beam/emacs.html for more information.")

;; These are used in parsing beam output

(defconst beam-error-regexp
  "^-- \\([A-Z0-9]+\\).*?\\(/\\*.*?\\*\/\\) +>>\\(>.*?\\)$"
  "Regexp to match a beam error and extract needed information")

(defconst beam-line-regexp
  "^ *?\"\\(.+\\)\", line \\([0-9]+\\): +\\(.*?\\)$"
  "Regexp to match error line and extract the needed information")

(defconst beam-error-regexp-error   1 "Error string in the beam-error-regexp")
(defconst beam-error-regexp-comment 2 "Comment string in the beam-error-regexp")
(defconst beam-error-regexp-hash    3 "Hash string in the beam-error-regexp")

(defconst beam-line-regexp-file     1 "Filename string in the beam-line-regexp")
(defconst beam-line-regexp-line     2 "Line number in the beam-line-regexp")
(defconst beam-line-regexp-text     3 "Error text in the beam-line-regexp")


;; This is used in parsing the extended help file

(defconst beam-snippet-regexp
  "<snippet +\"\\(.*?\\)\".*?>\n\\(\\(\n\\|.\\)*?\\)</snippet>"
  "Regexp to match a snippet in an extended help file")

(defconst beam-snippet-name-index 1 "Snippet name in the beam-snippet-regexp")
(defconst beam-snippet-text-index 2 "Snippet text in the beam-snippet-regexp")

;; This is used when looking for path color information in the error file

(defconst beam-intervals-regexp
  "^\"\\([^\"]+\\)\":.*?\\((beam-color-intervals *?'([0-9()]+)[^)]*?)$\\)"
  "Regexp to match the intervals to color from the error file")

(defconst beam-intervals-file-index 1
  "File name in the beam-intervals-regexp")

(defconst beam-intervals-intervals-index 2
  "Interval in the beam-intervals-regexp")

;; These are the list indicies in the error and line lists

(defconst beam-error-line-list-index 0 "Line-list index in the error-list")
(defconst beam-error-marker-index    1 "Marker in the error-list")
(defconst beam-error-extent-index    2 "Extent in the error-list")
(defconst beam-error-color-index     3 "Color in the error-list")
(defconst beam-error-hash-index      4 "Hash string in the error-list")
(defconst beam-error-comment-index   5 "Comment string in the error-list")
(defconst beam-error-error-index     6 "Error string in the error-list")

(defconst beam-line-error-list-index  0 "Error-list index in the line-list")
(defconst beam-line-file-index        1 "Filename in the line-list")
(defconst beam-line-line-index        2 "Line number in the line-list")
(defconst beam-line-marker-index      3 "Marker in the line-list")
(defconst beam-line-extent-index      4 "Extent in the line-list")
(defconst beam-line-beam-marker-index 5 "Beam-buffer marker in the line-list")


(defconst beam-root-dir-regexp-pre "^BEAM_ROOT"
  "First half of the regexp to match the beam root dir")

(defconst beam-root-dir-regexp-post "=\\(.*\\)$"
  "Second half of the regexp to match the beam root dir")

(defconst beam-build-root-regexp "^BEAM_BUILD_ROOT=\\(.*\\)$"
  "Regexp to match the build root directory")

(defconst beam-root-file-regexp "^\\$ROOT\\([0-9]+\\)/\\(.*\\)$"
  "Regexp to match a relative file name that refers to $ROOTn")

(defconst beam-root-file-root-index 1 "Root index in the root-file regexp")
(defconst beam-root-file-file-index 2 "File index in the root-file regexp")

(defconst beam-error-dir-regexp "^BEAM_DIRECTORY_WRITE_ERRORS=\\(.*\\)$" 
  "Regexp to match error directory")

(defconst beam-innocent-dir-regexp "^BEAM_DIRECTORY_WRITE_INNOCENTS=\\(.*\\)$"
  "Regexp to match innocent directory")

;; Other miscellaneous settings

(defvar beam-innocent-suffix ""
  "This suffix will be appended to the innocents filename before it is opened.")

(defvar beam-error-suffix ""
  "This suffix will be appended to the errors filename before it is opened.")

(defvar beam-global-buffer nil
  "The current beam buffer being used to find errors.")

(defvar beam-source-buffer nil
  "The source buffer containing the current source file.")

(defvar beam-interval-buffer nil
  "The buffer that's being colored during a path color. Used internally.")

(defvar beam-extended-help-buffer nil 
  "The extended help buffer, if help is open")

(defvar beam-path-color-list nil
  "List of color handles used to color a path")

(defvar beam-error-color-list nil
  "List of color handles used to color the error")

(defvar beam-path-color-enabled t
  "True if path coloring is on, false otherwise. Default is t")

;; Our faces, used to color the errors and such
;; These can be customized via the normal emacs face editing facilities
;; (In XEmacs: 'M-x customize-face RET' or Menu: "Options -> Edit Faces...")
;; (In Emacs:  'M-x customize-face RET' or
;;              Menu: "Options -> Customize Emacs -> Specific Face", RET)

(defface beam-error-face
  '((((class color)) (:foreground "red")))
  "Used to highlight errors that have not been visited")

(defface beam-visited-error-face
  '((((class color)) (:foreground "orange")))
  "Used to highlight errors that have been visited")

(defface beam-innocent-face
  '((((class color)) (:foreground "green")))
  "Used to highlight errors that have been marked as innocent")

(defface beam-path-face
  '((((class color)) (:foreground "yellow")))
  "Used to highlight a path in the source code that the beam error is for")

(defface beam-highlight-face
  '((((class color)) (:foreground "lightblue")))
  "Used to mark the current beam error in the beam buffer and the source file")

(defface beam-hover-face
  '((((class color)) (:foreground "blue" :background "gray")))
  "Used to highlight an error as the mouse passes over it")


;; This is used to pass data between the comment buffer
;; and the innocent function
(defvar beam-comment-data nil "Used internally")

;; This is used to map source files to open buffers.
(defvar beam-file-buffer-hash nil "Used internally")

;; This is the hash table that caches off help file contents
(defvar beam-extended-help-hash nil "Used internally")

;; This is used as a backwards compatibility. Use faces instead.
(defvar beam-highlight-color nil
  "Don't set me anymore. See 'beam-highlight-face'.")


;;========================================
;; Interactive functions
;;========================================

(defun beam-parse-buffer (&optional buffer-to-parse)
  "This function will activate a beam log file to enable
the compilation commands to work on it"
  (interactive)
  (beam-erase-path) ;; Kill stale colors
  (let* ((buffer-str (or buffer-to-parse
			 (read-buffer "Beam log buffer? "
				      (buffer-name (current-buffer)) t)))
	 (buffer (get-buffer buffer-str)))
    (save-excursion
      (if (beam-get-buffer t) (beam-unparse-buffer))
      (setq beam-global-buffer buffer)
      (beam-error-parse-buffer)))

  ;; Activate it
  (switch-to-buffer (beam-get-buffer))
  (goto-char (point-min))

  ;; Redisplay
  (beam-redisplay t))


(defun beam-unparse-buffer ()
  "Returns the beam log to a normal (not parsing) state"
  (interactive)
  (save-excursion
    (let ((last-buffer (beam-get-buffer)))
      (beam-clear-colors beam-path-color-list)
      (beam-clear-colors beam-error-color-list)
      (setq beam-path-color-list nil)
      (setq beam-error-color-list nil)
      (setq beam-path-color-enabled nil)
      (beam-clear-buffer-colors)
      (set-buffer last-buffer)
      (fundamental-mode)
      (kill-local-variable 'beam-error-list)
      (kill-local-variable 'beam-error-list-index)
      (kill-local-variable 'beam-line-list)
      (kill-local-variable 'beam-line-list-index)
      (setq beam-global-buffer nil)
      (setq beam-source-buffer nil)
      (setq beam-extended-help-hash nil)
      (setq beam-file-buffer-hash nil)
      (message "Released beam buffer."))))


(defun beam-next-any-error (argp)
  "Acts like beam-next-error if a beam buffer is being used.
Otherwise, acts like next-error from compile.el"
  (interactive "p")
  (if (equal nil (beam-get-buffer t))
      (next-error argp)
    (beam-next-error argp)))


(defun beam-next-error (argp)
  "Moves to the next beam error found in the beam buffer"
  (interactive "p")
  (beam-error-goto-next-error argp))


(defun beam-next-line-error (argp)
  "Moves to the next error line in the current error's path"
  (interactive "p")
  (beam-line-error-goto-next-error argp))


(defun beam-revisit-error ()
  "Moves to the current error in the beam log if you lost it"
  (interactive)
  (beam-redisplay))


(defun beam-mark-innocent ()
  "Marks the current error innocent without moving to the next."
  (interactive)
  (beam-mark-innocent-error))


(defun beam-mark-innocent-and-next-error (argp)
  "Marks the error being looked at as innocent, and moves to
the next (or nth) error."
  (interactive "p")
  (beam-mark-innocent-error)
  (beam-next-error argp))


(defun beam-mark-innocent-in-source ()
  "Writes the innocent comment of the current error into the source buffer"
  (interactive)
  (beam-mark-innocent-source))


(defun beam-mark-innocent-in-source-and-next-error (argp)
  "Writes the innocent comment of the current error into the source buffer
and moves to the next (or nth) error."
  (interactive "p")
  (beam-mark-innocent-source)
  (beam-next-error argp))


(defun beam-mark-innocent-and-comment ()
  "Marks the error being looked at as innocent, with a comment that goes
in the innocent file."
  (interactive)
  (beam-get-innocent-comment 0))


(defun beam-mark-innocent-and-comment-and-next-error (argp)
  "Marks the error being looked at as innocent, with a comment, and moves
to the next (or nth) error."
  (interactive "p")
  (beam-get-innocent-comment argp))


(defun beam-color-path ()
  "If this error has a path coloring, this function colors the path"
  (interactive)
  (beam-clear-colors beam-path-color-list)
  (setq beam-path-color-list nil)
  (beam-find-and-execute-color-string)			
  (setq beam-path-color-enabled t))


(defun beam-erase-path ()
 "If a path is colored, this erases that color"
 (interactive)
 (beam-clear-colors beam-path-color-list)
 (setq beam-path-color-list nil)
 (setq beam-path-color-enabled nil))


(defun beam-extended-help ()
 "This toggles the extended help window. While open, this window will display
a generic, high-level description of the current error, common causes,
and some hints as to where to look in your code to figure out what
could be wrong.

Running this command while the help window is activated will de-activate
it. 'Activated' is defined as visible - so if the help window is
visible, this will make it disappear; if it is not visible, this will
make it visible.

Moving to a different error will update this window with a description
of the new error if this window is still visible.
"
 (interactive)

 (if beam-extended-help-buffer
     ;; It's active; disable it
     (progn
       (kill-buffer beam-extended-help-buffer)
       (setq beam-extended-help-buffer nil))
   ;; else, enable it
   (setq beam-extended-help-buffer
	 (get-buffer-create " *Beam Extended Help* ")))

 ;; Redisplay to get the right windows showing
 (if (beam-get-buffer t) (beam-redisplay)))


;;========================================
;; Utilities
;;========================================

; Gnu emacs doesn't have (line-number point) so I make one if I can't find one
(if (not (fboundp 'line-number))
    (defun line-number (&optional point)
      "Returns the line that point exists on in the current buffer"
      (save-excursion
	(if point (goto-char point))
	(+ (count-lines 1 (point))
	   (if (= (current-column) 0) 1 0)))))

; Gnu emacs doesn't have (copy-list list) so I make one if I can't find one
(if (not (fboundp 'copy-list))
    (defun copy-list (lis)
      "Returns a shallow copy the list"
      (let ((copy nil)
	    (orig lis))
	(while orig
	  (setq copy (append copy (list (car orig))))
	  (setq orig (cdr orig)))
	copy)))


; Gnu emacs doesn't have (read-directory-name opts) so
; I make one if I can't find one
(if (not (fboundp 'read-directory-name))
    (defun read-directory-name (prompt &optional dir default must-match
				                 initial-contents history)
      "Just calls read-file-name. It's a shame emacs doesn't
have a way to do this better."
      (read-file-name prompt dir default must-match initial-contents)))

; Gnu emacs doesn't have puthash, but it has cl-puthash
(if (not (fboundp 'puthash))
    (progn
      (require 'cl)
      (defun puthash (key value table)
        "Just calls cl-puthash"
        (cl-puthash key value table))))

(defun beam-regexp-group (num &optional target)
  "Returns the num'th group in target of the last regexp run.
Target defaults to the current buffer's contents."
  (if target
      (substring target (match-beginning num) (match-end num))
    (buffer-substring (match-beginning num) (match-end num))))


(defun beam-get-buffer (&optional silent)
  "Finds the beam buffer, returning it. Errors (unless silent is 't') when
there is no beam buffer."
  (or
   (and
    (or beam-global-buffer nil)
    (or (buffer-live-p beam-global-buffer) (setq beam-global-buffer nil))
    beam-global-buffer)
   (if silent nil
     (error
      "Beam log not found. Perhaps you need to call (beam-parse-buffer)"))))


(defun beam-get-source-buffer ()
  "Finds the source buffer, returning it. Returns nil if the buffer
doesn't exist, or has been closed."
  (cond
   ((not beam-source-buffer) nil)
   ((not (buffer-live-p beam-source-buffer)) (setq beam-source-buffer nil))
   (t beam-source-buffer)))


(defun beam-get-help-buffer ()
  "Finds the help buffer, returning it. Returns nil if the buffer doesn't exist,
but re-creates it if it was closed."
  (cond
   ((not beam-extended-help-buffer) nil)
   ((not (buffer-live-p beam-extended-help-buffer)) 
    (setq beam-extended-help-buffer
	  (get-buffer-create " *Beam Extended Help* ")))
   (t beam-extended-help-buffer)))


(defun beam-get-root-dir (&optional suffix)
  "Locates the BEAM_ROOT=... above the current beam error in the
beam buffer. If suffix is given, this searches for BEAM_ROOTsuffix instead
of just BEAM_ROOT"
  (let ((buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer buffer)
      (beam-goto-current-beam-error)
      (let* ((root (concat "BEAM_ROOT" (or suffix "")))
	     (regexp (concat beam-root-dir-regexp-pre
			     (or suffix "")
			     beam-root-dir-regexp-post))
	     (search-pos (re-search-backward regexp nil t)))
	(if (equal nil search-pos)
	    (error 
	     (concat
	      "Buffer Parse Error: No " root "=... above current error!"))
	  (let ((root-dir (beam-regexp-group 1)))
	    (if (equal 0 (length root-dir))
		(expand-file-name (read-directory-name 
				   (concat root
					   " was empty."
					   " Beam root directory: ")
				   nil nil t))
	      root-dir)))))))


(defun beam-get-build-root ()
  "Locates the beam-build-root-regexp above the current beam error
in the beam buffer. May return nil if no build-root is found."
  (let ((buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer buffer)
      (beam-goto-current-beam-error)
      (let ((search-pos (re-search-backward beam-build-root-regexp nil t)))
	(if (equal nil search-pos)
            nil
          (beam-regexp-group 1))))))


(defun beam-get-innocent-dir ()
  "Locates the beam-innocent-dir-regexp above the current beam error
in the beam buffer"
  (let ((buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer buffer)
      (beam-goto-current-beam-error)
      (let ((search-pos (re-search-backward beam-innocent-dir-regexp nil t)))
	(if (equal nil search-pos)
	    (error (concat
		    "Buffer Parse Error: No BEAM_DIRECTORY_WRITE_INNOCENTS=..."
		    " above current error!"))
	  (let ((innocent-dir (beam-regexp-group 1)))
	    (if (equal 0 (length innocent-dir))
		(progn
		  (setq innocent-dir
			(expand-file-name
			 (read-directory-name
			  (concat
			   "BEAM_DIRECTORY_WRITE_INNOCENTS was empty."
			   " Beam innocent directory: "))))
		  (if (not (file-directory-p innocent-dir))
		      (progn
			(if (file-exists-p innocent-dir) 
			    (error (concat "Aborted: "
					   innocent-dir
					   " is a file name")))
			(if (yes-or-no-p (concat innocent-dir
						 " does not exist. Create? "))
			    (make-directory-path innocent-dir)
			  (error (concat "Aborted: "
					 innocent-dir
					 " does not exist.")))))))
	    innocent-dir))))))


(defun beam-get-error-dir ()
  "Locates the beam-error-dir-regexp above the current beam error
in the beam buffer"
  (let ((buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer buffer)
      (beam-goto-current-beam-error)
      (let ((search-pos (re-search-backward beam-error-dir-regexp nil t)))
	(if (equal nil search-pos)
	    (error (concat 
		    "Buffer Parse Error: No BEAM_DIRECTORY_WRITE_ERRORS=..."
		    " above current error!"))
	  (let ((error-dir (beam-regexp-group 1)))
	    (if (equal 0 (length error-dir))
		(expand-file-name
		 (read-directory-name
		  (concat
		   "BEAM_DIRECTORY_WRITE_ERRORS was empty."
		   " Beam error directory: ") nil nil t))
	      error-dir)))))))


(defun beam-get-error-file (source-file)
  "Locates the error file (with path color information) based on
the given source file.

If the source-file is relative, it is resolved according to
the rules in beam-resolve-file-name.

The source-file is then shortened according to these rules:

  - If there was a BUILD_ROOT in the complaint output, BEAM
    wrote the error file in a heirarchy inside the error
    directory. The source file has the build root stripped
    from it, and the resulting relative path is appended to
    the error directory.

  - If there was no BUILD_ROOT in the complaint output, BEAM
    write the error file directly in the error directory,
    using the basename of the source file.

Return the absolute path to the error file, which contains
the highlighting information."
  (let* ((full-name (beam-resolve-file-name source-file))
         (build-root (beam-get-build-root))
         (relative-path (if build-root
                            (beam-strip-prefix build-root full-name)
                          (beam-flatten-filename filename-string))))

    (concat (beam-get-error-dir)
            "/"
            relative-path
            beam-error-suffix)))


(defun beam-strip-prefix (prefix name)
  "Returns the name with the prefix removed, if the name starts
with the prefix. Returns the name unchanged if the prefix was not
found."
  (let* ((plen (length prefix))
         (nlen (length name))
         (mlen (min plen nlen)))
    (if (string-equal prefix (substring name 0 mlen))
        (substring name mlen)
      name)))


(defun beam-resolve-file-name (name)
  "Resolves the relative file name by replacing $ROOTn with
the value of BEAM_ROOTn from the top of the complaints, or
by replacing relative file names with absolute file names
using the primary BEAM_ROOT.

This function also ensures the file exists - if it can not
be found, the user is prompted for a valid replacement file
name."
  (let ((rel-name nil)
	(root-dir nil))
    (cond
     ((string-match "^/" name) ;; Absolute
      (progn
	(setq rel-name (beam-flatten-filename name))
	(setq root-dir (beam-dirname name))))

     ((string-match beam-root-file-regexp name) ;; Refers to $ROOTn
      (progn
	(let ((number (beam-regexp-group beam-root-file-root-index name)))
	  (setq rel-name (beam-regexp-group beam-root-file-file-index name))
	  (setq root-dir (beam-get-root-dir number)))))

     (t ;; Any relative path
      (progn
	(setq rel-name name)
	(setq root-dir (beam-get-root-dir)))))

    ;; Create a full file name from root-dir and rel-name and locate it
    (let ((full-name (concat root-dir "/" rel-name)))

      ;; Ensure this file exists, or prompt for a replacement
      ;; return:
      (beam-ensure-file-exists full-name rel-name root-dir))))


(defun beam-get-current-error ()
  "Returns the buffer-local current error in the error-list"
  (save-excursion
    (set-buffer (beam-get-buffer))
    (if (equal nil beam-error-list-index)
	(error (concat "Not currently looking at an error."
		       " Try (beam-next-error) or (beam-next-line-error)")))
    (nth beam-error-list-index beam-error-list)))


(defun beam-get-current-line-error ()
  "Returns the buffer-local current line-error in the line-list"
  (save-excursion
    (set-buffer (beam-get-buffer))
    (if (equal nil beam-line-list-index)
	(error (concat "Not currently looking at an error."
		       " Try (beam-next-error) or (beam-next-line-error)")))
    (nth beam-line-list-index beam-line-list)))


(defun beam-get-first-line-error ()
  "Returns the first line error in the current beam error"
  (save-excursion
    (set-buffer (beam-get-buffer))
    (if (equal nil beam-line-list-index)
	(error (concat "Not currently looking at an error."
		       " Try (beam-next-error) or (beam-next-line-error)")))
    (let* ((error-list-index
	    (beam-get-line-error-data beam-line-error-list-index))
	   (error-list (nth error-list-index beam-error-list))
	   (first-line-list-index 
	    (beam-get-error-data beam-error-line-list-index error-list)))

      (nth first-line-list-index beam-line-list))))


(defun beam-get-error-data (piece &optional error-list)
  "Indexes the error-list"
  (if (equal nil error-list)
      (setq error-list (beam-get-current-error)))
  (nth piece error-list))


(defun beam-get-line-error-data (piece &optional line-list)
  "Indexes the line-list"
  (if (equal nil line-list)
      (setq line-list (beam-get-current-line-error)))
  (nth piece line-list))


(defun beam-goto-current-beam-error ()
  "Finds the current beam error being looked at, and moves to it."
  (if (not (equal (current-buffer) (beam-get-buffer t)))
      (error (concat "[Internal error]: not in beam buffer"
		     " [beam-goto-current-beam-error]")))
  (goto-char (marker-position (beam-get-error-data beam-error-marker-index))))


(defun beam-open-append-write-close (filename text)
  "Opens a file in a temp buffer, writes text on the last line, and saves
and closes the buffer. Backups of the file (~ files) are not made."
  (save-excursion
    (and (file-exists-p filename)
	 (not (file-readable-p filename))
	 (error (concat "Can not read innocents file " filename)))
    (and (not (file-writable-p filename))
	 (error (concat "Can not write innocents file " filename)))
    (let ((buffer (create-file-buffer filename))
          (should-change-modes t))
      (set-buffer buffer)

      ; No backups for this buffer
      (set (make-local-variable 'backup-inhibited) t)

      (if (file-exists-p filename)
          (progn
            (insert-file-contents filename)
            (setq should-change-modes nil)))
      (goto-char (point-max))
      (insert text)
      (insert "\n")
      (write-file filename)

      ; 438 in dec is 666 in oct
      (if should-change-modes (set-file-modes filename 438))

      (kill-buffer buffer)))
  (message (concat "Appended information to file " filename)))


(defun beam-mark-innocent-error ()
  "Marks the current error as innocent by writing the innocent file"
  (let ((temp-data beam-comment-data))
    (setq beam-comment-data nil)

    (let ((filename-string 
	   (beam-get-line-error-data beam-line-file-index
				     (beam-get-first-line-error)))
	  (hash-string (beam-get-error-data beam-error-hash-index))
	  filename)

      (if (equal nil filename-string)
	  (error (concat "Beam log format error: Could not find filename"
			 " of error above current point")))
      (if (equal nil hash-string)
	  (error (concat "Beam log format error: Could not find error hash"
			 " above current point")))
      (setq filename (concat (beam-get-innocent-dir) "/" 
			     (beam-flatten-filename filename-string)
			     beam-innocent-suffix))

      ; If this was called from a comment buffer, there should be data here
      ; If so, we'll append the comment to the hash string 
      (if (and temp-data (nth 4 temp-data))
	  (setq hash-string
		(concat hash-string
			"\n" "/*" "\n"
			(nth 4 temp-data)
			"\n" "*/" "\n")))
      
      (beam-open-append-write-close filename hash-string)
      (beam-change-error-color 'beam-innocent-face))))


(defun beam-get-innocent-comment (argp)
  "Opens up a temporary buffer and takes in a comment.
The comment will be written to the innocent file."
  (let ((cur (current-buffer))
	(new (get-buffer-create " *Beam Innocent Log* "))
	(config (current-window-configuration))
	(error (beam-get-error-data beam-error-error-index)))

    ; Switch to the new buffer, set a local key to complete the task
    (switch-to-buffer-other-window new)
    (auto-fill-mode 1)

    (setq beam-comment-data (list cur new config error nil argp))
    
    ; When they hit C-c C-c, we'll check to make sure things are ok,
    ; and we'll call mark-innocent
    (local-set-key "\C-c\C-c"
		   (function
		    (lambda () (interactive)
		      (and (equal nil beam-comment-data)
			   (error (concat "You can not return to this buffer"
					  " after doing other things."
					  " Kill it and try again.")))
		      (let ((continue t)
			    (cur (nth 0 beam-comment-data))
			    (new (nth 1 beam-comment-data))
			    (config (nth 2 beam-comment-data))
			    (error (nth 3 beam-comment-data))
			    (text nil)
			    (argp   (nth 5 beam-comment-data)))

			(save-excursion
			  (goto-char (point-min))
			  
			  ;; Check for an empty buffer - no comment
			  (if (re-search-forward "[^ \t\n]" nil t)
			      (setq text 
				    (buffer-substring (point-min) (point-max))))

			(setq beam-comment-data
			      (list cur new config error text argp))
			
			;; If they changed the current error since this
			;; function was created, warn them that they are
			;; about to add this comment to a different error
		      
			(if (not (equal error
					(beam-get-error-data 
					 beam-error-error-index)))
			    (setq continue
				  (yes-or-no-p
				   (concat
				    "You are looking at a different error."
				    " Mark this new error as innocent with"
				    " this comment? "))))
			
			;; Try it here, so if it errors, we don't
			;; lose the buffer
			(if continue 
			    (progn
			      (beam-mark-innocent-error)
			      (kill-buffer new)
			      (set-window-configuration config)
			      (if cur (switch-to-buffer cur))
			      (beam-next-error argp))))))))

    (message "Enter a comment. C-c C-c to finish.")))


(defun beam-mark-innocent-source ()
  "Writes the current error's innocent comment string into the source file"
  (let ((source-pos (beam-get-line-error-data beam-line-marker-index 
					      (beam-get-first-line-error)))
	(source-buffer (beam-get-source-buffer)))

    (if (equal nil source-buffer)
	(error "No source file to mark. Try beam-next-error."))
    (if (or (equal nil source-pos)
	    (equal nil (marker-position source-pos)))
	(error "Source position is missing. Try beam-next-error."))
    (save-excursion
      (set-buffer source-buffer)
      (goto-char (marker-position source-pos))
      (end-of-line)
      (insert (concat " " (beam-get-error-data beam-error-comment-index))))
    (beam-change-error-color 'beam-innocent-face)
    (message "Comment added to source. Changes need saving.")))
      

(defun beam-flatten-filename (filename)
  "This removes any path leading up to the filename.
Ex: here/there/file.t => file.t"
  (if (string-match ".*/\\([^/]+\\)" filename)
      (beam-regexp-group 1 filename)
    filename))


(defun beam-dirname (filename)
  "Returns the directory that the file exists in.
If the file has no directory, returns the empty string"
  (if (string-match "^\\(.*\\)/[^/]*$" filename)
      (beam-regexp-group 1 filename)
    ""))


(defun beam-find-and-execute-color-string ()
  "Finds the error file, locates this error's color lisp expression,
and executes it. Supports multiple color expressions and only uses
the ones that have file names that match the buffer name"
  (let ((last-buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer last-buffer)

      ;; Nothing to do unless we're on an error
      (if beam-error-list-index
	  (let ((filename-string (beam-get-line-error-data
				  beam-line-file-index
				  (beam-get-first-line-error)))
		(hash-string     (beam-get-error-data beam-error-hash-index))
		filename)
	    (if (equal nil filename-string)
		(error (concat "Beam log format error: Could not find filename"
			       " above current point")))
	    (if (equal nil hash-string)
		(error (concat "Beam log format error: Could not find the hash"
			       " above current point")))
	
	    ;; Filename needs to be located
	    (setq filename (beam-get-error-file filename-string))
	
	    (cond
	     ((not (file-exists-p filename))
	      (message (concat "Color information not available:"
			       " Error log not found: " filename)))
	     ((not (file-readable-p filename))
	      (message (concat "Color information not available:"
			       " Can't read error log: " filename)))
	     (t
	      (let ((buffer (create-file-buffer filename))
		    search-pos search-begin search-end
		    file-name interval)
		(set-buffer buffer)
		(insert-file-contents filename)
		(goto-char (point-min))

		;; Find the beginning and end of the section to search

		(setq search-begin
		      (re-search-forward (concat ">>" hash-string) nil t))
		(if (equal nil search-begin)
		    (message "Current error has no path information")
		  ; else
		  (goto-char search-begin)

		  (setq search-end
			(re-search-forward "^-- " nil t))
		  (if (equal nil search-end) (setq search-end (point-max)))
		  (goto-char search-begin)

		  ;; Loop through that section looking for intervals

		  (while (re-search-forward beam-intervals-regexp search-end t)
		    (setq file-name
			  (beam-regexp-group beam-intervals-file-index))

		    ;; If we have a buffer open with this file name, execute
		    ;; the color string with that buffer as the current buffer

		    (setq beam-interval-buffer
			  (beam-get-file-buffer file-name t))

		    (if beam-interval-buffer
			(eval-region
			 (match-beginning beam-intervals-intervals-index)
			 (match-end beam-intervals-intervals-index))))

		  (kill-buffer buffer))))))))))

; This is called by eval-region on the color code in the error output
; As of version 3.0, we ignore the color argument and always use the face
; As of version 3.2.1, we use the buffer that was set up in
; beam-find-and-execute-color-string

(defun beam-color-intervals (intervals &optional color)
  "Colors each interval in list intervals, where an interval is
'(line &optional start-col end-col)"

  (if (equal nil beam-interval-buffer)
      (error "[Internal error]: Coloring with no interval-buffer"))
  (while intervals
    (setq beam-path-color-list
	  (append beam-path-color-list
		  (list (beam-color-interval beam-interval-buffer 
					     (car intervals)
					     'beam-path-face 
					     250))))
    (setq intervals (cdr intervals))))


(defun beam-color-interval (buffer interval color &optional priority)
  "Colors interval '(line &optional start-col end-col) in the buffer.
Returns the color handle."
  (let ((line-no (car interval))
	(cols    (cdr interval))
	col-start col-end
	line-start line-end)
    (save-excursion
      (set-buffer buffer)
      (goto-line line-no)
      (beginning-of-line)
      (setq line-start (point))
      (end-of-line)
      (setq line-end (point)))
    (if cols
	  (progn
	    (setq col-start (+ line-start (car cols)))
	    (setq col-end (+ line-start (cadr cols)))
	    (if (< col-end line-end) (setq line-end col-end))
	    (if (> col-start line-start) (setq line-start col-start))))
    (if (<= line-start line-end)
	(save-excursion
	  (set-buffer buffer)
	  (beam-color-region line-start line-end color priority))
      (error "Interval to color is invalid: start is greater than end."))))


(defun beam-clear-colors (color-list)
  "Clears the coloring by deleting the color handles in the list"
  (let ((list color-list))
    (while list
      (beam-delete-color-handle (car list))
      (setq list (cdr list)))))


(defun beam-clear-buffer-colors ()
  "Walks the beam buffer and deletes all the color handles"
  (let ((beam-buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer beam-buffer)
      (let ((error-list beam-error-list))
	(while error-list
	  (beam-delete-color-handle
	   (beam-get-error-data beam-error-extent-index (car error-list)))
	  (setq error-list (cdr error-list))))
      (let ((error-list beam-line-list))
	(while error-list
	  (beam-delete-color-handle
	   (beam-get-line-error-data beam-line-extent-index (car error-list)))
	  (setq error-list (cdr error-list)))))))


(defun beam-color-region (start end color &optional priority)
  "Colors the region given by start and end in the current buffer,
and returns a handle useful for deleting it"
  (cond
   ((and (fboundp 'make-extent) (fboundp 'set-extent-property)) ; XEmacs
    (let ((extent (make-extent start end)))
      (if color (set-extent-property extent 'face color))
      (set-extent-property extent 'priority (or priority 200))
      extent))
    ((and (fboundp 'make-overlay) (fboundp 'overlay-put)) ; Emacs
     (let ((overlay (make-overlay start end)))
       (if color (overlay-put overlay 'face color))
       (overlay-put overlay 'priority (or priority 200))
       overlay))
    (t (error (concat "Can not color any paths; Neither 'make-extent' nor"
		      " 'make-overlay' exist in this emacs.")))))


(defun beam-change-color (handle new-color)
  "Changes the color of a handle"
  (cond
   ((and (fboundp 'set-extent-property) (fboundp 'extentp) (extentp handle))
    ; XEmacs
    (set-extent-property handle 'face new-color))
   ((and (fboundp 'overlay-put) (fboundp 'overlayp) (overlayp handle))
    ; Emacs
    (overlay-put handle 'face new-color))
   (t (error (concat "Can not change colors; Neither 'set-extent-property'"
		     " nor 'overlay-put' exist in this emacs.")))))


(defun beam-set-handle-function (handle function)
  "Makes a color handle so that if it's activated (middle mouse click or
enter pressed) it calls this function."
  (let ((local-map (make-keymap)))
    (define-key local-map [return] function)
    (define-key local-map [button2] (lambda () (interactive) nil)) ; XEmacs
    (define-key local-map [button2up] function) ; XEmacs
    (define-key local-map [mouse-2] function) ; Emacs
    (cond
     ((fboundp 'set-extent-property) ; XEmacs
      (progn
	(set-extent-property handle 'mouse-face 'beam-hover-face)
	(set-extent-property handle 'keymap local-map)))
     ((fboundp 'overlay-put) ; Emacs
      (progn
	(overlay-put handle 'mouse-face 'beam-hover-face)
	(overlay-put handle 'local-map local-map)))
     (t (error (concat "Can not set up any handle functions; Neither"
		       " 'set-extent-property' nor 'overlay-put' exists"
		       " in this emacs."))))))


(defun beam-change-error-color (new-color)
  "Changes the error color of the current error both visibly and in the
stored copy, so it will also be redisplayed in the new color"
  (let ((handle (beam-get-error-data beam-error-extent-index)))
    ; Change the visible color by modifying the extent
    (beam-change-color handle new-color)

    ; Change the stored color so that the next time this happens,
    ;  the color is still right
    (beam-list-replace-item (beam-get-current-error)
			    beam-error-color-index 
			    new-color)))


(defun beam-delete-color-handle (handle)
  "Removes the coloring on a handle that was returned by beam-color-region"
  (cond
   ((and (fboundp 'extentp)
	 (fboundp 'delete-extent) (extentp handle)) ; XEmacs
    (delete-extent handle))

   ((and (fboundp 'overlayp)
	 (fboundp 'delete-overlay) (overlayp handle)) ; Emacs
    (delete-overlay handle))

   (t (error (concat "Can not erase any paths; Neither 'delete-extent'"
		     " nor 'delete-overlay' exist in this emacs.")))))


(defun beam-ensure-file-exists (file prompt &optional starting-dir)
  "Returns 'file' if it exists, or the result of prompting the user
if it doesn't. If 'starting-dir' is given, the prompt will start in
that directory."

  (if (not (file-exists-p file))
      (progn
	(message (concat "File not found: " file))
	(sit-for 1.5)
	(setq file (expand-file-name
		    (read-file-name 
		     (concat "Looking for '" prompt "': ")
		     starting-dir
		     nil t)))))
  ; return
  file)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun beam-error-parse-buffer ()
  "Sets up the beam buffer, making it the target for beam commands."

  ; We need to walk through the beam buffer and set up the
  ; error-list and the line-list

  (let ((beam-buffer (beam-get-buffer))
	root-dir)
    (save-excursion
      (set-buffer beam-buffer)
      (goto-char (point-min))

      ; Make sure this looks like beam output
      (setq root-dir
	    (re-search-forward (concat beam-root-dir-regexp-pre
				       beam-root-dir-regexp-post) nil t))

      (if (equal nil root-dir)
	  (progn
	    (setq beam-global-buffer nil)
	    (error (concat "This does not appear to be beam output"
			   " (Could not find 'BEAM_ROOT=...')"))))

      ; Set up the buffer
      (setq mode-name "Beam Log")

      (set (make-local-variable 'beam-error-list) nil)
      (set (make-local-variable 'beam-error-list-index) nil)
      (set (make-local-variable 'beam-line-list) nil)
      (set (make-local-variable 'beam-line-list-index) nil)

      ; Search through for beam errors, and set up the error lists
      ; The scanning is done like this:
      ;  First, beam errors with hash strings are found
      ;  For each error, we find the next "\n\n" which should be after all
      ;    of the lines that go with this error. Then, we find each of the
      ;    lines between the error and the "\n\n", and we know they all
      ;    belong to this error.

      (message "Parsing beam buffer...")

      ; For each outer "big" error:
      ;  Make a new item and insert it into the beam-error-list
      ;  Find "\n\n" or end-of-buffer
      ;  For each innter "line" error until then:
      ;    Extract the line info from the regexp
      ;    Make an extent in the beam error buffer
      ;      with mouse-over to activate it
      ;    Make a new item and insert it into the beam-line-list

      (save-excursion
	(goto-char (point-min))
	(let ((next-error (re-search-forward beam-error-regexp nil t))
	      (end-of-next-error nil)
	      (next-line-error nil)
	      (error-index -1)
	      (line-error-index -1))
	  (while next-error
	    (setq error-index (+ 1 error-index))

	    ; Extract the info and place it in the beam-error-list
	    (let ((hash-string
		   (beam-regexp-group beam-error-regexp-hash))
		  (comment-string
		   (beam-regexp-group beam-error-regexp-comment))
		  (error-string
		   (beam-regexp-group beam-error-regexp-error))
		  (buffer-pos 
		   (save-excursion 
		     (goto-char (match-beginning 0)) (point-marker)))
		  (color 'beam-error-face)
		  color-handle)

	      ; Make a color handle in the beam buffer that contains this error
	      (setq color-handle (beam-color-interval
				  beam-buffer
				  (list (line-number 
					 (marker-position buffer-pos)))
				  color))

	      ; Add this to the list
	      (setq beam-error-list
		    (append beam-error-list (list (list (+ 1 line-error-index)
							buffer-pos
							color-handle
							color
							hash-string
							comment-string
							error-string)))))

	    ; Find all "line errors" before the next double-newline
	    (save-excursion 
	      (setq end-of-next-error (or (re-search-forward "\n\n" nil t) 
					  (point-max))))
	    (setq next-line-error
		  (re-search-forward beam-line-regexp end-of-next-error t))

	    ; Assert that there is at least one line error for this beam error
	    (if (equal nil next-line-error)
		(progn
		  (beam-unparse-buffer)
		  (error "Beam log format error: BEAM error had no lines")))

	    (while next-line-error
	      (setq line-error-index (+ 1 line-error-index))

	      ; Set up this line error with an extent, a mouse-over, etc.
	      (let ((file (beam-regexp-group beam-line-file-index))
		    (line (beam-regexp-group beam-line-line-index))
		    (buffer-pos
		     (save-excursion 
		       (goto-char (match-beginning 0)) (point-marker)))
		    color-handle
		    color-handle-function)

		; Make a "clear" color so we can add mouse-over to it
		(setq color-handle
		      (beam-color-interval
		       beam-buffer
		       (list (line-number (marker-position buffer-pos)))
		       nil))
	      
		; Here we 'fake' a closure of line-error-index
                ; by eval-ing it right into the lambda-expression
		; This only works because it is a constant
		(setq color-handle-function
		      (eval
		       `(lambda ()
			  "Called when the color handle is activated"
			  (interactive)
			  (beam-display-line-error ,line-error-index))))

	      ; Set up a function to run for activation of that handle
	      (beam-set-handle-function color-handle color-handle-function)

	      ; Store this in the line-list
	      (setq beam-line-list
		    (append beam-line-list 
			    (list (list error-index file
					line nil
					color-handle buffer-pos))))

	      ; Find the next line-error
	      (setq next-line-error 
		    (re-search-forward beam-line-regexp end-of-next-error t))))

	    ; Find the next beam-error
	    (setq next-error (re-search-forward beam-error-regexp nil t)))))

      (message (concat "Parsing beam buffer...done!   "
		       (int-to-string (length beam-error-list))
		       " beam error(s) found." )))))


(defun beam-error-goto-next-error (&optional num)
  "Moves forwards or backwards in the beam buffer, bringing up the error
in the source file"

  ; Traverses to the next beam error

  (let ((beam-buffer (beam-get-buffer))
	error-list-index)
    (save-excursion
      (set-buffer beam-buffer)
      (setq error-list-index (beam-list-traverse-n beam-error-list
						   beam-error-list-index
						   num)))
      
    ; Outside save-excursion
    (beam-display-error error-list-index)))


(defun beam-line-error-goto-next-error (&optional num)
  "Moves forwards or backwards in the beam buffer, bringing up the error
in the source file"

  ; Different from the above in that this one traverses to the next line error

  (let ((beam-buffer (beam-get-buffer))
	line-list-index)
    (save-excursion
      (set-buffer beam-buffer)
      (setq line-list-index (beam-list-traverse-n beam-line-list
						  beam-line-list-index
						  num)))
      
    ; Outside save-excursion
    (beam-display-line-error line-list-index)))


(defun beam-list-traverse-n (error-list current num)
  "Walks the list n spaces, returning the new list index. If the end of the list
is reached, returns nil"
  
  (let ((error-list-len (length error-list))
	(error-list-index))

    (setq error-list-index current)
    
    (if (and error-list-index
	     (>= error-list-index error-list-len))
	(error "[Internal error]: Error list is shorter than the current pos!"))
    (if (equal nil error-list) 
	(error "There are no errors in the beam buffer"))
    
    (cond
     ((> 0 num) ; Moving backwards
      (cond
       ((equal nil error-list-index) ; Not on an error yet...
	(setq error-list-index (+ error-list-len num)))
       ((equal 0 error-list-index)   ; At beginning of list...
	(setq error-list-index nil))
       ((> (- num) error-list-index) ; Moving too far...
	(progn
	  (setq error-list-index 0)
	  (message 
	   "Moving back past first error; Resetting to first error")))
       (t ; Normal motion
	(setq error-list-index (+ error-list-index num)))))
     
     ((< 0 num) ; Moving forwards
      (cond
       ((equal nil error-list-index) ; Not on an error yet...
	(setq error-list-index (- num 1)))
       ((equal (- error-list-len 1) error-list-index) ; At end of list...
	(setq error-list-index nil))
       ((>= (+ num error-list-index) error-list-len)  ; Moving too far...
	(progn
	  (setq error-list-index (- error-list-len 1))
	  (message
	   "Moving forward past last error; Resetting to last error")))
       (t ; Normal motion
	(setq error-list-index (+ error-list-index num))))))

    error-list-index))


(defun beam-display-error (index)
  "Displays the beam error by showing the first line error within"

  (let ((beam-buffer (beam-get-buffer))
	(error-list))
    (save-excursion
      (set-buffer beam-buffer)
      (setq error-list beam-error-list))

    ;; Outside save-excursion...
    (if index
	(beam-display-line-error
	 (beam-get-error-data beam-error-line-list-index
			      (nth index error-list)))
      ;; else
      (beam-display-line-error nil))))


(defun beam-display-line-error (index)
  "Moves the beam buffer to the error, opens the source file, and sets
up the colors"

  ; Setting up the error involves:
  ; - If this error has a mark, use it
  ; - Otherwise, create one for each error in this source file
  ; - Set up the appropriate colors
  ; - Call redisplay

  (let ((beam-buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer beam-buffer)

      ; Clear previous colors (done twice in some cases, no big deal)
      (beam-clear-colors beam-error-color-list)
      (setq beam-error-color-list nil)

      ; Clear this here, add it in at the end if it's enabled
      (beam-clear-colors beam-path-color-list)
      (setq beam-path-color-list nil)

      ; Advance the list index
      (setq beam-line-list-index index)

      (if beam-line-list-index
	  (progn
	    ;; Since we have an index to go to, lets load things up
	    (setq beam-error-list-index
		  (beam-get-line-error-data beam-line-error-list-index))

	    ;; Load up all the source files under this error
	    (beam-load-source-files beam-error-list-index)

	    ;; Set up beam-source-buffer
	    (let ((file-name (beam-get-line-error-data beam-line-file-index)))
	      (setq beam-source-buffer (beam-get-file-buffer file-name)))
			       
	    ;; Set up the visited color unless this one is marked innocent
	    ;; (also changing the color in the item)
	    (if (not (equal (beam-get-error-data beam-error-color-index)
			    'beam-innocent-face))
		(beam-change-error-color 'beam-visited-error-face))

	    ;; Set up the beam buffer highlight color so it 
	    ;; marks this error as current
	    (setq beam-error-color-list
		  (list (beam-color-interval
			 beam-buffer
			 (list (line-number 
				(marker-position
				 (beam-get-line-error-data 
				  beam-line-beam-marker-index))))
			 'beam-highlight-face)))

	    ;; Color the current source position too
	    (let ((marker (beam-get-line-error-data beam-line-marker-index)))
	      (save-excursion
		(set-buffer beam-source-buffer)
		(setq beam-error-color-list
		      (append beam-error-color-list
			      (list (beam-color-interval 
				     beam-source-buffer
				     (list (line-number 
					    (marker-position marker)))
				     'beam-highlight-face 350))))))

	    ;; If path coloring is on, color the current path
	    (if beam-path-color-enabled (beam-find-and-execute-color-string)))

	;; Else, if we have no index to go to, reset some other things
	(progn
	  (setq beam-error-list-index nil)
	  (setq beam-source-buffer nil))))

    ; No longer in save-excursion...

    (beam-redisplay)))


(defun beam-redisplay (&optional silent)
  "Sets up the right number of windows with the right buffers, and scrolls
them all to the right lines"

  (let ((beam-buffer (beam-get-buffer))
	(source-buffer (beam-get-source-buffer))
	(help-buffer (beam-get-help-buffer))
	(error-text nil)
	(error-index nil)
	(num-errors nil)
	(buffers-to-display nil)
	(beam-window-pos 0)
	(beam-window-char 0)
	(last-window))

    ;; If we aren't looking at an error, we pass nil to the help routines
    (setq error-index (save-excursion (set-buffer beam-buffer)
				      beam-error-list-index))
    (if error-index
	(setq error-text (beam-get-error-data beam-error-error-index)))

    (setq num-errors (save-excursion (set-buffer beam-buffer)
				     (length beam-error-list)))

    ;; Set up which buffers get displayed:
    ;; - We always show the beam buffer
    (setq buffers-to-display (list beam-buffer))
    ;; - If we are looking at an error, we show the source buffer
    (if source-buffer
	(setq buffers-to-display
	      (append buffers-to-display (list source-buffer))))
    ;; - If we have extended help open, we show that too
    (if help-buffer
	(setq buffers-to-display
	      (append buffers-to-display (list help-buffer))))
    
    ;; Display the buffers
    (beam-setup-windows buffers-to-display)

    ;; Save off which window is selected
    (setq last-window (selected-window))

    ;; Update the error window if it's showing
    (if help-buffer
	(beam-display-extended-help error-text help-buffer))

    ;; Scroll the help buffer
    (if help-buffer
	(progn
	  (select-window (get-buffer-window help-buffer))
	  (goto-char (point-min))))

    ;; Scroll the beam-buffer
    (if error-index
	;; If we are looking at an error, scroll to it
	(progn
	  (setq beam-window-pos
		(marker-position (beam-get-error-data
				  beam-error-marker-index)))
	  (setq beam-window-char
		(marker-position (beam-get-line-error-data
				  beam-line-beam-marker-index)))))

    (progn
      (select-window (get-buffer-window beam-buffer))
      (goto-char beam-window-char)
      (set-window-start (selected-window) beam-window-pos))

    ;; Scroll the source file buffer
    (if source-buffer
	(progn
	  (select-window (get-buffer-window source-buffer))
	  (goto-char
	   (marker-position (beam-get-line-error-data
			     beam-line-marker-index)))))

    ;; Re-select the previously selected window
    (select-window last-window)

    ;; Display a message about which error we are on
    (if error-index
	(message (format "Currently on `%s' (%d of %d)"
			 error-text
			 (+ 1 error-index)
			 num-errors))
      ;; else, unless silent, display this message when not on any errors
      (if (not silent)
	  (message "Not currently on any errors. C-c b n for next error")))))


(defun beam-get-file-buffer (file-name &optional return-nil-if-killed)
  "Returns the buffer visiting the source for file-name, or opens a new one.
If a new one is opened, all marks will be set from the line-list. Normally,
if the buffer was killed, it is re-opened. Use return-nil-if-killed to change
the behavior."

  (let ((buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer buffer)

      ;; Initialize a hash if needed
      (if (equal nil beam-file-buffer-hash)
	  (setq beam-file-buffer-hash (make-hash-table :test 'equal)))

      ;; Look up the file name in the hash
      (let ((name file-name)
	    (file-buffer (gethash file-name beam-file-buffer-hash)))

	(if (and file-buffer (buffer-live-p file-buffer))
	    file-buffer
	  ; else
	  (if (and file-buffer return-nil-if-killed)
	      nil
	    ; else
	    (progn
	      ;; File was not loaded, or the buffer was killed
	      (puthash file-name nil beam-file-buffer-hash)

	      ;; Take the file name, and resolve it based on the BEAM
	      ;; root directories. Ensures it exists.
	      (setq name (beam-resolve-file-name name))

	      (let ((file-buffer (find-file-noselect name)))

		;; Save in the hash
		(puthash file-name file-buffer beam-file-buffer-hash)

		;; Set all marks for the new buffer
		(beam-place-markers file-buffer file-name)

		;; Return it
		file-buffer))))))))
		  
	      
(defun beam-load-source-files (error-list-index)
  "Load all files under the given error-list-index that aren't already loaded.
If loading the file from disk, set all markers from the line list that are in
the file."

  ;; Starting at the first line error under this error-list-index,
  ;; walk the line errors that are also under this error-list-index

  (let ((buffer (beam-get-buffer)))
    (save-excursion
      (set-buffer buffer)

      (let* ((error-list (nth error-list-index beam-error-list))
	     (line-list-index
	      (beam-get-error-data beam-error-line-list-index error-list))
	     (line-list (nth line-list-index beam-line-list)))

	(while (and line-list (equal error-list-index
				     (beam-get-line-error-data
				      beam-line-error-list-index
				      line-list)))

	  ;; This line error is from the same beam error. Load it.
	  (beam-get-file-buffer (beam-get-line-error-data
				  beam-line-file-index
				  line-list))
		  
	  ;; Advance line-list
	  (setq line-list-index (+ 1 line-list-index))
	  (setq line-list (nth line-list-index beam-line-list)))))))


(defun beam-place-markers (buffer file-name)
  "This places a marker in the buffer for each error in beam-error-list
matching file-name, and updates beam-error-list"
  (save-excursion
    (set-buffer (beam-get-buffer))

    ; If any items in the error list are in this file name, make a marker for it
    (let ((error-list beam-line-list))
      (set-buffer buffer)
      (while error-list
	(if (equal file-name
		   (beam-get-line-error-data beam-line-file-index 
					     (car error-list)))
	    ; Make a new marker and set it
	    (progn
	      (goto-line (string-to-int 
			  (beam-get-line-error-data beam-line-line-index 
						    (car error-list))))
	      (beam-list-replace-item (car error-list) 
				      beam-line-marker-index
				      (point-marker))))
	(setq error-list (cdr error-list))))))


(defun beam-list-replace-item (error-list error-index new-item)
  "Replaces the item at index in the error-list with the new-item.
This is destructive (it uses setcar)"
  (if (>= error-index (length error-list))
      (error (concat "[Internal error]: beam-error-replace-item: "
		     "index is outside list bounds")))

  (let ((target-cell (nthcdr error-index error-list)))
    (setcar target-cell new-item))

  error-list)


(defun beam-setup-windows (buffers)
  "Splits the screen into enough windows to hold all of the
buffers passed in, and sets them up top-down, leaving the
already-visible ones alone"
  
  ;; This is just to make sure we got at least one argument
  (if (equal 0 (length buffers))
      (error "[Internal error]: beam-setup-windows: zero-length buffer list!"))

  (let ((raw-placement nil)) ;; Will be 't' unless there are
                             ;; already enough windows

    ;; Calculate a few things and set up the windows
    (let ((window-count (length buffers))
	  percentage-per-window
	  total-window-lines
	  window-lines-per-window)
      
      ;; If we need to set up the windows...
      (if (not (equal window-count (count-windows)))
	  (progn
	    (delete-other-windows)
	    
	    ;; This means that we want the windows placed top-down,
	    ;; disregarding if they are already shown.
	    ;; If there already were the right number of windows,
	    ;; we never set this, and the ones that are visible stay put.
	    ;; In our case, we don't want that to happen, since we already
	    ;; re-arranged the windows anyhow.
	    (setq raw-placement t)

	    ;; Calculate the height per-window to make them all equal
	    (setq percentage-per-window (/ 1.0 (float window-count)))
	    (setq total-window-lines (window-height))
	    (setq window-lines-per-window 
		  (truncate (* percentage-per-window
			       (float total-window-lines))))

	    ;; Set up the windows, giving each new one
	    ;; window-lines-per-window lines
	    (while (not (equal (count-windows) window-count))
	      (split-window nil window-lines-per-window)
	      (other-window 1)))))

    ;; Now we have one window for each buffer. Decide which buffers
    ;; are not shown, and which windows don't have a buffer in our list,
    ;; and pair them up, top-down.

    ;; Algorithm:
    ;;
    ;;  Initialize a list of buffers-to-be-placed, and start it with 'buffers'
    ;;  Walk the windows, top to bottom.
    ;;  If the current window contains a buffer in buffers-to-be-placed, remove
    ;;   the buffer from buffers-to-be-placed, and move on to the next window.
    ;;  Otherwise, find a buffer suitable for this window:
    ;;   If raw-placement is 't', we just take the first buffer from the list.
    ;;   Otherwise, walk the buffers-to-be-placed list. If the first buffer is
    ;;   not visible in this frame, set it in this window and remove it from
    ;;   buffers-to-be-placed.
    ;;   Otherwise, it is visible, so it will be seen later - try again with the
    ;;   next buffer in the list.
    ;;  There should always be an available buffer in buffers-to-be-placed if
    ;;   we are placing N buffers in N windows, because if one window doesn't
    ;;   have a buffer that's in the list, then there should be at least one
    ;;   buffer in the list that doesn't have any window.

  (let ((buffers-to-be-placed (copy-list buffers)) 
	;;                     ^ copy-list because we use delete
	(current-buffer nil)
	(windows (beam-ordered-window-list))
	(current-window nil))

    (while windows
      (setq current-window (car windows))
      (setq current-buffer (window-buffer current-window))

      (setq windows (cdr windows))

      (if raw-placement
	  ;; In raw-placement mode (we split windows) we just
	  ;; set things up top-down
	  (progn
	    (set-window-buffer current-window (car buffers-to-be-placed))
	    (setq buffers-to-be-placed (cdr buffers-to-be-placed)))

	;; Else, not in raw-placement, we don't disturb existing buffers
	(progn

	  ;; First, see if this one is already taken by something
	  ;; we want displayed

	  (if (member current-buffer buffers-to-be-placed)
	      ;; This window is already set up with something we want to display
	      (setq buffers-to-be-placed
		    (delete current-buffer buffers-to-be-placed))

	    ;; else
	    ;; This window needs a new buffer - find one in
	    ;; the buffers-to-be-placed list
	    (let ((buffer-list (copy-list buffers-to-be-placed)))
	      (while buffer-list
		(setq current-buffer (car buffer-list))
		(setq buffer-list (cdr buffer-list))
	    
		;; If this buffer is visible in this frame already, we skip it.
		;; Otherwise, we set it up in this window and remove it from 
		;; buffers-to-be-placed

		(if (not (get-buffer-window current-buffer))
		    (progn
		      (set-window-buffer current-window current-buffer)
		      (setq buffers-to-be-placed
			    (delete current-buffer buffers-to-be-placed))
		      (setq buffer-list nil))))))))))))


(defun beam-ordered-window-list ()
  "Returns a list of windows like `window-list', but in order from top down"

    (let* ((lis nil)
	   (first-window nil)
	   (config (current-window-configuration)))

      (select-window (frame-first-window nil))
      (setq first-window (selected-window))
      
      (setq lis (list first-window))
      (other-window 1)

      (while (not (equal (selected-window) first-window))
	(setq lis (append lis (list (selected-window))))
	(other-window 1))

      (set-window-configuration config)

      lis))


(defun beam-get-help-file-location ()
  "Attempts to find the help file by using the beam-install-dir
variable, the load-path, and finally, by asking the user.

This function returns the filename of the help file, or 'nil' if
the user cancelled the prompt.
"

  (let ((help-file-loc nil))

    ;; If we have an install dir, return the filename
    ;; from inside there
    (if beam-install-dir
	(cond
	 ((file-exists-p (concat beam-install-dir "/etc/extended-help.txt"))
	  (setq help-file-loc
		(concat beam-install-dir "/etc/extended-help.txt")))
	 ((file-exists-p (concat beam-install-dir "/emacs/extended-help.txt"))
	  (setq help-file-loc
		(concat beam-install-dir "/emacs/extended-help.txt")))))

    ;; If we can find the beam dir from the load-path, return
    ;; the filename from inside there
     
    (if (equal nil help-file-loc)
	(let ((lis load-path))
	  (while lis
	    (if (file-exists-p (concat (car lis) "/extended-help.txt"))
		;; Found
		(progn
		  (setq help-file-loc (file-expand-path 
				       (concat (car lis) "/extended-help.txt")))
		  (setq lis nil)) ; to break
	      ;; else
	      (setq lis (cdr lis))))))
  
    (if (equal nil help-file-loc)
	;; Can't find the beam install dir? let the user know,
	;; and have them set one up manually

	;; Here we save off the window configuration,
	;; then we split the windows into 2 and show the error
	;; text, and prompt for the help file location. After
	;; getting a response, we restore the window configuration.

	(let ((error-buffer (get-buffer-create " *Beam Error* "))
	      (second-window)
	      (window-configuration))

	  (save-excursion
	    ;; Store off the current window configuration
	    (setq window-configuration (current-window-configuration))
	
	    ;; Split into 2 windows
	    (delete-other-windows)
	    (split-window)
	
	    ;; Make the bottom one the error window
	    (setq second-window (cadr (window-list nil 'skip-minibuf
						   (frame-first-window nil))))
	    (set-window-buffer second-window error-buffer)
	
	    ;; Select the new buffer, and fill it in with the error
	    (set-buffer error-buffer)
	    (erase-buffer)
	    (goto-char (point-min))

	    ;; Put an error into the buffer
	    (insert (beam-get-error-text 'beam-cant-find-extended-help-file))
	    (goto-char (point-min))

	    ;; Ask for the new beam install dir
	    (condition-case nil
		(setq help-file-loc
		      (expand-file-name 
		       (read-file-name 
			"Find file: " nil nil t)))
	      (quit))
	
	    ;; If they 'quit' from that prompt, we still want to clean up
	    ;; We will get control back because of the condition-case
	  
	    (set-window-configuration window-configuration)
	    (kill-buffer (get-buffer " *Beam Error* ")))))

    ;; Return help-file-loc
    help-file-loc))
  

(defun beam-parse-extended-help-file (buffer file-name)
  "Parses the buffer as extended help, and returns a hash table
of the errors found.

This function will never return nil. If the buffer does not
look like an extended help file, this function will error
instead.
"
  
  ;; The extended-help.txt file format is as follows:
  ;;
  ;;   <snippet "ERROR1">
  ;;   text
  ;;   text
  ;;   </snippet>
  ;;
  ;;   (anything here is ignored)
  ;;
  ;;   <snippet "ERROR2">
  ;;   text
  ;;   </snippet>
  ;;
  ;; Basically, at every "<snippet>", the text is extracted and
  ;; inserted into the hash, with a key of the snippet name.

  (let ((temp-hash (make-hash-table :test 'equal))
	(snippet-name nil)
	(snippet-text nil)
	(good nil))

    (save-excursion
      (set-buffer buffer)
      (goto-char (point-min))

      (message "Parsing the extended help file...")
      
      (while (re-search-forward beam-snippet-regexp nil t)

	;; The snippet name is the first substring
	(setq snippet-name (beam-regexp-group beam-snippet-name-index))

	;; The snippet text is the second substring
	(setq snippet-text (beam-regexp-group beam-snippet-text-index))

	;; Update the hash
	(puthash snippet-name snippet-text temp-hash)

	;; Set a flag
	(setq good t)))

    ;; Make sure there was at least one error
    (if (not good)
	(error (concat file-name " has no help messages")))

    ;; Return the hash
    temp-hash))


(defun beam-load-extended-help-file ()
  "Locates and loads the beam help file, parses it, and sets up
beam-extended-help-hash.

If 'beam-install-dir' isn't set, it is auto-detected and if all else fails,
the user is prompted for the location of the help file.

If reading the help file fails, this function errors and leaves the
hash variable 'nil' so that we re-enter and try again next time help
is requested.
"

  ;; First, don't do anything unless the hash doesn't exist
  (if (equal nil beam-extended-help-hash)

      (let ((help-file-loc (beam-get-help-file-location))
	    (temp-buffer nil))

	;; We have to find and load the file. If any of the finding
	;; or loading fails, we have to error out. After that, we
	;; parse the file and set its contents into a new hashtable
	;; that we'll assign to the global var 'beam-extended-help-hash'

	(if (equal nil help-file-loc)
	    (error "Can't find extended-help.txt"))

	(if (not (file-readable-p help-file-loc))
	    (error (concat "Can't read " help-file-loc)))

	(setq temp-buffer (find-file-noselect help-file-loc))

	(unwind-protect
	    (setq beam-extended-help-hash (beam-parse-extended-help-file
					   temp-buffer help-file-loc))
	  (kill-buffer temp-buffer)))))


(defun beam-display-extended-help (error-string buffer)
  "Displays the help matching 'error-string' into buffer 'buffer',
clearing 'buffer' first.

If the extended help file hasn't been loaded yet, it is loaded from
disk (from beam-install-dir/etc/extended-help.txt) and cached away.

If the help file doesn't contain a message for the current error,
the problem is displayed in the help buffer instead.
"

  (save-excursion

    ;; Set it up for input
    (set-buffer buffer)
  
    ;; Clear the buffer
    (erase-buffer)

    (if error-string

      ;; If error-string is set, we display the text associated with it
      (progn

	;; If nothing is loaded, this loads it. Otherwise, this does nothing.
	(beam-load-extended-help-file)

	;; Look up the error in the hash
	(if (gethash error-string beam-extended-help-hash)
	    (insert (concat "Extended Help\n=============\n\n"
			    (gethash error-string beam-extended-help-hash)))
	  ;; If it's not in the hash, print an error
	  (insert 
	   (beam-get-error-text 'beam-help-file-not-found error-string))))

      ;; If error-string is not set, we are not looking at an error
      (insert (beam-get-error-text 'beam-not-on-any-errors)))))


(defun beam-get-error-text (error &rest args)
  "Returns the lengthy text to a beam error that will be shown to
the user."

  (cond

   ;; **** beam-cant-find-extended-help-file ****

   ((equal error 'beam-cant-find-extended-help-file)
    (concat  "Error:

I can't find the file \"extended-help.txt\". It should be in the beam
install point as \"etc/extended-help.txt\", but I can't find the beam
install point, either. I tried looking through the load-path of emacs
to see where beam-parse.el was loaded from, but was unsuccessful.
In order for you to use features that require files from the beam
installation (like the extended help files), you will need to load
beam-parse.el in a special way, or set \"beam-install-dir\" in your
init file.

For more information, see:

  http://w3.eda.ibm.com/beam/emacs.html


For now:

Find the file \"extended-help.txt\". It should be in the beam install
point as \"etc/extended-help.txt\". I will use that file for the current
beam session.
"))

   ;; **** beam-help-file-not-found ****

   ((equal error 'beam-help-file-not-found)
    (concat "Error:

Could not find the help text for`" (nth 0 args) "'.

This means that the help file was loaded, but it either
didn't contain text for this error, or it is in a format
I don't understand.

To ensure that I get the correct help file loaded (the one
from the same beam installation that this beam-parse.el is
from), make sure you load beam-parse.el in the correct way.

For more information, see:

  http://w3.eda.ibm.com/beam/emacs.html
"))

   ;; **** beam-not-on-any-errors ****

   ((equal error 'beam-not-on-any-errors)
    (concat "There is no help because you are not currently looking
at any errors. When you move to an error, the help for it will be
shown here.

To look at an error, try:

  C-c b n          (beam-next-error)

Or:

  Middle click on a line in the beam buffer that contains a source
  file and line number (it will highlight when you hover over it)

If you don't want this extended help window open, type:

  C-c b h          (beam-extended-help)
"))


   ;; Nothing matched?

   (t
    (concat "[Internal error]:

An error message was requested (`" (symbol-name error) "') that
I do not have, so I can't display that error.

This is a bug in beam-parse.el and should be reported.
"))))
