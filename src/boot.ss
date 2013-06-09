(import core eq?)
(import core nil?)
(import core rest)
(import core first)
(import core cons)
(import core cons?)
(import core +)

(define (internal-transform-defines program new-program)
  (if (nil? program) new-program
    (internal-transform-defines
      (rest program)
      (transform-define (first program) new-program)))
)

(define (transform-define def new-program)
  (if (cons? def)
    (if (eq? (first def) (quote define))
      (cons (cons (rest def) (first new-program)) (rest new-program))
      (cons (first new-program) def))
    (cons (first new-program) def))
)

(define (transform-defines program)
  ((letlambdas
      ( ((mylet res)
          (cons
            (quote letlambdas)
            (cons (first res)
              (cons (rest res) ())))
        )
      )
      mylet)
    (internal-transform-defines program (cons () ()) )))

(define (transform-imports program imports)
  (if (nil? program) (write-imports program imports)
    (if (cons? (first program))
      (if (eq? (first (first program)) (quote import))
        (transform-imports (rest program) (cons (first program) imports))
        (write-imports (transform-defines program) imports))
      (write-imports (transform-defines program) imports))))

(define (make-symbol) (quote some-random-symbol))

(define (import-names imps)
  (if (nil? imps) ()
    (cons
      (first (rest (rest (first imps) )))
      (import-names (rest imps)))
  )
)

(define (write-imports body imports)
  (cons
    (quote letlambdas)
    (cons 
      (cons
        (cons (cons (make-symbol) (import-names imports))
          (cons body ()))
        ())
      (cons
        (cons (make-symbol) imports)
        ())
    )
  )
)

(define (transform program)
  (transform-imports program ())
)

transform
