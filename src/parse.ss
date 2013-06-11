(import core
  (+ * split first rest cons eq? nil?))

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

(define (parse-integer str value)
  (consume str (lambda (ch s)
    (let ((digit (digit-value ch)))
      (if (nil? digit) (cons value s)
        (parse-integer s (+ (* 10 value) digit)))))))

(define (lookup key map)
  (if (nil? map) (error)
    (if (eq? key (first map)) (first (rest map))
      (lookup key (first (rest (rest map)))))))

(define (parse-string-escaped str)
  (consume str (lambda (ch s)
    (lp (parse-string s) (lambda (val str))
      (cons
        (concat
          (lookup ch (quote ("n" "\n" "\"" "\"" "\\" "\\")))
          val) 
        str)))))

(define (parse-string str)
  (consume str (lambda (ch s))
    (concat
      (if (eq? ch "\"") (cons "" str))
        (if (eq? ch )))))

(define (parse-value str)
  (let ((str (skip-whitespace str)))
    (parse-integer str 0)))

(define (internal-parse str)
  (parse-value str))

(define (parse str)
  (internal-parse str))

(parse-value " \n42 \n")
