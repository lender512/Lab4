#include <vector>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <thread>
#include <functional>
#include <bitset>


#define SIZE(i) (i->size)
#define KEY(i,j) (i->key[j])
#define CHILD(i,j) (i->child[j])
#define PARENT(i) (i->parent)
#define IS_LEAF(i) (i->child[0] == null)
#define IS_NOT_LEAF(i) (i->child[0] != null)
#define NEXT(i) (i->child[degree])
#define CAN_GIVE(i) (SIZE(i) > min)
#define KEY_NOT_DIRTY(i, j) !(i->dirty[j])


template<typename Key, unsigned degree, unsigned workers>
class Set4Batches{
    public:
    struct KeyExistence{
        Key key;
        bool exists;
    };

    private:
    using Size = uint32_t;
    static constexpr Size capacity = degree-1;
    static constexpr Size mid = degree/2;
    static constexpr Size min = (degree/2) + (degree%2) - 1;
    static constexpr Size max_lvl = 22;
    static constexpr Size dirty_bitset_sz = (capacity/32) + 32*(capacity%32 != 0);

    struct Node{
        Size size;
        Key key[capacity+1];
        Node* child[degree+1] = {0};
        std::bitset<dirty_bitset_sz> dirty;
    };
    using Pointer = Node*;
    static constexpr Pointer null = 0;

    struct Job{
        int subset;
        std::function<void(void)> f;
        int from;
    };

    Pointer root;
    unsigned size;
    unsigned new_lvls;
    unsigned free_workers;

    void deleteAll(Pointer current){
        if(current == null) return;
        for(int i = 0; i<SIZE(current); ++i) deleteAll(CHILD(current, i));
        delete current;
    }

    void insert_r(std::vector<Key> const& keys, Pointer& sub_root){
        for(auto const& key : keys){
            if(sub_root == null){ 
                sub_root = new Node;
                SIZE(sub_root) = 1;
                KEY(sub_root, 0) = key;
                continue;
            } //Si no hay root
            
            int level = 0; //Head del stack
            Pointer parent[max_lvl]; //Stack de parents
            Pointer current = sub_root;
            while(IS_NOT_LEAF(current)){
                parent[level++] = current;
                int i = 0;
                for(; i< SIZE(current); ++i){
                    if(key < KEY(current, i)) break;
                }
                current = CHILD(current, i);
            } //Encontrar nodo hoja y el camino de parents
            int i = SIZE(current) - 1;
            SIZE(current) += 1;
            while(i>= 0 && key < KEY(current, i)){
                KEY(current, i + 1) = KEY(current, i);
                i -= 1;
            }
            KEY(current, i + 1) = key; //Insertar el key
            
            
            if(SIZE(current) <= capacity) continue; //Si no hay overflow, termina

            SIZE(current) = mid;
            Pointer other_half;
            other_half = new Node;
            SIZE(other_half) = degree - mid;
            memcpy(&(KEY(other_half,0)), &(KEY(current,mid)), sizeof(Key)*(degree - mid));
            NEXT(other_half) = NEXT(current);
            NEXT(current) = other_half;
                        //Se hace split y el centro se queda en el nodo hoja y tambien sube
                        //Se actualiza la double linked list

            do{
                
                if(current == sub_root){
                    sub_root = new Node;
                    SIZE(sub_root) = 1;
                    KEY(sub_root, 0) = KEY(current, SIZE(current));
                    CHILD(sub_root, 0) = current;
                    CHILD(sub_root, 1) = other_half;
                    break;
                } //Si es el root, se crear otro root y termina
            
                Pointer child = other_half;
                Key k = KEY(current, SIZE(current)); //Se obtiene la key que sube
                current = parent[--level]; //Se atiende al padre
                int i = SIZE(current) - 1;
                SIZE(current) += 1;
                while(i>= 0 && key < KEY(current, i)){
                    KEY(current, i + 1) = KEY(current, i);
                    CHILD(current, i + 2) = CHILD(current, i+1);
                    i -= 1;
                }
                KEY(current, i + 1) = k;
                CHILD(current, i + 2) = other_half; //Se inserta la key y el hijo

                if(SIZE(current) <= capacity) break; //Si no hay overflow, termina

                SIZE(current) = mid;
                other_half = new Node;
                SIZE(other_half) = degree - mid - 1;
                memcpy(&(KEY(other_half,0)), &(KEY(current,mid+1)), sizeof(Key)*(degree - mid - 1));
                memcpy(&(CHILD(other_half,0)), &(CHILD(current,mid+1)), sizeof(Pointer)*(degree - mid));
                    //Se hace split y el centro sube, pero no se queda en el nodo hijo 
            }while(true);
        }
    }


    void insert_t(std::vector<Key> const& keys, Pointer& current){
        if(IS_NOT_LEAF(current) && keys.size() > 1){  
            std::vector<std::vector<Key>> subset(SIZE(current) + 1);
            const int reservation = 2*(keys.size())/(SIZE(current) + 1);
            for(auto& s : subset) s.reserve(reservation);
            for(auto const& key : keys) {
                int i = 0;
                for(; i<SIZE(current); ++i){
                    if(key < KEY(current, i)) break;
                }
                subset[i].push_back(key);
            }
            
            std::vector<Job> jobs; jobs.reserve(SIZE(current) + 1);
            for(int i = 0; i <= SIZE(current); ++i) if(subset[i].size() > 0) {
                jobs.push_back({
                    i, 
                    [this, &subset, current, i]{
                        this->insert_r(subset[i], CHILD(current, i));
                    }
                });
            }
            std::sort(jobs.begin(), jobs.end(), [&subset](Job const& a, Job const& b){return subset[a.subset].size() > subset[b.subset].size();});

            
            std::vector<std::thread> threads;
            if (free_workers < jobs.size()){
                const int equality = keys.size() / free_workers;
                while(free_workers && free_workers <= jobs.size()){
                    int acc = -1;
                    std::vector<std::function<void(void)>> worker_jobs;
                    while(acc < equality && free_workers <= jobs.size()){
                        auto const& job = jobs.back();
                        worker_jobs.push_back(job.f);
                        acc += subset[job.subset].size();
                        jobs.pop_back();
                    }
                    threads.emplace_back([worker_jobs]{for(auto const& worker_job : worker_jobs) worker_job();});
                    --free_workers;
                }
            }
            else if (free_workers > jobs.size()){
                while(jobs.size() > 1){
                    const auto worker_job = jobs.back().f;
                    threads.emplace_back([worker_job]{worker_job();});
                    jobs.pop_back();
                    --free_workers;
                }
                const int i = jobs.back().subset;
                threads.emplace_back([this, &subset, current, i]{
                    this->insert_t(subset[i], CHILD(current, i));
                });
            }
            else{
                while(jobs.size()){
                    const auto worker_job = jobs.back().f;
                    threads.emplace_back([worker_job]{worker_job();});
                    jobs.pop_back();
                    --free_workers;
                }
            }
            for(auto& t : threads) t.join();
        }
        else{
            insert_r(keys, current);
        }
    }

    void contains_r(std::vector<Key> const& keys, std::vector<KeyExistence>& result, int from, Pointer sub_root){
        for(int i = 0; i<keys.size(); ++i) result[from + i] = {keys[i], false};
        for(int i = 0; i<keys.size(); ++i){
            Key const& key = keys[i];
            Pointer current = sub_root;
            while(IS_NOT_LEAF(current)){
                int j = 0;
                for(; j< SIZE(current); ++j){
                    if(key < KEY(current, j)) break;
                }
                current = CHILD(current, j);
                
            } //Encontrar nodo hoja

            for(int j = 0; j< SIZE(current); ++j){
                if(key == KEY(current, j) && KEY_NOT_DIRTY(current, j)){
                    result[from + i].exists = true;
                    break;
                }
            } //Encontrar key en nodo hoja

        }
    }

    void contains_t(std::vector<Key> const& keys, std::vector<KeyExistence>& result, int from, Pointer current){
        if(IS_NOT_LEAF(current) && keys.size() > 1){
            std::vector<std::vector<Key>> subset(SIZE(current) + 1);
            const int reservation = 2*(keys.size())/(SIZE(current) + 1);
            for(auto& s : subset) s.reserve(reservation);
            for(auto const& key : keys) {
                int i = 0;
                for(; i<SIZE(current); ++i){
                    if(key < KEY(current, i)) break;
                }
                subset[i].push_back(key);
            }

            int sub_from = from;
            std::vector<Job> jobs; jobs.reserve(SIZE(current) + 1);
            for(int i = 0; i <= SIZE(current); ++i) if(subset[i].size() > 0) {
                jobs.push_back({
                    i, 
                    [this, &subset, &result, sub_from, current, i]{
                        this->contains_r(subset[i], result, sub_from, CHILD(current, i));
                    }, 
                    sub_from
                });
                sub_from += subset[i].size();
            }
            std::sort(jobs.begin(), jobs.end(), [&subset](Job const& a, Job const& b){return subset[a.subset].size() > subset[b.subset].size();});
            
            std::vector<std::thread> threads;
            if (free_workers < jobs.size()){
                const int equality = keys.size() / free_workers;
                while(free_workers && free_workers <= jobs.size()){
                    int acc = -1;
                    std::vector<std::function<void(void)>> worker_jobs;
                    while(acc < equality && free_workers <= jobs.size()){
                        auto const& job = jobs.back();
                        worker_jobs.push_back(job.f);
                        acc += subset[job.subset].size();
                        jobs.pop_back();
                    }
                    threads.emplace_back([worker_jobs]{for(auto const& worker_job : worker_jobs) worker_job();});
                    --free_workers;
                }
            }else if (free_workers > jobs.size()){
                while(jobs.size() > 1){
                    const auto worker_job = jobs.back().f;
                    threads.emplace_back([worker_job]{worker_job();});
                    jobs.pop_back();
                    --free_workers;
                }
                const int i = jobs.back().subset;
                sub_from = jobs.back().from;
                threads.emplace_back([this, &subset, &result, sub_from, current, i]{
                    this->contains_t(subset[i], result, sub_from, CHILD(current, i));
                });
            }else{
                while(jobs.size()){
                    const auto worker_job = jobs.back().f;
                    threads.emplace_back([worker_job]{worker_job();});
                    jobs.pop_back();
                    --free_workers;
                }
            }
            for(auto& t : threads) t.join();
        }else{
            contains_r(keys, result, from, current);
        }
    }

    public:
    /* CONSTRUCTOR & DESTRUCTOR */
    Set4Batches(): size{0}, new_lvls{0}, root{null}{}
    ~Set4Batches(){deleteAll(root);}


    void insert(std::vector<Key> const& keys){
        if(workers <= 1 || root == null) {
            insert_r(keys, root);
        }
        else {
            free_workers = workers;
            insert_t(keys, root);
        }
    }

    std::vector<KeyExistence> contains(std::vector<Key> const& keys){
        std::vector<KeyExistence> result(keys.size());
        if(root != null)
        if(workers <= 1) {
            contains_r(keys, result, 0, root);
        }
        else {
            free_workers = workers;
            contains_t(keys, result, 0, root);
        }
        return result;
    }

};


template<typename Key, unsigned workers>
class Set4Batches<Key, 0, workers>{};

template<typename Key, unsigned workers>
class Set4Batches<Key, 1, workers>{};

template<typename Key, unsigned workers>
class Set4Batches<Key, 2, workers>{};