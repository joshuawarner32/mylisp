(letlambdas 
  (((some-random-symbol + cons? cons first rest nil? eq?) 
      (letlambdas 
        (((transform program) 
            (transform-lets 
              (transform-imports program ()))) 
          ((transform-lets program) 
            (if 
              (nil? program) () 
              (cons 
                (transform-let 
                  (first program)) 
                (transform-lets 
                  (rest program))))) 
          ((transform-let form) 
            (if 
              (cons? form) 
              (if 
                (eq? 
                  (first form) 
                  (quote let)) 
                (cons 
                  (quote letlambdas) 
                  (cons 
                    (let-fn 
                      (rest form)) 
                    (cons 
                      (let-body 
                        (rest form)) ()))) 
                (cons 
                  (transform-let 
                    (first form)) 
                  (transform-lets 
                    (rest form)))) form)) 
          ((let-body form) 
            (cons 
              (quote current-let) 
              (seconds 
                (first form)))) 
          ((let-fn form) 
            (cons 
              (cons 
                (cons 
                  (quote current-let) 
                  (firsts 
                    (first form))) 
                (cons 
                  (first 
                    (rest form)) ())) ())) 
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
    (import core cons?) 
    (import core cons) 
    (import core first) 
    (import core rest) 
    (import core nil?) 
    (import core eq?)))
