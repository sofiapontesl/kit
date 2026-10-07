#ifndef SOLUTION_H
#define SOLUTION_H


#include "data.h"
#include <vector>

struct Solution
{
    std::vector<int> route;
    double cost;

    Solution(std::vector<int> route, double cost): route(route), cost(cost) {}
    Solution() :  route(std::vector<int>(Data::getInstance().n + 1)), cost(0) {}
    Solution(const Solution &s) : route(s.route), cost(s.cost) {}


    // 2-opt 
    
    double evaluate2opt(const int i, const int j);
    void apply2opt(const int i, const int j);


    // or-opt

    double evaluateOrOpt(const int i, const int size, const int j);
    void applyOrOpt(const int i, const int size, const int j);

    double recomputeCost();
    
    void buildGreedyRandomized(double alpha);

    void buildTrivial();

    void print();

    void copy(const Solution &other);

    double evaluateSwap(const int i, const int j);

    void swap(const int i, const int j);

};

struct InsertionInfo {
    int node;      // nó k a inserir
    int edgePos;   // posição i que a aresta é (route[i], route[i+1])
    double cost;   // delta
};


#endif