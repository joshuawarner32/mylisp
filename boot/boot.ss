(letlambdas 
  (((some-random-symbol + sym? cons? cons first rest nil? eq?) 
      (letlambdas 
        (((transform program) 
            (macroexpand 
              (transform-imports program ()) 
              (cons 
                (cons 
                  (quote lambda) process-lambda) 
                (cons 
                  (cons 
                    (quote let) process-let) ())))) 
          ((process-let form) 
            (cons 
              (quote letlambdas) 
              (cons 
                (let-fn 
                  (rest form)) 
                (cons 
                  (let-body 
                    (rest form)) ())))) 
          ((let-body form) 
            (cons 
              (quote current-let) 
              (seconds 
                (first form)))) 
          ((let-fn form) 
            (make-lambdas 
              (quote current-let) 
              (cons 
                (firsts 
                  (first form)) 
                (rest form)))) 
          ((process-lambda form) 
            (cons 
              (quote letlambdas) 
              (cons 
                (make-lambdas 
                  (quote current-lambda) 
                  (rest form)) 
                (cons 
                  (quote current-lambda) ())))) 
          ((make-lambdas name form) 
            (letlambdas 
              (((current-let fn-args) 
                  (cons 
                    (cons fn-args 
                      (rest form)) ()))) 
              (current-let 
                (cons name 
                  (first form))))) 
          ((macroexpand program macros) 
            (if 
              (nil? program) () 
              (if 
                (cons? program) 
                (letlambdas 
                  (((current-let f r) 
                      (if 
                        (sym? f) 
                        (letlambdas 
                          (((current-let m) 
                              (if 
                                (nil? m) 
                                (cons f 
                                  (map 
                                    (expander macros) r)) 
                                (macroexpand 
                                  (m program) macros)))) 
                          (current-let 
                            (find-macro f macros))) 
                        (cons 
                          (macroexpand f macros) 
                          (map 
                            (expander macros) r))))) 
                  (current-let 
                    (first program) 
                    (rest program))) program))) 
          ((expander macros) 
            (letlambdas 
              (((current-lambda program) 
                  (macroexpand program macros))) current-lambda)) 
          ((map func list) 
            (if 
              (nil? list) () 
              (cons 
                (func 
                  (first list)) 
                (map func 
                  (rest list))))) 
          ((find-macro sym macros) 
            (if 
              (nil? macros) () 
              (if 
                (eq? sym 
                  (first 
                    (first macros))) 
                (rest 
                  (first macros)) 
                (find-macro sym 
                  (rest macros))))) 
          ((seconds val) 
            (if 
              (nil? val) () 
              (cons 
                (first 
                  (rest 
                    (first val))) 
                (seconds 
                  (rest val))))) 
          ((firsts val) 
            (if 
              (nil? val) () 
              (cons 
                (first 
                  (first val)) 
                (firsts 
                  (rest val))))) 
          ((write-imports body imports) 
            (cons 
              (quote letlambdas) 
              (cons 
                (cons 
                  (cons 
                    (cons 
                      (make-symbol) 
                      (import-names imports)) 
                    (cons body ())) ()) 
                (cons 
                  (cons 
                    (make-symbol) imports) ())))) 
          ((import-names imps) 
            (if 
              (nil? imps) () 
              (cons 
                (first 
                  (rest 
                    (rest 
                      (first imps)))) 
                (import-names 
                  (rest imps))))) 
          ((make-symbol) 
            (quote some-random-symbol)) 
          ((transform-imports program imports) 
            (if 
              (nil? program) 
              (write-imports program imports) 
              (if 
                (cons? 
                  (first program)) 
                (if 
                  (eq? 
                    (first 
                      (first program)) 
                    (quote import)) 
                  (transform-imports 
                    (rest program) 
                    (cons 
                      (first program) imports)) 
                  (write-imports 
                    (transform-defines program) imports)) 
                (write-imports 
                  (transform-defines program) imports)))) 
          ((transform-defines program) 
            (letlambdas 
              (((current-let res) 
                  (cons 
                    (quote letlambdas) 
                    (cons 
                      (first res) 
                      (cons 
                        (rest res) ()))))) 
              (current-let 
                (internal-transform-defines program 
                  (cons () ()))))) 
          ((transform-define def new-program) 
            (if 
              (cons? def) 
              (if 
                (eq? 
                  (first def) 
                  (quote define)) 
                (cons 
                  (cons 
                    (rest def) 
                    (first new-program)) 
                  (rest new-program)) 
                (cons 
                  (first new-program) def)) 
              (cons 
                (first new-program) def))) 
          ((internal-transform-defines program new-program) 
            (if 
              (nil? program) new-program 
              (internal-transform-defines 
                (rest program) 
                (transform-define 
                  (first program) new-program))))) transform))) 
  (some-random-symbol 
    (import core +) 
    (import core sym?) 
    (import core cons?) 
    (import core cons) 
    (import core first) 
    (import core rest) 
    (import core nil?) 
    (import core eq?)))
