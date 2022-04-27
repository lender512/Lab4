#include "Set4Batches.hpp"
#include <chrono>
#include <fstream>

int64_t measure(int n, std::function<void(void)> f) {
  int64_t tiempo = 0;
  for (int _i = 0; _i < n; ++_i) {
    auto begin = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    tiempo += std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
                  .count();
  }
  return tiempo;
}

#define all for (int i = 0; i < N; ++i)

int main() { /* SPANGLICHHHH GAAAAA */

/* AMOUNT (REDUCE TO TEST FASTER) 135799*/
#define N 10000000
#define DEGREE 32

  /* HELPER DATA GENERATION*/
  std::vector<uint32_t> helper_data(N / DEGREE);
  {
    for (int i = 0; i < N / DEGREE; ++i)
      helper_data[i] = i * DEGREE;
  }

  /* LOAD DATA AND SORT*/
  std::vector<uint32_t> datos1(N);
  std::vector<uint32_t> datos2(N);
  {
    std::ifstream texto;
    texto.open("./output.txt");
    if (texto.is_open())
      all {
        texto >> datos1[i];
        datos1[i] *= 2;
      }
    all datos2[i] = datos1[i] + 1;
  }

  /* TEST PARALLEL LOOKUP + PARALLEL INSERT + PARALLEL ERASE */
  {
    Set4Batches<uint32_t, DEGREE, 4> tree(helper_data);
    tree.insert(datos1);
    tree.insert(datos2);
    const auto result1 = tree.contains(datos1);
    for (auto const &r : result1)
      if (r.exists == false)
        exit(0);
    const auto result2 = tree.contains(datos2);
    for (auto const &r : result2)
      if (r.exists == false)
        exit(0);
    tree.erase(datos1);
    const auto result3 = tree.contains(datos1);
    for (auto const &r : result3)
      if (r.exists)
        exit(0);
    printf("\nTEST PASSED!\n");
  }

  /* MEASURE LOOKUP*/
  {
    Set4Batches<uint32_t, DEGREE, 1> treeNP(helper_data); // Not parallel
    Set4Batches<uint32_t, DEGREE, 4> treeP(helper_data);  // Parallel
    treeNP.insert(datos1);
    treeP.insert(datos1);
    treeNP.insert(datos2);
    treeP.insert(datos2);

    auto not_parallel =
        measure(1, [&treeNP, &datos1] { auto r = treeNP.contains(datos1); });
    auto parallel =
        measure(1, [&treeP, &datos1] { auto r = treeP.contains(datos1); });
    auto diff = not_parallel - parallel;

    printf("\nLOOKUP\n");
    printf("Not parallel: \t %ld\n", not_parallel);
    printf("Parallel: \t %ld\n", parallel);
    printf("%%Reduction: \t %ld%%\n", (diff * 100) / not_parallel);
  }

  /* MEASURE INSERT*/
  {
    Set4Batches<uint32_t, DEGREE, 1> treeNP(helper_data); // Not parallel
    Set4Batches<uint32_t, DEGREE, 4> treeP(helper_data);  // Parallel
    treeNP.insert(datos1);
    treeP.insert(datos1);

    auto not_parallel =
        measure(1, [&treeNP, &datos2] { treeNP.insert(datos2); });
    auto parallel = measure(1, [&treeP, &datos2] { treeP.insert(datos2); });
    auto diff = not_parallel - parallel;

    printf("\nINSERT\n");
    printf("Not parallel: \t %ld\n", not_parallel);
    printf("Parallel: \t %ld\n", parallel);
    printf("%%Reduction: \t %ld%%\n", (diff * 100) / not_parallel);
  }

  /* MEASURE ERASE*/
  {
    Set4Batches<uint32_t, DEGREE, 1> treeNP(helper_data); // Not parallel
    Set4Batches<uint32_t, DEGREE, 4> treeP(helper_data);  // Parallel
    treeNP.insert(datos1);
    treeP.insert(datos1);
    treeNP.insert(datos2);
    treeP.insert(datos2);

    auto not_parallel =
        measure(1, [&treeNP, &datos1] { treeNP.erase(datos1); });
    auto parallel = measure(1, [&treeP, &datos1] { treeP.erase(datos1); });
    auto diff = not_parallel - parallel;

    printf("\nERASE\n");
    printf("Not parallel: \t %ld\n", not_parallel);
    printf("Parallel: \t %ld\n", parallel);
    printf("%%Reduction: \t %ld%%\n", (diff * 100) / not_parallel);
  }

  return 0;
}
