<a id="readme-top"></a>
<div align="center">
  <h3 align="center">Mors</h3>

  <p align="center">
    MiniZinc to OR-Tools transpiler
  </p>
</div>



  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#overview">Overview</a></li>
        <li><a href="#global-constraints">Global constraints</a></li>
        <li><a href="#reifications">Reifications</a></li>
        <li><a href="#inputting-model-parameters-at-runtime">Inputting model paremeters at runtime</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#build-from-source">Build from source</a></li>
        <li><a href="#installing-minizinc-stdlib-and-or-tools-redefinitions">Installing MiniZinc stdlib and OR-Tools redefinitions</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li>
    <a href="#usage">Usage</a>
    <ul>
        <li><a href="#run-mors">Run mors</a></li>
        <li><a href="#run-the-model">Run the model</a></li>
      </ul>
    </li>
    <li><a href="#further-development">Further development</a></li>
    <li><a href="#license">License</a></li>
  </ol>



<!-- ABOUT THE PROJECT -->
## About The Project

This cli tool is a transpiler, which takes MiniZinc model as an input and outputs OR-Tools CP-SAT model in Python. The project strives to combine high abstraction level and readability of MiniZinc with native OR-Tools interface. The transpiler supports all functionalities of MiniZinc and translates them into Python, OR-Tools compatible, code.

Mors will be most useful to:
  1. People who already implemented model and want to include it in a containerized application but are unable to run it without the robust MiniZinc tool package.
  2. People who want to migrate their system from MiniZinc to an OR-Tools environment.
  3. People who want to run their model with different parameters each time, without recompiling.

### Overview

Consider the model `xkcd` and its transpilation listed below. All parameters and variables are translated according Python types. We have our own implementation of arrays, which supports different indexing sets and array access constraints. Constraints are translated based on the context, the first constraint becomes a loop, second one is sum with array comprehension. Output is implemented in a class `SolutionPrinter`, which extends `cp_model.CpSolverSolutionCallback`.

#### MiniZinc
[xkcd.mzn](models/xkcd/xkcd.mzn)
```minizinc
int: menu_length = 6;
int: money_limit = 1505;
array[1..menu_length] of int: menu_prices =
  [215, 275, 335, 355, 420,580];
array[1..menu_length] of string: menu_names =
  ["fruit", "fries", "salad", "wings", "sticks", "sampler"];

array[1..menu_length] of var int: order;

constraint forall([order[i] >= 0 | i in 1..menu_length]);
constraint
  sum([
    order[i] * menu_prices[i] 
    | i in 1..menu_length
  ]) == money_limit;

solve satisfy; 

output [
  menu_names[i] ++ ": " ++ show(order[i]) ++ "\n"
  | i in 1..menu_length
];
```

#### Mors output
[xkcd.py](models/xkcd/xkcd.py)

```Python
from ortools.sat.python import cp_model
from mors_lib import *

model = cp_model.CpModel()

menu_length = 6
money_limit = 1505
menu_prices = Array(range(1, menu_length + 1), Array([215, 275, 335, 355, 420, 580]))
menu_names = Array(
    range(1, menu_length + 1),
    Array(["fruit", "fries", "salad", "wings", "sticks", "sampler"]),
)
order = IntVarArray("order", range(1, menu_length + 1), None)

for i in range(1, menu_length + 1):
    model.Add(order[i] >= 0)

model.Add(
    sum(Array([order[i] * menu_prices[i] for i in range(1, menu_length + 1)]))
    == money_limit
)

solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(menu_length, menu_names, order)
status = solver.solve(model, solution_printer)

# def on_solution_callback(self) -> None:
#   self.__solution_count += 1
#   for i in range(1, self.menu_length + 1):
#     print(
#       self.menu_names[i] + ": " + str(self.value(self.order[i])) + "\n",
#       end="",
#     )
```

### Global constraints

Global constraints are transpiled based on the OR-Tools redefinitions for FlatZinc. We provide wrapper implementations, which add the constraint to the `CpModel` instance, for all redefined constraints.

Example result for model using `alldifferent` constraint.

#### MiniZinc

[nqueens.mzn](models/queens/queens.mzn)

  
```minizinc

int: n = 10;

array [1..n] of var 1..n: q;

include "alldifferent.mzn";

constraint alldifferent(q);
constraint alldifferent(i in 1..n)(q[i] + i);
constraint alldifferent(i in 1..n)(q[i] - i);


solve satisfy;

output [
  if fix(q[i]) = j then "Q " else ". " endif ++
 	if j = n then "\n" else "" endif
  | i, j in 1..n
];

```
  
#### Mors output
[nqueens.py](models/queens/queens.py)

```Python
from ortools.sat.python import cp_model
from mors_lib import *

model = cp_model.CpModel()

def all_different_41(x):
    ortools_all_different(array1d(x)) # defined in mors_lib

def alldifferent_40(x):
    all_different_41(array1d(x))

n = 10
q = IntVarArray("q", range(1, n + 1), range(1, n + 1))
alldifferent_40(q)
alldifferent_40(Array([q[i] + i for i in range(1, n + 1)]))
alldifferent_40(Array([q[i] - i for i in range(1, n + 1)]))

solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(n, q)
status = solver.solve(model, solution_printer)

# def on_solution_callback(self) -> None:
#   self.__solution_count += 1
#   for i in range(1, self.n + 1):
#     for j in range(1, self.n + 1):
#       print(
#         ("Q " if self.value(self.q[i]) == j else ". ")
#         + ("\n" if j == self.n else ""),
#         end="",
#       )

```

### Reifications

We implement functions, which reify the OR-Tools `Constraint` objects to boolean literals, so that constraints can be composed together using logical operators, just like in MiniZinc.

The stable marriage model uses array access constraints as well as implications. These constraints can be defined in a single line expressions, thanks to our reification abstraction.

#### MiniZinc


[stable_marriage.mzn](models/stable-marriage/stable-marriage.mzn)

  
```minizinc
int: n = 5;

enum Men = _(1..n);
enum Women = _(1..n);

array[Women, Men] of int: rankWomen =
 [| 1, 2, 4, 3, 5,
  | 3, 5, 1, 2, 4,
  | 5, 4, 2, 1, 3,
  | 1, 3, 5, 4, 2,
  | 4, 2, 3, 5, 1 |];
array[Men, Women] of int: rankMen =
 [| 5, 1, 2, 4, 3,
  | 4, 1, 3, 2, 5,
  | 5, 3, 2, 4, 1,
  | 1, 5, 4, 3, 2,
  | 4, 3, 2, 1, 5 |];

array[Men] of var Women: wife;
array[Women] of var Men: husband;

% assignment
constraint forall (m in Men) (husband[wife[m]]=m);
constraint forall (w in Women) (wife[husband[w]]=w);
% ranking
constraint forall (m in Men, o in Women) (
     rankMen[m,o] < rankMen[m,wife[m]] -> 
         rankWomen[o,husband[o]] < rankWomen[o,m] );

constraint forall (w in Women, o in Men) (
     rankWomen[w,o] < rankWomen[w,husband[w]] -> 
         rankMen[o,wife[o]] < rankMen[o,w] );
solve satisfy;

output ["wives= \(wife)\nhusbands= \(husband)\n"];
```
#### Mors output
[stable_marriage.py](models/stable-marriage/stable-marriage.py)

```Python
from ortools.sat.python import cp_model
from mors_lib import *

model = cp_model.CpModel()

n = 5
Men = set(range(1, max(range(1, n + 1)) + 1))
Women = set(range(1, max(range(1, n + 1)) + 1))
rankWomen = Array(
    [Women, Men],
    Array([1, 2, 4, 3, 5, 3, 5, 1, 2, 4, 5, 4, 2, 1, 3, 1, 3, 5, 4, 2, 4, 2, 3, 5, 1]),
)
rankMen = Array(
    [Men, Women],
    Array([5, 1, 2, 4, 3, 4, 1, 3, 2, 5, 5, 3, 2, 4, 1, 1, 5, 4, 3, 2, 4, 3, 2, 1, 5]),
)
wife = IntVarArray("wife", Men, Women)
husband = IntVarArray("husband", Women, Men)
for m in Men:
    model.Add(husband[wife[m]] == m)
for w in Women:
    model.Add(wife[husband[w]] == w)
for m in Men:
    for o in Women:
        model.add_implication(
            mors_lib_bool(
                rankMen[m, o] < rankMen[m, wife[m]],
                rankMen[m, o] >= rankMen[m, wife[m]],
            ),
            mors_lib_bool(
                rankWomen[o, husband[o]] < rankWomen[o, m],
                rankWomen[o, husband[o]] >= rankWomen[o, m],
            ),
        )
for w in Women:
    for o in Men:
        model.add_implication(
            mors_lib_bool(
                rankWomen[w, o] < rankWomen[w, husband[w]],
                rankWomen[w, o] >= rankWomen[w, husband[w]],
            ),
            mors_lib_bool(
                rankMen[o, wife[o]] < rankMen[o, w],
                rankMen[o, wife[o]] >= rankMen[o, w],
            ),
        )
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(wife, husband)
status = solver.solve(model, solution_printer)

# def on_solution_callback(self) -> None:
#   self.__solution_count += 1
#   print(
#     "wives= "
#     + (
#       str([self.value(v) for v in self.wife])
#       + ("\nhusbands= " + (str([self.value(v) for v in self.husband]) + "\n"))
#     ),
#     end="",
# )
```

### Inputting model parameters at runtime

Mors provides the ability to load parameters at runtime from JSON file. Simply add the flag `--runtime-parameters` and you don't have to provide the data at compile time.

```sh
mors build models/stable-marriage-json/stable-marriage-json.mzn --runtime-parameters
```

Parameters will use `mors_lib` function `load_from_json`, which will extract the values out of a JSON file.

```Python
n = load_from_json('n')
Men = set(range(1, max(range(1, n + 1)) + 1))
Women = set(range(1, max(range(1, n + 1)) + 1))
rankWomen = load_array_from_json('rankWomen', 'int', Women, Men)
rankMen = load_array_from_json('rankMen', 'int', Men, Women)
wife = IntVarArray('wife', Men, Women)
husband = IntVarArray('husband', Women, Men)
```

Pass the JSON file, when you run the model. The JSON should be in the same format, as described in [MiniZinc documentation](https://docs.minizinc.dev/en/stable/spec.html#json-support).

```sh
python models/stable-marriage-json/stable-marriage-json.py models/stable-marriage-json/params.json
```

You can still pass datafiles at compile time, only the parameters without set value will be read at runtime.

```sh
mors build models/stable-marriage/stable-marriage.mzn part_of_data.dzn --runtime-parameters
```


<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started


### Prerequisites
* GCC compiler, version 15 or higher
* CMake
* Python 3.13

### Build from source
#### Compilation
1. Download the source code
    ```sh
    git clone https://github.com/Ktwsz/mors.git
    cd mors
    ```
3. Run CMake
    ```sh
    cd build
    cmake ..
    cmake --build .
    ```

### Installing MiniZinc stdlib and OR-Tools redefinitions
In order for mors to work, you need to have minizinc standard library installed (visible to the compiler) in version 2.9.3 as well as OR-Tools' redefinitions. We provide the minimal setup in the `share` directory. For more help with installation run mors with flag `check-installation`
```sh
mors check-installation
```

This command will help you identify where MiniZinc searches for stdlib and solvers.

If you want to manually specify the paths, use flags `--stdlib-dir` and `--search-dir`.

You can also refer to MiniZinc documentation [Configuration files](https://docs.minizinc.dev/en/stable/command_line.html#configuration-files).

### Installation
TBD &ndash; for now, we do not provide pre-built packages.

#### Manual installation
If you built the project from source and wish to install the resulting binary, make sure that `emitter` and `ir_python` Python modules, as well as `share` directory provided in this repository, are visible to the `mors` binary. The simplest solution is to copy these directories to the same directory as the binary. You can also take inspiration from [setup_project.sh](setup_project.sh) script.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage
### Run mors
For sample models, look at `models` directory.
```sh
mors build models/sudoku/sudoku.mzn models/sudoku/sudoku.dzn
```

### Run the model
#### Setup
In order to run the transpiled model you must install python library `or-tools` and make the `mors_lib` visible to the transpiled script (you can just copy the `mors_lib` folder to the same directory as the model). Here is a sample setup using venv

1. Create venv
    ```sh
    python3.13 -m venv venv
    source venv/scripts/activate
    ```

2. Install `or-tools` and `multimethod`
    ```sh
    pip install or-tools multimethod
    ```
3. Move the `mors_lib` to venv
    ```sh
    mv mors_lib venv/lib/python3.13/site-packages
    ```

#### Run model
```sh
python models/sudoku/sudoku.py
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Further development

Future plans for developing mors include:
- Reifications for float variables
- Reifications for set variables and constraints
- Additional data types: option, record and tuple types
- Support for TypeInst
- Support for Annotations
- Extending `mors_lib` with more MiniZinc stdlib functions
- Slicing arrays in `Array` type
- Adding `run` option to mors, to immediately run the transpiled model
- Formatting the Python generated code using formatter

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- LICENSE -->
## License

Distributed under the Mozilla Public License Version 2.0. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


[C++]: https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white
[Python]: https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54
[CMake]: https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white
[Conan]: https://img.shields.io/badge/conan-%236699CB.svg?style=for-the-badge&logo=conan&logoColor=white