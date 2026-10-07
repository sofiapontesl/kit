#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include "solution.h"

void RVND(Solution &s);

bool bestImprovementSwap(Solution &s);
bool bestImprovement2Opt(Solution &s);
bool bestImprovementOrOpt(Solution &s, int size); 

Solution solve();

Solution perturbacao(const Solution &best);

Solution ILS(int maxIter, int maxIterIls);

#endif // LOCAL_SEARCH_H