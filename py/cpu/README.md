Requirements
```bash
$ pip install -r requirements.txt
```
use generator to generate random matrix and output it's sums and hash
```bash
#python generator.py <rows> <cols> [<seed> <show:1>]
$ python generator.py 3 3
```
or make your own matrix calculate it's sums and get the hash by using hash tool
```bash
#python hash.py <matrix_cell_list>
$ python hash.py [[0,1,0],[0,1,0],[0,0,1]]
# 0d6fd8ed35d7d46d817c4a33464412d8
```
Pipe results to solver
```bash
$ python generator.py 3 3 | python solver.py
```
Use mpi to split solving for multiple processors. [windows release](https://www.microsoft.com/en-us/download/details.aspx?id=57467)
```bash
$ sudo apt install mpich
$ python generator.py 4 5 | mpiexec -np 2 python solver.py
```
