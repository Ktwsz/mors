% Langfords problem (m,n) m copies of digits 1..n
% where any two consecutive copies of digit k
% are separated by k other digits
int: n = 10;
set of int: DIG = 1..n;
int: m = 3;
set of int: COPY = 1..m;
int: l = m*n;
set of int: POS = 1..l;
array[DIG,COPY] of var POS: x;


set of int: DIGCOPY = 1..l;
array[POS] of var DIGCOPY: y;

include "inverse.mzn";
constraint true <-> inverse([x[d,c] | d in DIG, c in COPY], y);

constraint forall(d in DIG, c in 1..m-1)
      (x[d,c+1] = x[d,c] + d + 1);

solve satisfy;

output [show(x),"\n"]
       ++ [ show((y[p] - 1) div m + 1) ++ " "
          | p in POS ]
       ;