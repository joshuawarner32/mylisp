(import core +)
(import core cons)

(define (addone x)
  (+ 1 x))

(addone 2)

(let ((x 1) (y 2)) (cons x y))

(let ((x 1))
  (let ((y 1))
    (cons x y)))