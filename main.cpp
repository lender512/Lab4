#include <fstream>
#include <chrono>
#include "BPTreeS.hpp"


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

    /* AMOUNT (REDUCE TO TEST FASTER) */
    #define N 1000000


    /* READ */    
    std::ifstream texto;
    texto.open("./output.txt");
    std::vector<uint32_t> datos(N);
    if (texto.is_open()) all texto >> datos[i];


    /* INSERT (NOT PARALLEL) */
    BPTree tree(1024*128, 3);
    all tree.insert( datos[i] );
    std::sort(datos.begin(), datos.end());


    /* PARALLEL LOOKUP TEST:  ALL ONES, WORKS CORRECTLY*/
    // auto result = tree.containsMultiple(datos);
    // for(auto b : result) std::cout << b << '\n';
    // std::cout << std::flush;


    /* NOT PARALLEL VS PARALLEL TIMES */
    auto not_parallel = measure(10,[&tree, &datos]{std::vector<int> v(N, 0); all v[i] = tree.contains(datos[i]);});
    auto parallel = measure(10,[&tree, &datos]{auto result = tree.containsMultiple(datos);});
    auto diff = not_parallel - parallel;

    printf("Not parallel: \t %ld\n", not_parallel);
    printf("Parallel: \t %ld\n", parallel);
    printf("A. gain: \t %ld\n", diff);
    printf("R. gain: \t %ld%%\n", (diff*100)/not_parallel);
    
    exit(0);
}

