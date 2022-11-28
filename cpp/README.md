# Build
The recommended way to run the prorgram is with docker or a VM, so nothing bad happens when executing...
```bash
docker build . -t matrix
docker run --rm -it matrix
# inside the image you can execute ./generator or ./solver
```
or just build and run on your main machine
```bash
$ make
```
> Requirements:  
> g++ make libssl-dev openmpi-bin libopenmpi-dev libboost-all-dev

# Usage

use generator to generate random matrix and output it's sums and hash
```bash
#./generator <rows> <cols> [<seed> <show:0>]
$ ./generator 3 3
$ [2,2,1] [2,1,2] 80ca2b136cf8d9a2a962d5fe323dd34b 3187863844
```
or make your own matrix, calculate it's sums and get the hash by using hash tool
```bash
#./hash <matrix_cell_list>
$ ./hash [0,1,0,0,1,0,0,0,1]
$ 0d6fd8ed35d7d46d817c4a33464412d8
```
solver usage
```bash
./solver <row_sums> <col_sums> <hash> <seed_nb>
```

Pipe results of generator into solver and solve using mpi
```bash
$ ./generator 3 3 | mpirun -n 2 ./solver
```
