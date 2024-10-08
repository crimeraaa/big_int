package bigint

/* 
Our internal representation of a single digit of specified `DIGIT_BASE`.
It must be capable of holding all values in the range `0..=DIGIT_MAX`.
 */
DIGIT :: distinct u32

/* 
"Signed Word"

Representation of the result of arithmetic between 2 `DIGIT`. 

E.g. if `DIGIT_BASE == 1_000_000_000`, this must be capable of representing the
following:

              0 - DIGIT_MAX  = -999_999_999
      DIGIT_MAX + DIGIT_MAX  =  1_999_999_998
      DIGIT_MAX * DIGIT_MAX  =  999_999_998_000_000_001
    -(DIGIT_MAX * DIGIT_MAX) = -999_999_998_000_000_001
 */
SWORD :: distinct i64

/* 
"Unsigned Word"

A type used mainly for `bigint_set_from_integer`. We use unsigned because we
always attempt to get the absolute value of the input integer no matter what and
just track its signedness in a boolean.
 */
UWORD :: distinct u64

/* 
Some multiple of 10 that fits in `DIGIT_TYPE`. We use a multiple of 10 since
it makes things somewhat easier when conceptualizing.

Since `DIGIT_TYPE` is a `u32`, the maximum actual value is 4_294_967_296.
However we need the get the nearest lower power of 10.
 */
DIGIT_BASE :: 1_000_000_000
DIGIT_MAX :: DIGIT_BASE - 1

/* 
The number of base-10 digits in `DIGIT_MAX`.
 */
DIGIT_WIDTH :: 9

/* 
`DIGIT_WIDTH_{BIN,OCT,HEX}`:
    Number of base-N digits that, when all are set to N-1, can safely fit in their
    base-DIGIT_BASE representation.

Rationale, see `bases.py`.
 */
DIGIT_WIDTH_BIN :: 29
DIGIT_WIDTH_OCT :: 9
DIGIT_WIDTH_HEX :: 7

#assert(2 <= DIGIT_BASE && DIGIT_BASE < max(DIGIT))
