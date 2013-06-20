(import core
  (+ cons first rest concat ctor eq?))

(let ((m (module
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

  (export addn)
 )))
  
  ((((m (lambda (m)
    (if (eq? m 'core)
      (lambda (sym)
        (if (eq? sym '+) +
          (if (eq? sym 'cons) cons
            (if (eq? sym 'first) first
              (if (eq? sym 'rest) rest
                (if (eq? sym 'concat) concat
                  (if (eq? sym 'ctor) ctor
                    (error sym))))))))
      (error))))
    'addn)
    1) 2)
)
