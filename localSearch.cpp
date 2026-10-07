#include "localSearch.h"
#include "stdlib.h"
#include <iostream>
#include <algorithm>
#include <cmath>

void RVND(Solution &s){

    // adicionar 3, 4 e 5 na lista inicial OK 
    std::vector<int> NL = {1, 2, 3, 4, 5}; 
    bool improved = false;

    while(NL.empty() == false){
        int n = (rand() % NL.size());

        switch(NL[n]){
            case 1:
                improved = bestImprovementSwap(s);
                break;
            case 2:
                improved = bestImprovement2Opt(s);
                break;
            case 3: 
                improved = bestImprovementOrOpt(s, 1); 
                break; // Reinsertion
            case 4: 
                improved = bestImprovementOrOpt(s, 2); 
                break; // Or-opt-2
            case 5: 
                improved = bestImprovementOrOpt(s, 3); 
                break; // Or-opt-3
            default:
                break;
        }

        if (improved){
            // se melhorou, a lista recomeça com todos os 5 movimentos!
            NL = {1, 2, 3, 4, 5}; 
        } else {
            NL.erase(NL.begin() + n);
        }
    }
}






bool bestImprovementOrOpt(Solution &s, int size){ //ta de tras p frente e de frente p tras
    Data & data = Data::getInstance(); ///o size é pq por exemplo nao tem a função do 3 opt ai o sizr q vai passar qual vai sewr
    double delta, bestDelta = 0.0;
    int bestI = -1, bestJ = -1, i, j;
    bool improved = false;

    for(i = 1; i <= data.n - size; i++){
        for(j = 0; j < data.n; j++){
            // pular posicoes dentro ou coladas no bloco
            if (j >= i - 1 && j <= i + size - 1) continue;

            delta = s.evaluateOrOpt(i, size, j);
            if(delta < bestDelta){
                bestDelta = delta;
                bestI = i;
                bestJ = j;
                improved = true;
            }
        }
    }

    if(improved){
        s.applyOrOpt(bestI, size, bestJ);
    }

    return improved;


}bool bestImprovement2Opt(Solution &s){  //o melhor de todos!!! #tira os cruzamentos rs
    Data & data = Data::getInstance(); 
    double delta, bestDelta = 0.0; //basicamente pega as arestas e nao os vertices que nem o swap e troca p evitar os cruzamentos
    int bestI = -1, bestJ = -1, i, j;
    bool improved = false;

    for(i = 1; i < data.n - 1; i++){ 
        for(j = i + 1; j < data.n; j++){
            delta = s.evaluate2opt(i, j);
            if(delta < bestDelta){
                bestDelta = delta;
                bestI = i;
                bestJ = j;
                improved = true;
            }
        }
    }

    if(improved){
        s.apply2opt(bestI, bestJ);
    }

    return improved;
}

bool bestImprovementSwap(Solution &s){
    Data & data = Data::getInstance();
    double delta, bestDelta = 0.0;
    int bestI = -1, bestJ = -1, i, j;
    bool improved = false;

    for (i = 1; i < data.n - 1; i++){
        for(j = i + 1; j < data.n; j++){
            delta = s.evaluateSwap(i, j);
            if(delta < bestDelta){
                bestDelta = delta;
                bestI = i;
                bestJ = j;
                improved = true;
            }
        }
    }

    if(improved){
        s.swap(bestI, bestJ);
    }

    return improved;
}



Solution perturbacao(const Solution &best) {
    Data & data = Data::getInstance();
    Solution perturbed;
    perturbed.copy(best);

    int n = data.n;
    if(n < 8) return perturbed;

    int maxSegmentSize = std::max(2, static_cast<int>(std::ceil(n / 10.0)));
    int p1, p2, p3;
    int tries = 0;
    do {
        p1 = 1 + rand() % (n - 6);
        p2 = p1 + 2 + rand() % std::max(1, std::min(maxSegmentSize, n - p1 - 4));
        p3 = p2 + 2 + rand() % std::max(1, std::min(maxSegmentSize, n - p2 - 2));
        tries++;
    } while (!(p1 < p2 && p2 < p3 && p3 < n) && tries < 100);

    if (!(p1 < p2 && p2 < p3 && p3 < n)) return perturbed;

    std::vector<int> newRoute;
    newRoute.insert(newRoute.end(), perturbed.route.begin(), perturbed.route.begin() + p1);
    newRoute.insert(newRoute.end(), perturbed.route.begin() + p2, perturbed.route.begin() + p3);
    newRoute.insert(newRoute.end(), perturbed.route.begin() + p1, perturbed.route.begin() + p2);
    newRoute.insert(newRoute.end(), perturbed.route.begin() + p3, perturbed.route.end());

    perturbed.route = newRoute;
    perturbed.cost = perturbed.recomputeCost();
    return perturbed;
}

Solution solve(){
    Solution s = Solution();
    s.buildTrivial();
    std::cout << "Solucao inicial:" << std::endl;
    s.print();
    std::cout << "Solucao final:" << std::endl;
    RVND(s);
    return s;




}