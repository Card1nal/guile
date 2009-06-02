(use-modules (language tree-il))
(let ((source (list-ref (command-line) 1))
      (target (list-ref (command-line) 2)))
  (let ((in (open-input-file source))
        (out (open-output-file (string-append target ".tmp"))))
    (write '(eval-when (compile) (set-current-module (resolve-module '(guile))))
           out)
    (newline out)
    (let loop ((x (read in)))
      (if (eof-object? x)
          (begin
            (close-port out)
            (close-port in))
          (begin
            (write (tree-il->scheme
                    (sc-expand x 'c '(compile load eval)))
                   out)
            (newline out)
            (loop (read in))))))
  (system (format #f "mv -f ~s.tmp ~s" target target)))
