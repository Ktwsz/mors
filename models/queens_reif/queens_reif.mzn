
int: n = 10;                                 % The number of queens.

array [1..n] of var 1..n: q;

include "alldifferent.mzn";

constraint true <-> alldifferent(q);
constraint true <-> alldifferent(i in 1..n)(q[i] + i);
constraint true <-> alldifferent(i in 1..n)(q[i] - i);


solve satisfy;

output	
	[	if fix(q[i]) = j then "Q " else ". " endif ++
	 	if j = n then "\n" else "" endif
	|	i, j in 1..n
	];
