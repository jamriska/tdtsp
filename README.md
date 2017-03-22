Time-Dependent Traveling Salesman
=================================

This is our entry for the kiwi.com [Traveling Salesman Challenge](https://travellingsalesman.cz).
It employs an Iterated Local Search strategy with restarts.

We start by constructing an initial tour using the Nearest Neighbor heuristic
with one level of look-ahead. Next we try to improve the tour by repeatedly
perturbing it with double-bridge kicks followed by exhaustive 2-opt minimization.
The kicks are allowed to increase the tour cost up to a specified factor.

The improvement process will stagnate, eventually. We detect the stagnation
by measuring the time since last successful improvement, and restart the search
when it exceeds a specified limit. The search is restarted from a new candidate
tour, which is obtained by perturbing the best tour that was discovered so far.
However, we only do this on the smaller instances, where there's a reasonable
chance that the algorithm will reach a better tour. We observed that on the
larger instances there is often not enough time left for the tour to get better
after the restart.


|                                | data_40 | data_50 | data_60 | data_70 | data_100 | data_200 | data_300 |
| ------------------------------ | ------: | ------: | ------: | ------: | -------: | -------: | -------: |
| initial tour cost              |    8788 |    8529 |   10585 |   15244 |    16421 |    37434 |   43695  |
| cost after 30s of improvement  |    7660 |    7308 |    9117 |   11863 |    13445 |    27420 |   37679  |


Authors
-------
[Ondřej Jamriška](http://jamriska.cz) & [Jenda Keller](https://github.com/jendakeller)

License
-------
The code is released into the public domain.
