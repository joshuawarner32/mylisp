(module
  (import core
    (eq? rest first cons + ctor))

  (define (nil? v) (eq? (ctor v) 'Nil))
  (define (cons? v) (eq? (ctor v) 'cons))
  (define (sym? v) (eq? (ctor v) 'Symbol))

  (define (and a b) (if a b #f))
  (define (not a) (if a #f #t))

  (define (list . items) items)

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
      (if (cons? list)
        (cons
          (func (first list))
          (map func (rest list)))
        (func list))))

  (define (expander macros)
    (lambda (program)
      (macroexpand program macros)))

  (define (macroexpand program macros)
    (if (nil? program) ()
      (if (cons? program)
        (let ((f (first program))
              (r (rest program)))
          (if (and (sym? f) (not (eq? f 'quote)))
            (let ((m (find-macro f macros)))
              (if (nil? m)
                (cons f (map (expander macros) r))
                (macroexpand (m program) macros))
              )
            (cons (macroexpand f macros) (map (expander macros) r))))
        program)))

  (define (make-lambdas name form)
    (let ((fn-args (cons name (first form))))
      (list
        (cons fn-args (rest form)))))

  (define (process-lambda form)
    (list 'letlambdas
      (make-lambdas 'current-lambda (rest form))
      'current-lambda))

  (define (let-fn form)
    (make-lambdas 'current-let
      (cons (firsts (first form))
        (rest form))))

  (define (let-body form)
    (cons
      'current-let
      (seconds (first form))))

  (define (process-let form)
    (list 'letlambdas
      (let-fn (rest form))
      (let-body (rest form))))

  (define (collect-imports form)
    (if (nil? form) ()
      (if (not (cons? form)) ()
        (if (eq? (first (first form)) (quote import))
          (cons (rest (first form)) (collect-imports (rest form)))
          (collect-imports (rest form))))))

  (define (collect-defines form)
    (if (nil? form) ()
      (if (not (cons? form)) ()
        (if (eq? (first (first form)) (quote define))
          (cons (rest (first form)) (collect-defines (rest form)))
          (collect-defines (rest form))))))

  (define (collect-exports form)
    (if (nil? form) ()
      (if (not (cons? form)) ()
        (if (eq? (first (first form)) (quote export))
          (cons (rest (first form)) (collect-exports (rest form)))
          (collect-exports (rest form))))))

  (define (make-module-lambda inner)
    (list 'lambda
      (cons 'env ())
      inner))

  (define (make-import name)
    (list 'env
      (list 'quote name)))

  (define (module-map imports)
    (map (lambda (imp)
        (let ((name (first imp)))
          (cons name
            (cons (make-import name)
              () ))))
      imports))

  (define (make-modules-let imports inner)
    (list 'let
      (module-map imports)
      inner))

  (define (imports-map imports)
    (let ((name (first imports))
          (lst (first (rest imports))))
      (map (lambda (val)
          (list val
            (list name
              (list 'quote val))))
        lst)))

  (define (make-imports-let imports inner)
    (if (eq? imports ()) inner
      (list 'let
        (imports-map (first imports))
        (make-imports-let (rest imports) inner))))

  (define (make-defines-letlambda defines inner)
    (list 'letlambdas defines inner))

  (define (make-export-condition name)
    (list eq? 'sym-name (list 'quote name)))

  (define (make-exports-cases exports)
    (if (nil? exports) (list 'error 'sym-name)
      (list 'if
        (make-export-condition (first exports))
        (first exports)
        (make-exports-cases (rest exports)))))

  (define (make-exports-lambda exports)
    (list 'lambda
      (list 'sym-name)
      (make-exports-cases exports)))

  (define (process-module form)
    (let ((imports (collect-imports (rest form)))
          (defines (collect-defines (rest form)))
          (exports (collect-exports (rest form))))
          (make-module-lambda
            (make-modules-let imports
              (make-imports-let imports
                (make-defines-letlambda defines
                  (make-exports-lambda (first exports))))))))

  (define (default-macroexpand program)
    (macroexpand program
      (list
        (cons 'lambda process-lambda)
        (cons 'let process-let)
        (cons 'module process-module))))

  (define (transform program)
    (default-macroexpand program))

  (export transform default-macroexpand)
)