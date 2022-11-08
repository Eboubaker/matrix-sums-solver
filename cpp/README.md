Build
```bash
$ make
```
use generator to generate random matrix and output it's sums and hash
```bash
#./generator <rows> <cols> [<seed> <show:1>]
$ ./generator 3 3
```
or make your own matrix calculate it's sums and get the hash by using hash tool
```bash
#./hash <matrix_cell_list>
$ ./hash [0,1,0,0,1,0,0,0,1]
# 0d6fd8ed35d7d46d817c4a33464412d8
```
Pipe results to solver
```bash
$ ./generator 3 3 | ./solver
```
