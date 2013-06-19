(module
  (import core
    (+ cons first rest concat ctor))

  (define (nil? v) (eq? (ctor v) (quote Nil)))

  (define (addone x)
    (+ 1 x))

  (addone 2)

  (let ((x 1) (y 2)) (cons x y))

  (let ((x 1))
    (let ((y 1))
      (cons x y)))

  (define (map func list)
    (if (nil? list) ()
      (cons
        (func (first list))
        (map func (rest list)))))

  (map addone (quote (1 2 3 4 5 )))

  ((lambda (x) (+ 1 x)) 4)

  (define (addn n)
    (lambda (x)
      (+ x n)))

  ((addn 32) 10)

  (concat "Hello " "world")

  '(1 2 3 4)

  (export addn)
)
