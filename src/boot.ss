(letlambdas
  ( ((theenv + cons? cons first rest nil? eq?)
      (letlambdas
        ( ((internal-transform-defines program new-program)
            (if (nil? program) new-program
              (internal-transform-defines
                (rest program)
                (transform-define (first program) new-program)))
          )

          ((transform-define def new-program)
            (if (eq? (first def) (quote define))
              (cons (cons (rest def) (first new-program)) (rest new-program))
              (cons (first new-program) def))
          )

          ((transform-defines program)
            ((letlambdas
                ( ((mylet res)
                    (cons
                      (quote letlambdas)
                      (cons (first res)
                        (cons (rest res) ())))
                  )
                )
                mylet)
              (internal-transform-defines program (cons () ()) )
            )
          )

          ((and a b) (if a b #f))

          ((transform-imports program imports)
            (if (nil? program) (write-imports program imports)
              (if (eq? (first (first program)) (quote import))
                (transform-imports
                  (rest program)
                  (cons (first program) imports))
                (write-imports (transform-defines program) imports))
            )
          )

          ((make-symbol) (quote some-random-symbol))

          ((import-names imps)
            (if (nil? imps) ()
              (cons
                (first (rest (rest (first imps) )))
                (import-names (rest imps)))
            )
          )

          ((write-imports body imports)
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

          ((transform program)
            (transform-imports program ())
          )
        )
        transform
      )
    )
  )
  (theenv
    (import core +)
    (import core cons?)
    (import core cons)
    (import core first)
    (import core rest)
    (import core nil?)
    (import core eq?))
)