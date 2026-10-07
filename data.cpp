#include "data.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cmath>

using namespace std;

double CalcDistEuc ( double *X, double *Y, int I, int J );
double CalcDistAtt ( double *X, double *Y, int I, int J );
void CalcLatLong ( double *X, double *Y, int n, double *latit, double* longit );
double CalcDistGeo ( double *latit, double *longit, int I, int J );

// essa eh a funcao monstra que le o arquivo de texto (.tsp) e extrai tudo
void readData( int argc, char** argv, int* Dimension, double ***Mdist )
{
     // se voce rodou o programa mas esqueceu de passar o arquivo, ele reclama e morre, tem umas coisas meio inutil mas vai
     if (argc < 2) {
         cout << "\nFaltando parametros\n";
         cout << " ./exec [Instancia] "<< endl;
         exit(1);
     }

     // se voce passou coisa demais no terminal, ele tambem reclama e morre
     if (argc > 2) {
          cout << "\nMuitos parametros\n";
          cout << " ./exec [Instancia] " << endl;
          exit(1);
     }

    // o n guarda quantas cidades tem. comeca zerado pra nao pegar lixo da memoria do pc
    int N = 0; 
    string arquivo, ewt;

    // pega o nome do arquivo que vc digitou no terminal e tenta abrir
    char *instancia = argv[1];
    ifstream in( instancia, ios::in);

    // se o arquivo nao existir ou o nome tiver errado, ele da erro fatal
    if (!in) {
        cout << "ERRO FATAL: arquivo nao pode ser aberto\n";
        exit(1);
    }

    cout << "[DEBUG] Lendo arquivo... Procurando DIMENSION..." << endl;

    // fica lendo o arquivo palavra por palavra ate esbarrar na palavra "DIMENSION"
    while ( in >> arquivo ) {
        if (arquivo == "DIMENSION:" || arquivo == "DIMENSION") {
            break; // achou! para de procurar
        }
    }
    
    // as vezes o arquivo tem os dois pontos colados, as vezes soltos. aqui ele arruma isso
    if ( arquivo == "DIMENSION" )  in >> arquivo;
    in >> N; // salva o numero de cidades na variavel n

    cout << "[DEBUG] Dimensao (N) lida: " << N << endl;

    // se ele leu que tem zero cidades ou numero negativo, o arquivo ta quebrado
    if (N <= 0) {
        cout << "ERRO FATAL: Dimensao lida invalida ou nula. Verifique a formatacao do TSP!" << endl;
        exit(1);
    }

    // agora ele volta a ler palavra por palavra ate achar como as distancias sao calculadas
    while ( in >> arquivo ) {
        if (arquivo == "EDGE_WEIGHT_TYPE:" || arquivo == "EDGE_WEIGHT_TYPE") {
            break;
        }
    }
    
    // salva o tipo de calculo na variavel ewt (ex: euc_2d, explicit, geo)
    if ( arquivo == "EDGE_WEIGHT_TYPE" )  in >> arquivo;
    in >> ewt;

    cout << "[DEBUG] Tipo de peso lido: " << ewt << endl;

    // cria vetores pra guardar as coordenadas de x e y de cada cidade
    // ele bota n+1 porque os mapas do tsp comecam a contar do 1 e nao do zero
    double *x = new double [N+1];
    double *y = new double [N+1];

    // aqui ele cria a matriz principal que vai guardar a distancia de todo mundo pra todo mundo
    double **dist = new double*[N+1];

    for ( int i = 0; i < N+1; i++ ) {
        dist [i] = new double [N+1];
    }
    
    cout << "[DEBUG] Matriz alocada com sucesso!" << endl;
    
    // se as distancias ja vierem prontas/escritas no arquivo (explicit)
    if ( ewt == "EXPLICIT" ) {

        // ele procura qual o formato que essa matriz ta desenhada no texto
        while ( arquivo.compare("EDGE_WEIGHT_FORMAT:") != 0 && arquivo.compare("EDGE_WEIGHT_FORMAT" ) != 0 ) {
            in >> arquivo;
        }

        string ewf;
        if ( arquivo.compare("EDGE_WEIGHT_FORMAT" ) == 0 )  in >> arquivo;
        in >> ewf;

        if ( ewf == "FUNCTION" ) {
            cout << "FUNCTION - Nao suportado!" << endl; }

        // se a matriz inteira (ida e volta) tiver escrita no arquivo
        else if ( ewf == "FULL_MATRIX" ) {

            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) {
                in >> arquivo;
            }

            // preenche a matriz lendo linha por linha, coluna por coluna
            for ( int i = 1; i < N+1; i++ ) {
                for ( int j = 1; j < N+1; j++ ) {
                    in >> dist[i][j];
                }
            }
        }

        // se no arquivo so tiver a metade de cima da matriz (ja que a distancia de a->b é igual de b->a)
        else if ( ewf == "UPPER_ROW" ) {

            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) {
                in >> arquivo;
            }

            for ( int i = 1; i < N; i++ ) {
                for ( int j = i+1; j < N+1; j++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j]; // espelha a distancia pro outro lado da matriz
                }
            }

            // a distancia de uma cidade pra ela mesma é zero
            for ( int i = 1; i < N+1; i++ ) {
                dist[i][i] = 0;
            }

        }

        // mesma coisa que o de cima, mas se no arquivo so tiver a metade de baixo da matriz
        else if ( ewf == "LOWER_ROW" ) {

            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) {
                in >> arquivo;
            }

            for ( int i = 2; i < N+1; i++ ) {
                for ( int j = 1; j < i; j++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j]; // espelha
                }
            }

            for ( int i = 1; i < N+1; i++ ) {
                dist[i][i] = 0;
            }
        }

        // e aqui sao outras variacoes de como a matriz pode vir desenhada no arquivo
        // a logica eh sempre a mesma: ler o que tem e espelhar o resto
        else if ( ewf == "UPPER_DIAG_ROW" ) {
            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) { in >> arquivo; }
            for ( int i = 1; i < N+1; i++ ) {
                for ( int j = i; j < N+1; j++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j];
                }
            }
        }

        else if ( ewf == "LOWER_DIAG_ROW" ) {
            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) { in >> arquivo; }
            for ( int i = 1; i < N+1; i++ ) {
                for ( int j = 1; j <= i; j++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j];
                }
            }
        }

        else if ( ewf == "UPPER_COL" ) {
            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) { in >> arquivo; }
            for ( int j = 2; j < N+1; j++ ) {
                for ( int i = 1; i < j; i++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j];
                }
            }
            for ( int i = 1; i < N+1; i++ ) { dist[i][i] = 0; }
        }

        else if ( ewf == "LOWER_COL" ) {
            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) { in >> arquivo; }
            for ( int j = 1; j < N; j++ ) {
                for ( int i = j+1; i < N+1; i++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j];
                }
            }
            for ( int i = 1; i < N+1; i++ ) { dist[i][i] = 0; }
        }

        else if ( ewf == "UPPER_DIAG_COL" ) {
            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) { in >> arquivo; }
            for ( int j = 1; j < N+1; j++ ) {
                for ( int i = 1; i <= j; i++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j];
                }
            }
        }

        else if ( ewf == "LOWER_DIAG_COL" ) {
            while ( arquivo.compare("EDGE_WEIGHT_SECTION") != 0 ) { in >> arquivo; }
            for ( int j = 1; j < N+1; j++ ) {
                for ( int i = j; i < N+1; i++ ) {
                    in >> dist[i][j];
                    dist[j][i] = dist[i][j];
                }
            }
        }
    }

    // o formato mais comum! se as cidades forem coordenadas num plano cartesiano
    else if ( ewt == "EUC_2D" ) {

        while ( arquivo.compare("NODE_COORD_SECTION") != 0 ) {
            in >> arquivo;
        }
        
        // ele le o numero da cidade, e as coordenadas de x e de y dela
        int tempCity;
        for ( int i = 1; i < N+1; i++ ) {
            in >> tempCity >> x[i] >> y[i];
        }

        // aqui a magica acontece: ele preenche a sua matriz usando a formula da euclidiana
        // o floor arredonda pra baixo garantindo q seja inteiro padrao tsplib
        for ( int i = 1; i < N+1; i++ ) {
            for ( int j = 1; j < N+1; j++ ) {
                dist[i][j] = floor ( CalcDistEuc ( x, y, i, j ) + 0.5 );
            }
        }
    }

    // um monte de formato esquisito que o seu codigo avisa que nao aceita e pula
    else if ( ewt == "EUD_3D" ) { cout << "EUC_3D - Nao suportado!" << endl; }
    else if ( ewt == "MAX_2D" ) { cout << "MAX_2D - Nao suportado!" << endl; }
    else if ( ewt == "MAX_3D" ) { cout << "MAX_3D - Nao Suportado!" << endl; }
    else if ( ewt == "MAN_2D" ) { cout << "MAN_2D - Nao suportado!" << endl; }
    else if ( ewt == "MAN_3D" ) { cout << "MAN_3D - Nao Suportado!" << endl; }

    // igual ao euc_2d la de cima, mas em vez de arredondar normal, o ceil joga sempre pra cima
    else if ( ewt == "CEIL_2D" ) {

        while ( arquivo.compare("NODE_COORD_SECTION") != 0 ) {
            in >> arquivo;
        }
        
        int tempCity;
        for ( int i = 1; i < N+1; i++ ) {
            in >> tempCity >> x[i] >> y[i];
        }

        for ( int i = 1; i < N+1; i++ ) {
            for ( int j = 1; j < N+1; j++ ) {
                dist[i][j] = ceil ( CalcDistEuc ( x, y, i, j ) );
            }
        }
    }

    // se o mapa for geografico (tipo o burma14 e ali535), usando latitude e longitude reais da terra
    else if ( ewt == "GEO" ) {

        while ( arquivo.compare("NODE_COORD_SECTION") != 0 ) {
            in >> arquivo;
        }
        
        int tempCity;
        for ( int i = 1; i < N+1; i++ ) {
            in >> tempCity >> x[i] >> y[i];
        }

        double *latitude = new double [N+1];
        double *longitude = new double [N+1];

        // chama uma funcao pra converter o x e y estranho da tsplib pra latitude e longitude certinha
        CalcLatLong ( x, y, N, latitude, longitude );

        // calcula a distancia considerando a curvatura do planeta
        for ( int i = 1; i < N+1; i++ ) {
            for ( int j = 1; j < N+1; j++ ) {
                dist[i][j] = CalcDistGeo ( latitude, longitude, i, j );
            }
        }
    }

    // formato de pseudo-distancia euclidiana (muito especifico de um mapa chamado att48)
    else if ( ewt == "ATT" ) {

        while ( arquivo.compare("NODE_COORD_SECTION") != 0 ) {
            in >> arquivo;
        }

        int tempCity;
        int *tempX = new int [N+1];
        int *tempY = new int [N+1];

        for ( int i = 1; i < N+1; i++ ) {
            in >> tempCity >> tempX[i] >> tempY[i];
            x[i]=tempX[i];
            y[i]=tempY[i];
        }

        for ( int i = 1; i < N+1; i++ ) {
            for ( int j = 1; j < N+1; j++ ) {
                dist[i][j] = CalcDistAtt ( x, y, i, j );
            }
        }
    }

    else if ( ewt == "XRAY1" ) { cout << "XRAY1 - Nao suportado!" << endl; }
    else if ( ewt == "XRAY2" ) { cout << "XRAY2 - Nao suportado!" << endl; }
    else if ( ewt == "SPECIAL" ) { cout << "SPECIAL - Nao suportado!" << endl; }

    // no final de tudo, ele guarda a quantidade de cidades e a matriz monstra nos ponteiros oficiais
    *Dimension = N;
    *Mdist = dist;
}

// funcao de pitagoras puro: acha a hipotenusa pra saber a distancia em linha reta
double CalcDistEuc ( double *X, double *Y, int I, int J )
{
    return sqrt ( pow ( X[I] - X[J], 2 ) + pow ( Y[I] - Y[J], 2 ) );
}

// formula esquisita que o criador do mapa att48 inventou. ele divide por 10 e faz um arredondamento bizarro
double CalcDistAtt ( double *X, double *Y, int I, int J )
{
    double rij, tij, dij;
    rij = sqrt ( ( pow ( X[I] - X[J], 2 ) + pow ( Y[I] - Y[J], 2 ) ) / 10 );
    tij = floor ( rij + 0.5 );

    if ( tij < rij )
        dij = tij + 1;
    else
        dij = tij;

    return dij;
}

// converte os numeros soltos de x e y para radianos (pra poder usar seno e cosseno depois)
void CalcLatLong ( double *X, double *Y, int n, double *latit, double* longit )
{
    double PI = 3.141592, min;
    int deg;

    for ( int i = 1; i < n+1; i++ ) {
        deg = (int) X[i]; // pega so a parte inteira do numero (os graus)
        min = X[i] - deg; // pega so a parte quebrada (os minutos)
        latit[i] = PI * (deg + 5.0 * min / 3.0 ) / 180.0; // transforma tudo numa sopa de radiano
    }

    for ( int i = 1; i < n+1; i++ ) {
        deg = (int) Y[i];
        min = Y[i] - deg;
        longit[i] = PI * (deg + 5.0 * min / 3.0 ) / 180.0;
    }
}

// calcula a distancia na superficie de uma esfera (a terra). usa o raio do planeta de 6378 km.
double CalcDistGeo ( double *latit, double *longit, int I, int J )
{
    double q1, q2, q3, RRR = 6378.388;

    q1 = cos( longit[I] - longit[J] );
    q2 = cos( latit[I] - latit[J] );
    q3 = cos( latit[I] + latit[J] );

    return (int) ( RRR * acos( 0.5*((1.0+q1)*q2 - (1.0-q1)*q3) ) + 1.0);
}

// comeca as coisas da sua classe data

// garante que a classe nasce nula
Data * Data::instance = nullptr;

// construtor zerando tudo pra nao ter lixo de memoria
Data::Data(){
    this->n = -1;
    this->matrizAdj = nullptr;
}

// padrao singleton! isso é coisa de dev senior.
// ele garante que se o codigo pedir os "dados" mil vezes, ele nao vai criar mil copias, vai sempre devolver a mesma original.
Data & Data::getInstance()
{
    if(instance == nullptr) instance = new Data();
    return *instance;
}

// funcao amigavel que o seu main.cpp chama. ela junta todas aquelas funcoes gigantes ali de cima
// chama o readdata pra fazer o trabalho sujo e depois guarda o resultado nas variaveis oficiais da classe.
void Data::read(int argc, char **argv)
{
    int dimension;
    double **matrizAdj;

    readData(argc, argv, &dimension, &matrizAdj);
    instance->n = dimension;
    instance->matrizAdj = matrizAdj;
}