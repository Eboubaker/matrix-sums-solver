This program is an implementation of a proof-of-work algorithm concept which can be used in blockchain. I did not make the concept but I made the best algorithm to solve it in a very fast time.

# Problem
Suppose you have a matrix of given shape RxC where `R=number of rows` and `C=number of columns` each cell can have two possible values `0` or `1`.
```
[0 1 1 0 1 0]
[0 1 0 1 1 0]
[1 0 0 0 0 0]
[0 0 0 1 0 0]
[0 0 0 0 1 1]
[0 1 0 0 0 0]
```
the sum of each row and sum of each column will be calculated and the hash of this matrix's bytes will be computed.
```
 1 3 1 2 3 1
 -----------
[0 1 1 0 1 0] | 3
[0 1 0 1 1 0] | 3
[1 0 0 0 0 0] | 1
[0 0 0 1 0 0] | 1
[0 0 0 0 1 1] | 2
[0 1 0 0 0 0] | 1

row_sums=[3, 3, 1, 1, 2, 1]
col_sums=[1, 3, 1, 2, 3, 1]
hash=62d5e751d2e57a7c737998df5b55b9f9
```

## Restrictions
1. cells value can be 1 or 0.
2. row_sums and col_sums are valid sums for an existing matrix.
3. hash is a valid hash for an existing matrix.

these inputs are then given to another program which will only know three things.
1. colum_sums
2. row_sums 
3. matrix hash

the program must solve all the posibilities and only return the valid matrix.  

this repository has an implementation which can solve in complexcity of `O((R*C)*log(2^(R*C))))` best being `O(R*C)` and worst `O(2^(R*C))` in short `O(nlog(n))`.  
the program runs with parallelism to speed up the solving process using [MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface).  

python implementation will solve 6x6 matrix in ~2seconds.  
c++ implementation will solve 40 times faster

### Try Live
run project in [google notebooks](https://colab.research.google.com/drive/1Wd9WUQ0RjNZmNG1D2JPholer28cMLBO7#scrollTo=CJKC0mbFdGDf)
