#include <fstream>
#include <chrono>
#include "BPTreeS.hpp"


#define measure(_f){auto begin = std::chrono::steady_clock::now(); _f auto end = std::chrono::steady_clock::now(); auto tiempo = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count(); std::cout << tiempo << std::endl;}
#define all for(int i = 0; i<N; ++i)

int main() { /* SPANGLICHHHH GAAAAA */

    /* AMOUNT (REDUCE TO TEST FASTER) */
    #define N 100


    /* READ */    
    std::ifstream texto;
    texto.open("./output.txt");
    std::vector<uint32_t> datos(N);
    if (texto.is_open()) all texto >> datos[i];


    /* INSERT (NOT PARALLEL) */
    BPTree tree(1024*128, N, 3);
    all tree.insert( datos[i] );


    /* PARALLEL LOOKUP:  ---- > DEBUG THIS <---- */
    //std::vector<uint32_t> v = {0,1,2,3,4,5,6};
    std::sort(datos.begin(), datos.end());
    // for(auto b : datos) std::cout << b << std::endl;

    auto result = tree.containsMultiple(datos);
    for(auto b : result) std::cout << b << std::endl;


    /* NOT PARALLEL VS PARALLEL TIMES (UNCOMMENT WHEN WORKS CORRECTLY) */
    // measure(std::vector<bool> v(N, false); all v[i] = tree.contains(datos[i]);)
    // measure(std::sort(datos.begin(), datos.end());)
    // measure(auto result = tree.containsMultiple(datos);)
    
    exit(0);
}

