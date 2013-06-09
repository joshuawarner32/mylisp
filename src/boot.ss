(import core eq?)
(import core nil?)
(import core rest)
(import core first)
(import core cons)
(import core cons?)
(import core sym?)
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
  (let ((res (internal-transform-defines program (cons () ()) )))
    (cons
      (quote letlambdas)
      (cons (first res)
        (cons (rest res) ())))
  )
)

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

(define (firsts val)
  (if (nil? val) ()
    (cons (first (first val)) (firsts (rest val))))
)

(define (seconds val)
  (if (nil? val) ()
    (cons (first (rest (first val))) (seconds (rest val))))
)

(define (find-macro sym macros)
  (if (nil? macros) ()
    (if (eq? sym (first (first macros)))
      (rest (first macros))
      (find-macro sym (rest macros)))))

(define (map func list)
  (if (nil? list) ()
    (cons
      (func (first list))
      (map func (rest list)))))

(define (expander macros)
  (lambda (program)
    (macroexpand program macros)))

(define (macroexpand program macros)
  (if (nil? program) ()
    (if (cons? program)
      (let ((f (first program))
            (r (rest program)))
        (if (sym? f)
          (let ((m (find-macro f macros)))
            (if (nil? m)
              (cons f (map (expander macros) r))
              (macroexpand (m program) macros))
            )
          (cons (macroexpand f macros) (map (expander macros) r))))
      program)))

(define (make-lambdas name form)
  (let ((fn-args (cons name (first form))))
    (cons
      (cons fn-args (rest form))
      ())))

(define (process-lambda form)
  (cons (quote letlambdas)
    (cons (make-lambdas (quote current-lambda) (rest form))
      (cons (quote current-lambda)
        ()))))

(define (let-fn form)
  (make-lambdas (quote current-let)
    (cons (firsts (first form))
      (rest form))))

(define (let-body form)
  (cons
    (quote current-let)
    (seconds (first form))))

(define (process-let form)
  (cons (quote letlambdas)
    (cons (let-fn (rest form))
      (cons (let-body (rest form))
        ()))))

(define (transform program)
  (macroexpand
      (transform-imports program ())
    (cons
      (cons (quote lambda) process-lambda)
      (cons 
        (cons (quote let) process-let)
        ()))
  )
)

transform
