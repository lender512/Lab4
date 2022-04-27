#include <algorithm>
#include <atomic>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#define SIZE(i) (i->size)
#define KEY(i, j) (i->key[j])
#define CHILD(i, j) (i->child[j])
#define PARENT(i) (i->parent)
#define IS_LEAF(i) (i->child[0] == null)
#define IS_NOT_LEAF(i) (i->child[0] != null)
#define NEXT(i) (i->child[degree])
#define CAN_GIVE(i) (SIZE(i) > min)
#define DIRTY(i, j) i->dirty[j]
#define NOT_DIRTY(i, j) !(i->dirty[j])
#define SET_DIRTY(i, j) i->dirty[j] = 1
#define SET_NOT_DIRTY(i, j) i->dirty[j] = 0

template <typename Key, unsigned degree, unsigned workers> class Set4Batches {
public:
  struct KeyExistence {
    Key key;
    bool exists;
  };
  // 121904
private:
  using Size = uint32_t;
  static constexpr Size capacity = degree - 1;
  static constexpr Size mid = degree / 2;
  static constexpr Size min = (degree / 2) + (degree % 2) - 1;
  static constexpr Size max_lvl = 22;
  static constexpr Size dirty_bitset_sz =
      (degree / 32) + 32 * (degree % 32 != 0);

  struct Node {
    Size size;
    Key key[capacity + 1];
    Node *child[degree + 1] = {0};
    std::bitset<capacity + 1> dirty;
  };
  using Pointer = Node *;
  static constexpr Pointer null = 0;

  Pointer root;
  unsigned free_workers;
  std::atomic<unsigned> size;
  std::atomic<unsigned> new_subroots;
  std::atomic<unsigned> erased;

  void deleteAll(Pointer current) {
    if (current == null)
      return;
    for (int i = 0; i < SIZE(current); ++i)
      deleteAll(CHILD(current, i));
    delete current;
  }

  void insert_r(std::vector<Key> const &keys, Pointer &sub_root) {
    for (auto const &key : keys) {
      if (sub_root == null) {
        sub_root = new Node;
        SIZE(sub_root) = 1;
        KEY(sub_root, 0) = key;
        continue;
      } // Si no hay root

      int level = 0;           // Head del stack
      Pointer parent[max_lvl]; // Stack de parents
      Pointer current = sub_root;
      while (IS_NOT_LEAF(current)) {
        parent[level++] = current;
        int i = 0;
        for (; i < SIZE(current); ++i) {
          if (key < KEY(current, i))
            break;
        }
        current = CHILD(current, i);
      } // Encontrar nodo hoja y el camino de parents

      bool was_present = false;
      for (int i = 0; i < SIZE(current); ++i) {
        if (key < KEY(current, i))
          break;
        else if (key == KEY(current, i)) {
          if (DIRTY(current, i))
            SET_NOT_DIRTY(current, i);
          else
            size.fetch_sub(1);
          was_present = true;
          break;
        }
      }
      if (was_present)
        continue;

      int i = SIZE(current) - 1;
      SIZE(current) += 1;
      while (i >= 0 && key < KEY(current, i)) {
        KEY(current, i + 1) = KEY(current, i);
        DIRTY(current, i + 1) = DIRTY(current, i);
        i -= 1;
      }
      KEY(current, i + 1) = key; // Insertar el key
      SET_NOT_DIRTY(current, i + 1);

      if (SIZE(current) <= capacity)
        continue; // Si no hay overflow, termina

      SIZE(current) = mid;
      Pointer other_half;
      other_half = new Node;
      SIZE(other_half) = degree - mid;
      memcpy(&(KEY(other_half, 0)), &(KEY(current, mid)),
             sizeof(Key) * (degree - mid));
      for (int j = 0; j < (degree - mid); ++j) {
        DIRTY(other_half, j) = DIRTY(current, mid + j);
        SET_NOT_DIRTY(current, mid + j);
      }
      NEXT(other_half) = NEXT(current);
      NEXT(current) = other_half;
      // Se hace split y el centro se queda en el nodo hoja y tambien sube
      // Se actualiza la linked list

      do {

        if (current == sub_root) {
          sub_root = new Node;
          SIZE(sub_root) = 1;
          KEY(sub_root, 0) = KEY(current, SIZE(current));
          CHILD(sub_root, 0) = current;
          CHILD(sub_root, 1) = other_half;
          if (workers > 1)
            new_subroots.fetch_add(1);
          break;
        } // Si es el root, se crear otro root y termina

        Pointer child = other_half;
        Key k = KEY(current, SIZE(current)); // Se obtiene la key que sube
        current = parent[--level];           // Se atiende al padre
        int i = SIZE(current) - 1;
        SIZE(current) += 1;
        while (i >= 0 && key < KEY(current, i)) {
          KEY(current, i + 1) = KEY(current, i);
          CHILD(current, i + 2) = CHILD(current, i + 1);
          i -= 1;
        }
        KEY(current, i + 1) = k;
        CHILD(current, i + 2) = other_half; // Se inserta la key y el hijo

        if (SIZE(current) <= capacity)
          break; // Si no hay overflow, termina

        SIZE(current) = mid;
        other_half = new Node;
        SIZE(other_half) = degree - mid - 1;
        memcpy(&(KEY(other_half, 0)), &(KEY(current, mid + 1)),
               sizeof(Key) * (degree - mid - 1));
        memcpy(&(CHILD(other_half, 0)), &(CHILD(current, mid + 1)),
               sizeof(Pointer) * (degree - mid));
        // Se hace split y el centro sube, pero no se queda en el nodo hijo
      } while (true);
    }
  }

  void insert_t(std::vector<Key> const &keys, Pointer &current) {
    if (current != null && free_workers > 1 && keys.size() > free_workers &&
        IS_NOT_LEAF(current)) {
      std::vector<std::vector<Key>> subset(SIZE(current) + 1);
      const int reservation = 2 * (keys.size()) / (SIZE(current) + 1);
      for (auto &s : subset)
        s.reserve(reservation);
      for (auto const &key : keys) {
        int i = 0;
        for (; i < SIZE(current); ++i) {
          if (key < KEY(current, i))
            break;
        }
        subset[i].push_back(key);
      }

      std::vector<int> subtrees;
      subtrees.reserve(SIZE(current) + 1);
      for (int i = 0; i <= SIZE(current); ++i)
        if (subset[i].size() > 0)
          subtrees.push_back(i);
      std::sort(subtrees.begin(), subtrees.end(), [&subset](int a, int b) {
        return subset[a].size() < subset[b].size();
      });

      const int ideal_group_sz = keys.size() / free_workers;
      int max_subtrees_group = 0;
      int max_subtrees_group_sz = 0;
      std::vector<int> subtrees_groups;
      subtrees_groups.reserve(SIZE(current) + 1);
      for (int i = 0; i < subtrees.size();) {
        int sz = 0;
        while (sz < ideal_group_sz && i < subtrees.size()) {
          sz += subset[subtrees[i]].size();
          i += 1;
        }
        if (sz > max_subtrees_group_sz) {
          max_subtrees_group = subtrees_groups.size();
          max_subtrees_group_sz = sz;
        }
        subtrees_groups.push_back(i);
      }

      int start = 0;
      std::vector<std::thread> threads;
      free_workers -= subtrees_groups.size();
      if (free_workers == 0 && max_subtrees_group_sz < 2 * ideal_group_sz) {
        for (int i = 0; i < subtrees_groups.size(); ++i) {
          const int end = subtrees_groups[i];
          threads.emplace_back([this, &subset, &subtrees, start, end, current] {
            for (int j = start; j < end; ++j)
              this->insert_r(subset[subtrees[j]], CHILD(current, subtrees[j]));
          });
          start = end;
        }
      } else {
        for (int i = 0; i < subtrees_groups.size(); ++i) {
          const int end = subtrees_groups[i];
          if (i != max_subtrees_group) {
            threads.emplace_back(
                [this, &subset, &subtrees, start, end, current] {
                  for (int j = start; j < end; ++j)
                    this->insert_r(subset[subtrees[j]],
                                   CHILD(current, subtrees[j]));
                });
          } else {
            if (start < (end - 1)) {
              threads.emplace_back(
                  [this, &subset, &subtrees, start, end, current] {
                    for (int j = start; j < (end - 1); ++j)
                      this->insert_r(subset[subtrees[j]],
                                     CHILD(current, subtrees[j]));
                  });
            }
            threads.emplace_back([this, &subset, &subtrees, end, current] {
              this->insert_t(subset[subtrees[end - 1]],
                             CHILD(current, subtrees[end - 1]));
            });
          }
          start = end;
        }
      }
      for (auto &t : threads)
        t.join();
    } else {
      insert_r(keys, current);
    }
  }

  void contains_r(std::vector<Key> const &keys,
                  std::vector<KeyExistence> &result, int from,
                  Pointer sub_root) {
    for (int i = 0; i < keys.size(); ++i)
      result[from + i] = {keys[i], false};
    for (int i = 0; i < keys.size(); ++i) {
      Key const &key = keys[i];
      Pointer current = sub_root;
      while (IS_NOT_LEAF(current)) {
        int j = 0;
        for (; j < SIZE(current); ++j) {
          if (key < KEY(current, j))
            break;
        }
        current = CHILD(current, j);

      } // Encontrar nodo hoja
      for (int j = 0; j < SIZE(current); ++j) {
        if (key < KEY(current, j))
          break;
        else if (key == KEY(current, j)) {
          if (NOT_DIRTY(current, j)) {
            result[from + i].exists = true;
          }
          break;
        }
      } // Encontrar key en nodo hoja
    }
  }

  void contains_t(std::vector<Key> const &keys,
                  std::vector<KeyExistence> &result, int from,
                  Pointer current) {
    if (current != null && free_workers > 1 && keys.size() > free_workers &&
        IS_NOT_LEAF(current)) {
      std::vector<std::vector<Key>> subset(SIZE(current) + 1);
      const int reservation = 2 * (keys.size()) / (SIZE(current) + 1);
      for (auto &s : subset)
        s.reserve(reservation);
      for (auto const &key : keys) {
        int i = 0;
        for (; i < SIZE(current); ++i) {
          if (key < KEY(current, i))
            break;
        }
        subset[i].push_back(key);
      }

      std::vector<std::array<int, 2>> subtrees;
      subtrees.reserve(SIZE(current) + 1);
      int sub_from = from;
      for (int i = 0; i <= SIZE(current); ++i) {
        if (subset[i].size() > 0)
          subtrees.push_back({i, sub_from});
        sub_from += subset[i].size();
      }
      std::sort(subtrees.begin(), subtrees.end(),
                [&subset](std::array<int, 2> a, std::array<int, 2> b) {
                  return subset[a[0]].size() < subset[b[0]].size();
                });

      const int ideal_group_sz = keys.size() / free_workers;
      int max_subtrees_group = 0;
      int max_subtrees_group_sz = 0;
      std::vector<int> subtrees_groups;
      subtrees_groups.reserve(SIZE(current) + 1);
      for (int i = 0; i < subtrees.size();) {
        int sz = 0;
        while (sz < ideal_group_sz && i < subtrees.size()) {
          sz += subset[subtrees[i][0]].size();
          i += 1;
        }
        if (sz > max_subtrees_group_sz) {
          max_subtrees_group = subtrees_groups.size();
          max_subtrees_group_sz = sz;
        }
        subtrees_groups.push_back(i);
      }

      int start = 0;
      std::vector<std::thread> threads;
      free_workers -= subtrees_groups.size();
      if (free_workers == 0 && max_subtrees_group_sz < 2 * ideal_group_sz) {
        for (int i = 0; i < subtrees_groups.size(); ++i) {
          const int end = subtrees_groups[i];
          threads.emplace_back([this, &subset, &subtrees, &result, start, end,
                                current] {
            for (int j = start; j < end; ++j)
              this->contains_r(subset[subtrees[j][0]], result, subtrees[j][1],
                               CHILD(current, subtrees[j][0]));
          });
          start = end;
        }
      } else {
        for (int i = 0; i < subtrees_groups.size(); ++i) {
          const int end = subtrees_groups[i];
          if (i != max_subtrees_group) {
            threads.emplace_back([this, &subset, &subtrees, &result, start, end,
                                  current] {
              for (int j = start; j < end; ++j)
                this->contains_r(subset[subtrees[j][0]], result, subtrees[j][1],
                                 CHILD(current, subtrees[j][0]));
            });
          } else {
            if (start < (end - 1)) {
              threads.emplace_back(
                  [this, &subset, &subtrees, &result, start, end, current] {
                    for (int j = start; j < (end - 1); ++j)
                      this->contains_r(subset[subtrees[j][0]], result,
                                       subtrees[j][1],
                                       CHILD(current, subtrees[j][0]));
                  });
            }
            threads.emplace_back(
                [this, &subset, &subtrees, &result, end, current] {
                  this->contains_t(subset[subtrees[end - 1][0]], result,
                                   subtrees[end - 1][1],
                                   CHILD(current, subtrees[end - 1][0]));
                });
          }
          start = end;
        }
      }
      for (auto &t : threads)
        t.join();
    } else {
      contains_r(keys, result, from, current);
    }
  }

  void erase_r(std::vector<Key> const &keys, Pointer &sub_root) {
    for (auto const &key : keys) {
      if (sub_root == null) {
        sub_root = new Node;
        SIZE(sub_root) = 1;
        KEY(sub_root, 0) = key;
        continue;
      } // Si no hay root

      int level = 0;           // Head del stack
      Pointer parent[max_lvl]; // Stack de parents
      Pointer current = sub_root;
      while (IS_NOT_LEAF(current)) {
        parent[level++] = current;
        int i = 0;
        for (; i < SIZE(current); ++i) {
          if (key < KEY(current, i))
            break;
        }
        current = CHILD(current, i);
      } // Encontrar nodo hoja y el camino de parents

      bool wasnt_clean = true;
      for (int i = 0; i < SIZE(current); ++i) {
        if (key < KEY(current, i))
          break;
        else if (key == KEY(current, i)) {
          if (NOT_DIRTY(current, i)) {
            SET_DIRTY(current, i);
            wasnt_clean = false;
          }
          break;
        }
      }
      if (wasnt_clean)
        erased.fetch_sub(1);
    }
  }

  void erase_t(std::vector<Key> const &keys, Pointer &current) {
    if (current != null && free_workers > 1 && keys.size() > free_workers &&
        IS_NOT_LEAF(current)) {
      std::vector<std::vector<Key>> subset(SIZE(current) + 1);
      const int reservation = 2 * (keys.size()) / (SIZE(current) + 1);
      for (auto &s : subset)
        s.reserve(reservation);
      for (auto const &key : keys) {
        int i = 0;
        for (; i < SIZE(current); ++i) {
          if (key < KEY(current, i))
            break;
        }
        subset[i].push_back(key);
      }

      std::vector<int> subtrees;
      subtrees.reserve(SIZE(current) + 1);
      for (int i = 0; i <= SIZE(current); ++i)
        if (subset[i].size() > 0)
          subtrees.push_back(i);
      std::sort(subtrees.begin(), subtrees.end(), [&subset](int a, int b) {
        return subset[a].size() < subset[b].size();
      });

      const int ideal_group_sz = keys.size() / free_workers;
      int max_subtrees_group = 0;
      int max_subtrees_group_sz = 0;
      std::vector<int> subtrees_groups;
      subtrees_groups.reserve(SIZE(current) + 1);
      for (int i = 0; i < subtrees.size();) {
        int sz = 0;
        while (sz < ideal_group_sz && i < subtrees.size()) {
          sz += subset[subtrees[i]].size();
          i += 1;
        }
        if (sz > max_subtrees_group_sz) {
          max_subtrees_group = subtrees_groups.size();
          max_subtrees_group_sz = sz;
        }
        subtrees_groups.push_back(i);
      }

      int start = 0;
      std::vector<std::thread> threads;
      free_workers -= subtrees_groups.size();
      if (free_workers == 0 && max_subtrees_group_sz < 2 * ideal_group_sz) {
        for (int i = 0; i < subtrees_groups.size(); ++i) {
          const int end = subtrees_groups[i];
          threads.emplace_back([this, &subset, &subtrees, start, end, current] {
            for (int j = start; j < end; ++j)
              this->erase_r(subset[subtrees[j]], CHILD(current, subtrees[j]));
          });
          start = end;
        }
      } else {
        for (int i = 0; i < subtrees_groups.size(); ++i) {
          const int end = subtrees_groups[i];
          if (i != max_subtrees_group) {
            threads.emplace_back([this, &subset, &subtrees, start, end,
                                  current] {
              for (int j = start; j < end; ++j)
                this->erase_r(subset[subtrees[j]], CHILD(current, subtrees[j]));
            });
          } else {
            if (start < (end - 1)) {
              threads.emplace_back(
                  [this, &subset, &subtrees, start, end, current] {
                    for (int j = start; j < (end - 1); ++j)
                      this->erase_r(subset[subtrees[j]],
                                    CHILD(current, subtrees[j]));
                  });
            }
            threads.emplace_back([this, &subset, &subtrees, end, current] {
              this->erase_t(subset[subtrees[end - 1]],
                            CHILD(current, subtrees[end - 1]));
            });
          }
          start = end;
        }
      }
      for (auto &t : threads)
        t.join();
    } else {
      erase_r(keys, current);
    }
  }

  void remake() {
    Pointer current = root;
    while (IS_NOT_LEAF(current))
      current = CHILD(current, 0);
    std::vector<Key> keys(size);
    for (int i = 0; i < size; current = NEXT(current)) {
      for (int j = 0; j < SIZE(current); ++j) {
        if (NOT_DIRTY(current, j))
          keys[i++] = KEY(current, j);
      }
    }
    deleteAll(root);
    root = null;
    insert_r(keys, root);
    new_subroots.store(0);
    erased.store(0);
  }

public:
  /* CONSTRUCTOR & DESTRUCTOR */
  Set4Batches(std::vector<Key> const &helper_data)
      : size{0}, root{null}, new_subroots{0}, erased{0} {
    if (helper_data.empty())
      return;
    insert_r(helper_data, root);
    Pointer current = root;
    while (IS_NOT_LEAF(current))
      current = CHILD(current, 0);
    for (int i = 0; i < helper_data.size(); current = NEXT(current)) {
      for (int j = 0; j < SIZE(current); ++j) {
        SET_DIRTY(current, j);
        i += 1;
      }
    }
  }
  ~Set4Batches() { deleteAll(root); }

  void insert(std::vector<Key> const &keys) {
    free_workers = workers;
    size += keys.size();
    insert_t(keys, root);
    if (new_subroots.load() > 32)
      remake();
  }

  std::vector<KeyExistence> contains(std::vector<Key> const &keys) {
    std::vector<KeyExistence> result(keys.size());
    free_workers = workers;
    if (root != null)
      contains_t(keys, result, 0, root);
    return result;
  }

  void erase(std::vector<Key> const &keys) {
    if (root == null)
      return;
    free_workers = workers;
    const auto erased_prev = erased.load();
    erased += keys.size();
    erase_t(keys, root);
    size -= (erased.load() - erased_prev);
    if (erased >= size * 10)
      remake();
  }
};

template <typename Key, unsigned workers> class Set4Batches<Key, 0, workers> {};

template <typename Key, unsigned workers> class Set4Batches<Key, 1, workers> {};

template <typename Key, unsigned workers> class Set4Batches<Key, 2, workers> {};
