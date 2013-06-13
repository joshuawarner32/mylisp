(import core
  (+ * concat split first rest cons eq? ctor make-sym))

(define (nil? v) (eq? (ctor v) (quote Nil)))
(define (cons? v) (eq? (ctor v) (quote cons)))
(define (sym? v) (eq? (ctor v) (quote Symbol)))

(define (nil? v) (eq? (ctor v) (quote Nil)))

(define (or a b) (if a #t b))
(define (not a) (if a #f #t))

(define (lp val func)
  (func (first val) (rest val)))

(define (consume str func)
  (let ((parts (split str 1)))
    (let ((ch (first parts))
      (s (first (rest parts))))
    (func ch s))))

(define (skip-whitespace str)
  (consume str (lambda (ch s)
    (if (or (eq? ch " ") (eq? ch "\n"))
      (skip-whitespace s)
      str))))

(define (parse-list str)
  (let ((str (skip-whitespace str)))
    str))

(define (list-index-inner lst val n)
  (if (nil? lst) ()
    (if (eq? (first lst) val) n
      (list-index-inner (rest lst) val (+ n 1)))))

(define (list-index lst val)
  (list-index-inner lst val 0))

(define (list-contains lst val)
  (if (nil? lst) #f
    (if (eq? (first lst) val) #t
      (list-contains (rest lst) val))))

(define (digit-value ch)
  (list-index (quote ("0" "1" "2" "3" "4" "5" "6" "7" "8" "9")) ch))

(define (isletter ch)
  (list-contains
    (quote ("a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k" "l" "m" "n" "o" "p" "q" "r" "s" "t" "u" "v" "w" "x" "y" "z"
      "A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "Y" "Z"
      "?" "+" "-" "*" "/"))
    ch))

(define (parse-integer str value continue)
  (consume str (lambda (ch s)
    (let ((digit (digit-value ch)))
      (if (nil? digit) (continue value str)
        (parse-integer s
          (+ (* 10 value) digit)
          continue))))))

(define (lookup key map)
  (if (nil? map) (error)
    (if (eq? key (first map)) (first (rest map))
      (lookup key (first (rest (rest map)))))))

(define (parse-symbol sym str continue)
  (consume str (lambda (ch s)
    (if (isletter ch)
      (parse-symbol (concat sym ch) s continue)
      (continue (make-sym sym) str)))))

(define (parse-bool str continue)
  (consume str (lambda (ch s)
    (if (eq? ch "t") (continue #t s)
      (if (eq? ch "f") (continue #f s)
        (concat "<parse-bool-error:" ch ">"))))))

(define (parse-string-escaped val str continue)
  (consume str (lambda (ch s)
    (if (eq? ch "\"") (parse-string (concat val "\"") s continue)
      (if (eq? ch "\\") (parse-string (concat val "\"") s continue)
        (if (eq? ch "n") (parse-string (concat val "\n") s continue)
          "<bad-escape>"))))))

(define (parse-string val str continue)
  (consume str (lambda (ch s)
    (if (eq? ch "\\") (parse-string-escaped val s continue)
      (if (eq? ch "\"") (continue val s)
        (parse-string (concat val ch) s continue))))))

(define (parse-list str continue)
  (let ((str (skip-whitespace str)))
    (consume str (lambda (ch s)
      (if (eq? ch ")") (continue () s)
      (parse-value str (lambda (f s)
        (parse-list s (lambda (r s)
          (continue (cons f r) s))))))))))

(define (parse-value str continue)
  (let ((str (skip-whitespace str)))
    (consume str (lambda (ch s)
      (let ((digit (digit-value ch)))
        (if (not (nil? digit))
          (parse-integer str 0 continue)
          (if (eq? ch "(")
            (parse-list s continue)
            (if (isletter ch)
              (parse-symbol ch s continue)
              (if (eq? ch "#")
                (parse-bool s continue)
                (if (eq? ch "\"")
                  (parse-string "" s continue)
                  "<parse-error>"))))))))))

(parse-value " \n((1) (2 #f) 3 a \"this \\\"is test\\n\" b) \n" (lambda (value str)
  value))
