#include "solution.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <cstdlib>




double Solution::evaluateOrOpt(const int i, const int size, const int j){
    Data & data = Data::getInstance();
    int prev  = route[i - 1];
    int start = route[i];
    int end   = route[i + size - 1];
    int next  = route[i + size];
    int a = route[j];
    int b = route[j + 1];

    double removal   = data.matrizAdj[prev][next] - data.matrizAdj[prev][start] - data.matrizAdj[end][next];
    double insertion = data.matrizAdj[a][start] + data.matrizAdj[end][b] - data.matrizAdj[a][b]; // calcula a diferença de custo da solução após sofrer um movimento
    //o retorno é a soma de tudo, se o valor for negativo, é pq ta melhor 
    return removal + insertion;
}

void Solution::applyOrOpt(const int i, const int size, const int j){
    cost += evaluateOrOpt(i, size, j);

    std::vector<int> block(route.begin() + i, route.begin() + i + size);
    route.erase(route.begin() + i, route.begin() + i + size);

    int insertPos = j;
    if (j > i) insertPos -= size; //indices deslocaram após o erase

    route.insert(route.begin() + insertPos + 1, block.begin(), block.end());
}

double Solution::evaluate2opt(const int i, const int j){
    Data & data = Data::getInstance();
    double delta = data.matrizAdj[route[i-1]][route[j]] + data.matrizAdj[route[i]][route[j+1]]
                 - data.matrizAdj[route[i-1]][route[i]] - data.matrizAdj[route[j]][route[j+1]];
    return delta;
}

void Solution::apply2opt(const int i, const int j){
    cost += evaluate2opt(i, j);
    int a = i, b = j;
    while(a < b){
        std::swap(route[a], route[b]);
        a++; b--;
    }
}

void Solution::print(){
    std::cout << "Route: ";
    for(int i = 0; i < Data::getInstance().n; i++){
        std::cout << route[i] << " - ";
    }
    std::cout << route[Data::getInstance().n] << std::endl;
    std::cout << "Cost: " << cost << std::endl;
}

void Solution::copy(const Solution &other){
    route = std::vector<int>(other.route);
    cost = other.cost;
}

double Solution::recomputeCost(){
    Data & data = Data::getInstance();
    double c = 0;

    for(int i = 0; i < data.n; i++){
        c += data.matrizAdj[route[i]][route[i+1]];
    }

    return c;
}




void Solution::buildTrivial(){
    Data & data = Data::getInstance();
    int i;

    for(i = 1; i <= data.n; i++){
        route[i-1] = i;
    }
    route[data.n] = 1;
    cost = 0;
    for(i = 0; i < data.n - 1; i++){
        cost += data.matrizAdj[route[i]][route[i+1]];
    }
    cost += data.matrizAdj[route[data.n-1]][route[0]];
}


double Solution::evaluateSwap(const int i, const int j){
    Data & data = Data::getInstance();
    
    double  a_subtrair, a_somar, delta;

    if ((j == i + 1)){
        
        a_subtrair = data.matrizAdj[route[i-1]][route[i]] + data.matrizAdj[route[j]][route[j+ 1]]; // arcos que serão "cortados" da solução
        a_somar = data.matrizAdj[route[i -1]][route[j]] + data.matrizAdj[route[i]][route[j + 1]]; // arcos que serão "adicionados" na solução
        
    }
    else{
         a_subtrair = data.matrizAdj[route[i - 1]][route[i]] + data.matrizAdj[route[i]] [route[i + 1]] 
                    + data.matrizAdj[route[j-1]][route[j]] + data.matrizAdj[route[j]][route[j+1]];

        a_somar = data.matrizAdj[route[i - 1]][route[j]] + data.matrizAdj[route[j]] [route[i + 1]] 
                    + data.matrizAdj[route[j - 1]][route[i]] + data.matrizAdj[route[i]][route[j+1]];
        
    }

    delta = a_somar - a_subtrair; // calcula a diferença de custo da solução após sofrer um movimento
    
    //
    
    return delta;
}

void Solution::swap(const int i, const int j){
    Data & data = Data::getInstance();
    cost += evaluateSwap(i, j); // somar a diferença de custo da solução após sofrer um movimento 
                                //implica em atualizar o custo da solução
    int aux = route[i];
    route[i] = route[j];
    route[j] = aux;
    
}

void Solution::buildGreedyRandomized(double alpha){
    Data & data = Data::getInstance();
    int n = data.n;

    alpha = std::max(0.0, std::min(1.0, alpha));
    // monta a lista de TODAS as cidades exceto a 1 , pq ja foi passada no inicio 
    std::vector<int> allNodes;
    for(int node = 2; node <= n; node++) allNodes.push_back(node);
    //vai tentar misturar

    std::mt19937 generator(static_cast<unsigned int>(rand()));  //suigestão da ia pq tava dando erro (foi algo sobre a versão do c++) tenho que ciar o generaton em outro canto
    std::shuffle(allNodes.begin(), allNodes.end(), generator);
    //tava no kit
    this->route.clear();
    route.push_back(1);
    route.push_back(allNodes[0]);
    route.push_back(allNodes[1]);
    route.push_back(allNodes[2]);
    route.push_back(1);

    std::vector<int> candidates(allNodes.begin() + 3, allNodes.end());

    while(!candidates.empty()){
        std::vector<InsertionInfo> insertions;

        for(size_t edgePos = 0; edgePos + 1 < route.size(); edgePos++){
            int from = route[edgePos];
            int to = route[edgePos + 1];

            for(int node : candidates){
                double delta = data.matrizAdj[from][node]  // delta = c[i][k] + c[k][j] - c[i][j] 
                    // "quanto a rota piora se eu inserir k entre from e to"
                    + data.matrizAdj[node][to]
                    - data.matrizAdj[from][to];
                insertions.push_back({node, static_cast<int>(edgePos), delta});
            }
        }
      // Ordena Ωem ordem crescente de delta os melhores (menor custo
        // de inserção) ficam no início 
        std::sort(insertions.begin(), insertions.end(),
            [](const InsertionInfo &left, const InsertionInfo &right){
                return left.cost < right.cost;
            });
            //pega só os alpha melhores
        int rclSize = std::max(1, static_cast<int>(alpha * insertions.size()));
        InsertionInfo selected = insertions[rand() % rclSize];

        route.insert(route.begin() + selected.edgePos + 1, selected.node);
        candidates.erase(std::remove(candidates.begin(), candidates.end(), selected.node), //vai remover quando for inserindo nao sei se faz mt sentiddo ser desse jeito mas ok
                         candidates.end());
    }

    cost = recomputeCost();
}