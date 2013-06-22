(module
  (import core
    (eq? cons))

  (import lang/transform (transform default-macroexpand))

  (define (list a b)
    (cons a (cons b ())))

  (define (make-failure value expected)
    (cons 'failure
      (cons (list 'got value)
        (cons (list 'expected expected)
          ()))))

  (define (cases a b)
    (if (eq? a 'good)
      (if (eq? b 'good)
        'good
        b)
      a))

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

  (define (main)
    (cases
      (test-lambda-macro)
      (test-let-macro)))

  (export main)
)