#include "Set4Batches.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>

int64_t measure(int n, std::function<void(void)> f) {
  int64_t time = 0;
  for (int _i = 0; _i < n; ++_i) {
    auto begin = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    time += std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
                .count();
  }
  return time / n;
}

int main(int argc, char **argv) {
  if ({{EXPERIMENT}} == 1) {
    int N = 10000000;
    std::vector<uint32_t> helper_data(N / {{DEGREE}});

    {
      for (int i = 0; i < N / {{DEGREE}}; ++i)
        helper_data[i] = i * {{DEGREE}};
    }

    std::vector<uint32_t> data(N);

    {
      std::ifstream texto;
      texto.open("{{FILE}}");
      if (texto.is_open())
        for (int i = 0; i < N; i++) {
          texto >> data[i];
        }
    }

    int sort = {{SORT}}; // 0 - unordered 1 - ordered 2 - reverse ordered
                         //
    if (sort == 1) {
      std::sort(data.begin(), data.end());
    }

    if (sort == 2) {
      std::sort(data.begin(), data.end());
      std::reverse(data.begin(), data.end());
    }

    Set4Batches<uint32_t, {{DEGREE}}, {{THREADS}}> treeP(helper_data);
    for (int i = 0; i < N; i += 1000) {
      // create a sub vector of 1000 elements
      auto parallel = measure(1, [&treeP, &data, i] {
        std::vector<uint32_t> subVector(data.begin() + i,
                                        data.begin() + i + 1000);
        treeP.insert(subVector);
      });
      std::cout << parallel << std::endl;
    }
  }

  if ({{EXPERIMENT}} == 2) {
    int N = 10000000;
    std::vector<uint32_t> helper_data(N / {{DEGREE}});

    {
      for (int i = 0; i < N / {{DEGREE}}; ++i)
        helper_data[i] = i * {{DEGREE}};
    }

    std::vector<uint32_t> data(N);

    {
      std::ifstream texto;
      texto.open("{{FILE}}");
      if (texto.is_open())
        for (int i = 0; i < N; i++) {
          texto >> data[i];
        }
    }

    Set4Batches<uint32_t, {{DEGREE}}, {{THREADS}}> treeP(helper_data);
    for (int i = 0; i < N; i += 1000000) {
      // create a sub vector of 1000000 elements
      std::vector<uint32_t> subVector(data.begin() + i,
                                      data.begin() + i + 1000000);
      treeP.insert(subVector);
      auto parallel = measure(
          1, [&treeP, &subVector] { auto r = treeP.contains(subVector); });
      std::cout << parallel << std::endl;
    }
  }

  if ({{EXPERIMENT}} == 3) {
    int N = 10000000;
    std::vector<uint32_t> helper_data(N / {{DEGREE}});

    {
      for (int i = 0; i < N / {{DEGREE}}; ++i)
        helper_data[i] = i * {{DEGREE}};
    }

    std::vector<uint32_t> data(N);

    {
      std::ifstream texto;
      texto.open("{{FILE}}");
      if (texto.is_open())
        for (int i = 0; i < N; i++) {
          texto >> data[i];
        }
    }

    Set4Batches<uint32_t, {{DEGREE}}, {{THREADS}}> treeP(helper_data);
    treeP.insert(data);

    for (int i = 0; i < N; i += 1000000) {
      // create a sub vector of 1000000 elements
      std::vector<uint32_t> subVector(data.begin() + i,
                                      data.begin() + i + 1000000);
      auto parallel =
          measure(1, [&treeP, &subVector] { treeP.erase(subVector); });
      std::cout << parallel << std::endl;
    }
  }

  if ({{EXPERIMENT}} == 4) {
    int N = 10000000;
    std::vector<uint32_t> helper_data(N / {{DEGREE}});

    {
      for (int i = 0; i < N / {{DEGREE}}; ++i)
        helper_data[i] = i * {{DEGREE}};
    }

    std::vector<uint32_t> data(N);

    {
      std::ifstream texto;
      texto.open("{{FILE}}");
      if (texto.is_open())
        for (int i = 0; i < N; i++) {
          texto >> data[i];
        }
    }

    Set4Batches<uint32_t, {{DEGREE}}, {{THREADS}}> treeP(helper_data);
    treeP.insert(data);
    auto parallel =
        measure(1, [&treeP, &data] { auto r = treeP.contains(data); });
    std::cout << parallel / 1000 << std::endl;
  }
}
