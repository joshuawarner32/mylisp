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
    (check-eq (default-macroexpand '(lambda (x) x))
        '(letlambdas (((current-lambda x) x)) current-lambda)))

  (define (test-let-macro)
    (check-eq (default-macroexpand '(let ((x 1)) x))
        '(letlambdas (((current-let x) x)) (current-let 1))))

  (define (main)
    (cases
      (test-lambda-macro)
      (test-let-macro)))

  (export main)
)