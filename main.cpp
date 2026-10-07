#include "data.h"
#include <time.h>
#include <iostream>
#include <cmath>

#include "localSearch.h"

int main(int argc, char** argv) {

    srand(time(NULL)); ///maneira de aleatoizar o alpha com base no relogio (uau)
    Data & data = Data::getInstance(); //singleton , construtor privado
    
    data.read(argc, argv);

    //solution 
    int maxIterIls = (data.n >= 150) ? data.n / 2 : data.n; 
    Solution s = ILS(50, maxIterIls); ///posso mudar dps o limite 

    s.print(); 

    if (s.recomputeCost() != s.cost) {
        std::cout << "custo acumulado diferente do custo recomputado" << std::endl;
    }

    return 0;
}

Solution ILS(int maxIter, int maxIterIls){  ///max global e o max de acordo com cada instancia tipo instancia tem x pontos ent é max x
    
    ///criando variaveis
    
    Solution bestOfAll;
    bestOfAll.cost = INFINITY; ///gambiarra tipo e se eu colcoar 999999999999999999 kkkkkkkkkkkkkkk funciona? 

    for(int i = 0; i < maxIter; i++){
        Solution s = Solution();
        double alpha = static_cast<double>(rand()) / RAND_MAX;  ///lembrando que o alpha diz o quao guloso é 
        s.buildGreedyRandomized(alpha);
        Solution best = s;

        int iterIls = 0;
        while(iterIls < maxIterIls){  ///enquanto o número de tentativas seguidas sem sucesso (iterIls) for menor que o limite de paciência 
            RVND(s);
            if(s.cost < best.cost){
                best.copy(s);
                iterIls = 0;
            }
            s = perturbacao(best);
            iterIls++;
        }
        if(best.cost < bestOfAll.cost) bestOfAll.copy(best);
    }
    return bestOfAll;
}