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


    /* LOAD DATA AND SORT*/
    std::vector<uint32_t> datos1(N);
    std::vector<uint32_t> datos2(N);
    {
        std::ifstream texto;
        texto.open("./output.txt");
        if (texto.is_open()) all {texto >> datos1[i]; datos1[i]*=2;}
        std::sort(datos1.begin(), datos1.end());
        all datos2[i] = datos1[i] + 1;
    }


    /* TEST PARALLEL LOOKUP + PARALLEL INSERT */
    {
        BPTree<3> tree;
        for(auto& e : datos1) tree.insert(e); // No paralelo pues esta vacio el arbol
        tree.insertMultiple(datos2); // Paralelo pues ya tiene por lo menos dos niveles
        const auto result1 = tree.containsMultiple(datos1); for(auto b : result1) if(b == 0) exit(b);
        const auto result2 = tree.containsMultiple(datos2); for(auto b : result2) if(b == 0) exit(b);
        printf("\nTEST PASSED!\n");
    }


    /* MEASURE LOOKUP*/
    {
        BPTree<3> treeNP; // Not parallel
        BPTree<3> treeP(4); // Parallel
        treeNP.insertMultiple(datos1);
        treeP.insertMultiple(datos1);

        auto not_parallel = measure(1,[&treeNP, &datos1]{auto r = treeNP.containsMultiple(datos1);});
        auto parallel = measure(1,[&treeP, &datos1]{auto r = treeP.containsMultiple(datos1);});
        auto diff = not_parallel - parallel;

        printf("\nLOOKUP\n");
        printf("Not parallel: \t %ld\n", not_parallel);
        printf("Parallel: \t %ld\n", parallel);
        printf("%%Reduction: \t %ld%%\n", (diff*100)/not_parallel);
    }


    /* MEASURE INSERT*/
    {
        BPTree<3> treeNP; // Not parallel
        BPTree<3> treeP(4); // Parallel
        treeNP.insertMultiple(datos1);
        treeP.insertMultiple(datos1);

        auto not_parallel = measure(1,[&treeNP, &datos2]{treeNP.insertMultiple(datos2);});
        auto parallel = measure(1,[&treeP, &datos2]{treeP.insertMultiple(datos2);});
        auto diff = not_parallel - parallel;

        printf("\nLOOKUP\n");
        printf("Not parallel: \t %ld\n", not_parallel);
        printf("Parallel: \t %ld\n", parallel);
        printf("%%Reduction: \t %ld%%\n", (diff*100)/not_parallel);
    }

    
    
    return 0;
}

