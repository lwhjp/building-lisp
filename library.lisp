;;
;; Functions used in macro definitions
;;

(define (append a b) (foldr cons b a))

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))

(define (foldl proc init list)
  (if list
      (foldl proc
             (proc init (car list))
             (cdr list))
      init))

(define (foldr proc init list)
  (if list
      (proc (car list)
            (foldr proc init (cdr list)))
      init))

(define (list . items)
  (foldr cons nil items))

(define (unary-map proc list)
  (foldr (lambda (x rest) (cons (proc x) rest))
         nil
         list))

(define (map proc . arg-lists)
  (if (car arg-lists)
      (cons (apply proc (unary-map car arg-lists))
            (apply map (cons proc
                             (unary-map cdr arg-lists))))
      nil))

;;
;; Quasiquote
;;

(defmacro (quasiquote x)
  (if (pair? x)
      (if (eq? (car x) 'unquote)
          (cadr x)
          (if (eq? (if (pair? (car x)) (caar x) nil) 'unquote-splicing)
              (list 'append
                    (cadr (car x))
                    (list 'quasiquote (cdr x)))
              (list 'cons
                    (list 'quasiquote (car x))
                    (list 'quasiquote (cdr x)))))
      (list 'quote x)))

;;
;; Macros
;;

(defmacro (and . terms)
  (if terms
      `(if ,(car terms)
           (and ,@(cdr terms))
           nil)
      t))

(defmacro (begin . body)
  `((lambda () ,@body)))

(defmacro (cond . clauses)
  (if clauses
      (let ((test (caar clauses))
           (body (cdar clauses)))
           `(if ,test
                (begin ,@body)
                (cond ,@(cdr clauses))))
      nil))

(defmacro (let defs . body)
  `((lambda ,(map car defs) ,@body)
    ,@(map cadr defs)))

(defmacro (or . terms)
  (if terms
      `(if ,(car terms)
           t
           (or ,@(cdr terms)))
      nil))

(defmacro (unless test . body)
  `(when (not ,test) ,@body))

(defmacro (when test . body)
  `(if ,test (begin ,@body) nil))

;;
;; Numeric functions
;;

(define +
  (let ((old+ +))
    (lambda xs (foldl old+ 0 xs))))

(define -
  (let ((old- -))
    (lambda (x . xs)
      (if xs
          (foldl old- x xs)
          (old- 0 x)))))

(define *
  (let ((old* *))
    (lambda xs (foldl old* 1 xs))))

(define /
  (let ((old/ /))
    (lambda (x . xs)
      (if xs
          (foldl old/ x xs)
          (old/ 1 x)))))

(define (<= a b) (or (= a b) (< a b)))
(define (> a b) (< b a))
(define (>= a b) (<= b a))

(define (abs x) (if (negative? x) (- x) x))

(define (even? x) (= (modulo x 2) 0))

(define (gcd . xs)
  (define (gcd-inner a b)
    (if (zero? b) a (gcd-inner b (remainder a b))))
  (abs (foldl gcd-inner 0 xs)))

(define (lcm . xs)
  (if xs
      (/ (abs (apply * xs))
         (apply gcd xs))
      1))

(define (max x . xs)
  (foldl (lambda (a b) (if (> a b) a b)) x xs))

(define (min x . xs)
  (foldl (lambda (a b) (if (< a b) a b)) x xs))

(define (negative? x) (< x 0))

(define (odd? x) (= (modulo x 2) 1))

(define (positive? x) (> x 0))

(define (quotient a b) (/ a b))

(define (remainder a b) (- a (* b (quotient a b))))

(define (zero? x) (= x 0))

;; TODO: modulo


;;
;; List functions
;;

(define (for-each proc . arg-lists)
  (when (car arg-lists)
    (apply proc (map car arg-lists))
    (apply for-each
           (append (list proc)
                   (map cdr arg-lists)))))

(define (length list)
  (foldl (lambda (count x) (+ count 1)) 0 list))

(define (list-ref x k) (car (list-tail x k)))

(define (list-tail x k)
  (if (zero? k)
      x
      (list-tail (cdr x) (- k 1))))

(define (reverse list)
  (foldl (lambda (a x) (cons x a)) nil list))

;;
;; Other functions
;;

(define (list? x)
  (or (null? x)
      (and (pair? x)
           (list? (cdr x)))))

(define (not x) (if x nil t))

(define (null? x) (not x))
