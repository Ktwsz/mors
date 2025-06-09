include "cumulative.mzn";
include "disjunctive_strict_opt.mzn";
include "lex_less.mzn";

int: n; % number of tasks
set of int: TASK = 1..n;
array[TASK] of int: d; % duration

int: m; % number of resources
set of int: RESOURCE = 1..m;
array[RESOURCE] of int: L; % resource limit
array[RESOURCE,TASK] of int: res; % usage

int: l; % number of precedences
set of int: PREC = 1..l;
array[PREC,1..2] of TASK: pre; % predecessor/successor pairs

int: maxt; % maximum time
set of int: TIME = 0..maxt;

array[TASK] of var TIME: s; % start time

constraint forall(p in PREC)(s[pre[p,1]]+d[pre[p,1]] <= s[pre[p,2]]);

constraint forall(r in RESOURCE)
                 (cumulative(s,d,[res[r,t]|t in TASK],L[r]));

constraint fzn_disjunctive_strict_opt(d, L);
constraint lex_less(pre[.., 1], pre[.., 2]);

solve minimize max(t in TASK)(s[t] + d[t]);

output [show(s)];


