
(import core
  (concat split + - * /
    modulo nil? cons?
    sym? int? str? eq?
    sym-name first rest
    ctor))

(define (list-tostring value indent list-on-newline value-stringer)
  (if (nil? value) ")"
    (if (cons? value)
      (concat " "
        (value-stringer (first value) indent #t)
        (list-tostring (rest value) indent #t value-stringer))
      (concat " . "
        (value-stringer value indent #t)
        ")"))))

(define (n-spaces n)
  (if (eq? n 0) ""
    (concat " " (n-spaces (- n 1)))))

(define (nth lst n)
  (if (eq? n 0)
    (first lst)
    (nth (rest lst) (- n 1))))

(define (str-digit n)
  (nth (quote ("0" "1" "2" "3" "4" "5" "6" "7" "8" "9")) n))

(define (int-to-str-digits i)
  (if (eq? i 0) ""
    (let ((i (/ i 10))
          (r (modulo i 10)))
      (concat (int-to-str-digits i) (str-digit r)))))

(define (int-to-str i)
  (if (eq? i 0) "0" (int-to-str-digits i)))

(define (mapstr ch lst)
  (if (nil? lst) ch
    (if (eq? (first lst) ch)
      (first (rest lst))
      (mapstr ch (rest (rest lst))))))

(define (escape-str-inner s)
  (if (eq? s "") ""
    (let ((parts (split s 1)))
      (let ((ch (first parts))
            (s  (first (rest parts))))
        (concat
          (mapstr ch (quote ("\"" "\\\"" "\n" "\\n" "\\" "\\\\")))
          (escape-str-inner s))))))

(define (escape-str s)
  (concat "\"" (escape-str-inner s) "\""))

(define (bool-to-str s)
  (if s "#t" "#f"))

(define (value-tostring value indent list-on-newline)
  (if (nil? value) "()"
    (if (cons? value)
      (concat
        (if list-on-newline (concat "\n" (n-spaces indent)) "")
        "("
        (value-tostring (first value) (+ indent 2) #f)
        (list-tostring  (rest value)  (+ indent 2) #t value-tostring))
      (if (sym? value)
        (sym-name value)
        (if (int? value)
          (int-to-str value)
          (if (str? value)
            (escape-str value)
            (if (eq? (ctor value) (quote Bool))
              (bool-to-str value)
              "<unknown>")))))))

(define (tostring value)
  (value-tostring value 0 #f))

(define (tostring-indented value indent)
  (value-tostring value indent #f))

tostring-indented
