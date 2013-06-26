(module
  (import core
    (eq? cons ctor))

  (import lang/transform (transform default-macroexpand))

  (import lang/parse (parse))

  (import lang/prettyprint (tostring))

  (define (nil? a) (eq? (ctor a) 'Nil))

  (define (list . items) items)

  (define (make-failure value expected)
    (list 'failure
      (list 'got value)
      (list 'expected expected)))

  (define (cases first . rest)
    (if (eq? first 'good)
      (if (nil? rest) 'good
        (cases . rest))
      first))

  (define (check-eq value expected)
    (if (eq? value expected) 'good
      (make-failure value expected)))

  (define (test-lambda-macro)
    (cases
      (check-eq (default-macroexpand '(lambda (x) x))
        '(letlambdas (((current-lambda x) x)) current-lambda))
      (check-eq (default-macroexpand '(lambda (x y) (+ x y)))
        '(letlambdas (((current-lambda x y) (+ x y))) current-lambda))))

  (define (test-let-macro)
    (cases
      (check-eq (default-macroexpand '(let ((x 1)) x))
        '(letlambdas (((current-let x) x)) (current-let 1)))
      (check-eq (default-macroexpand '(let ((x 1) (y 2)) (+ x y)))
        '(letlambdas (((current-let x y) (+ x y))) (current-let 1 2)))))

  (define (test-transform)
    (cases
      (test-lambda-macro)
      (test-let-macro)))

  (define (test-parse-ints)
    (cases
      (check-eq (parse "0" #f) 0)
      (check-eq (parse "42" #f) 42)))

  (define (test-parse-syms)
    (cases
      (check-eq (parse "a" #f) 'a)
      (check-eq (parse "abc" #f) 'abc)
      (check-eq (parse "ab/c" #f) 'ab/c)))

  (define (test-parse-dot)
    (cases
      (check-eq (parse "(a . b)" #f) (cons 'a 'b))
      (check-eq (parse "(1 2 . 3)" #f) (cons 1 (cons 2 3)))))

  (define (test-parse)
    (cases
      (test-parse-ints)
      (test-parse-syms)
      (test-parse-dot)))

  (define (test-prettyprint)
    (cases
      (check-eq (tostring 0) "0")
      (check-eq (tostring 42) "42")))

  (define (main)
    (cases
      (test-transform)
      (test-parse)
      (test-prettyprint)))

  (export main)
)