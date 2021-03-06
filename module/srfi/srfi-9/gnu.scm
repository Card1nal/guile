;;; Extensions to SRFI-9

;; 	Copyright (C) 2010 Free Software Foundation, Inc.
;;
;; This library is free software; you can redistribute it and/or
;; modify it under the terms of the GNU Lesser General Public
;; License as published by the Free Software Foundation; either
;; version 3 of the License, or (at your option) any later version.
;; 
;; This library is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; Lesser General Public License for more details.
;; 
;; You should have received a copy of the GNU Lesser General Public
;; License along with this library; if not, write to the Free Software
;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

;;; Commentary:

;; Extensions to SRFI-9. Fully documented in the Guile Reference Manual.

;;; Code:

(define-module (srfi srfi-9 gnu)
  #:export (set-record-type-printer!))

(define (set-record-type-printer! type thunk)
  "Set a custom printer THUNK for TYPE."
  (struct-set! type vtable-index-printer thunk))
