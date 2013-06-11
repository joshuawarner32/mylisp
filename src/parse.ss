(import core +)
(import core *)
(import core split)
(import core first)
(import core rest)
(import core eq?)
(import core nil?)

(define (or a b) (if a #t b))
(define (not a) (if a #f #t))

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

(define (parse-integer str)
  (consume str (lambda (ch s)
    (let ((d (digit-value ch)))
      (if (nil? d) ()
        (let ((val (parse-integer s)))
          (if (nil? val) d
            (+ (* 10 d) val))))))))

(define (parse-value str)
  (let ((str (skip-whitespace str)))
    (let ((int (parse-integer str)))
      (if (not (nil? int)) int
        ()))))

(define (internal-parse str)
  (parse-value str))

(define (parse str)
  (internal-parse str))

(parse-value " \n42 \n")