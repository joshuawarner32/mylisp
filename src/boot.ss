(letlambdas
  ( ((theenv + cons? cons first rest nil? eq?)
      (letlambdas
        ( ((internal-transform program new-program)
            (if (nil? program) new-program
              (internal-transform
                (rest program)
                (transform-define (first program) new-program)))
          )
          ((transform-define def new-program)
            (if (eq? (first def) (quote define))
              (cons (cons (rest def) (first new-program)) (rest new-program))
              (cons (first new-program) def))
          )
          ((transform program)

            ((letlambdas
                ( ((mylet res)
                    (cons
                      (quote letlambdas)
                      (cons (first res)
                        (cons (rest res) ())))
                  )
                )
                mylet)
              (internal-transform program (cons () ()) )
            )
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