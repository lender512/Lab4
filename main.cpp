#include <fstream>
#include <chrono>
#include <functional>
#include "BPTree.hpp"


int64_t measure(int n, std::function<void(void)> f){
    int64_t tiempo = 0;                                                                         
    for(int _i = 0; _i<n; ++_i){                                                                
        auto begin = std::chrono::steady_clock::now();                                          
        f();                                                                                    
        auto end = std::chrono::steady_clock::now();                                            
        tiempo += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    }
    return tiempo;                
}

#define all for(int i = 0; i<N; ++i)

int main() { /* SPANGLICHHHH GAAAAA */

    /* AMOUNT (REDUCE TO TEST FASTER) 135799*/
    #define N 1000000


    /* LOAD DATA */
    std::vector<uint32_t> datos1(N);
    std::vector<uint32_t> datos2(N);
    {
        std::ifstream texto;
        texto.open("./output.txt");
        if (texto.is_open()) all {texto >> datos1[i]; datos1[i]*=2;}
        all datos2[i] = datos1[i] + 1;
        texto.close();
    }


    /* TEST PARALLEL LOOKUP + PARALLEL INSERT */
    {
        BPTree<3> tree(4);
        tree.insertMultiple(datos1); // No paralelo pues esta vacio el arbol
        tree.insertMultiple(datos2); // Paralelo pues ya tiene por lo menos dos niveles
        auto result = tree.containsMultiple(datos1); for(auto b : result) if(!b) exit(-1);
        result = tree.containsMultiple(datos2); for(auto b : result) if(!b) exit(-1);
        printf("\nTEST PASSED!\n");
    }


    /* MEASURE LOOKUP*/
    {
        BPTree<3> treeNP; // Not parallel
        BPTree<3> treeP(4); // Parallel
        auto not_parallel = measure(10,[&treeNP, &datos1]{auto r = treeNP.containsMultiple(datos1);});
        auto parallel = measure(10,[&treeP, &datos1]{auto r = treeP.containsMultiple(datos1);});
        auto diff = not_parallel - parallel;

        printf("\nLOOKUP\n");
        printf("Not parallel: \t %ld\n", not_parallel);
        printf("Parallel: \t %ld\n", parallel);
        printf("%%Reduction: \t %ld%%\n", (diff*100)/not_parallel);
    }


    /* MEASURE INSERT*/
    {
        int64_t not_parallel = 0, parallel = 0;
        for(int _i = 0; _i = 10; ++_i){
            BPTree<3> treeNP; // Not parallel
            BPTree<3> treeP(4); // Parallel
            treeNP.insertMultiple(datos1); // No paralelo pues esta vacio el arbol
            treeP.insertMultiple(datos1);
             
            not_parallel += measure(1,[&treeNP, &datos2]{treeNP.insertMultiple(datos2);});
            parallel += measure(1,[&treeP, &datos2]{treeP.insertMultiple(datos2);});
               
        }
        auto diff = not_parallel - parallel;
        printf("\nINSERT\n");
        printf("Not parallel: \t %ld\n", not_parallel);
        printf("Parallel: \t %ld\n", parallel);
        printf("%%Reduction: \t %ld%%\n", (diff*100)/not_parallel);
    }

    
    
    return 0;
}

