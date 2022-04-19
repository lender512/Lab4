//Por José Ignacio Huby Ochoa

#include <vector>
#include <cstdint>
#include <algorithm>
#include <queue>
#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>

//Macros para facilitar el uso de un vector como gestor de memoria
#define SIZE(i) (i->size)
#define KEY(i,j) (i->key[j])
#define CHILD(i,j) (i->child[j])
#define PARENT(i) (i->parent)
#define IS_LEAF(i) (i->child[0] == null)
#define IS_NOT_LEAF(i) (i->child[0] != null)
#define NEXT(i) (i->child[degree])
#define CAN_GIVE(i) (SIZE(i) > min)

template<uint32_t degree>
class BPTree{
    using Key = uint32_t;
    using Value = uint32_t; // No se usa, pero en la practica deberia haber un value
    using Size = uint32_t;
    static constexpr Size capacity = degree-1;
    static constexpr Size mid = degree/2;
    static constexpr Size min = (degree/2) + (degree%2) - 1;
    static constexpr Size max_lvl = 22;
    static constexpr Size max_unbalance = 4;

    struct Node{
        Size size;
        Key key[capacity+1];
        Node* child[degree+1] = {0};
        uint32_t dummy[1]; //Mejora la performance con estos extras bytes
    };
    using Pointer = Node*;
    static constexpr Pointer null = 0;
    

    
    Pointer root;
    unsigned unbalance;

    const unsigned workers;
    unsigned free_workers;

    void eraseInternal(Pointer internal, int i){
        Pointer current = CHILD(internal, i + 1);
        while(IS_NOT_LEAF(current)){
            current = CHILD(current, 0);
        }
        KEY(internal, i) = KEY(current, 0);
    }

    void deleteAll(Pointer current){
        if(current == null) return;
        for(int i = 0; i<SIZE(current); ++i) deleteAll(CHILD(current, i));
        delete current;
    }

    public:
    BPTree(unsigned workers = 0): workers{workers}, unbalance{0}, root{null}{}

    ~BPTree(){
        deleteAll(root);
    }

    void insert(Key const& key){
        if(root == null){ 
            root = new Node;
            SIZE(root) = 1;
            KEY(root, 0) = key;
            return;
        } //Si no hay root
        
        int level = 0; //Head del stack
        Pointer parent[max_lvl]; //Stack de parents
        Pointer current = root;
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
        
        
        if(SIZE(current) <= capacity) return; //Si no hay overflow, termina

        SIZE(current) = mid;
        Pointer other_half = new Node;
        SIZE(other_half) = degree - mid;
        memcpy(&(KEY(other_half,0)), &(KEY(current,mid)), sizeof(Key)*(degree - mid));
        NEXT(other_half) = NEXT(current);
        NEXT(current) = other_half;
                    //Se hace split y el centro se queda en el nodo hoja y tambien sube
                    //Se actualiza la double linked list

        do{
            if(current == root){
                root = new Node;
                
                SIZE(root) = 1;
                KEY(root, 0) = KEY(current, SIZE(current));
                CHILD(root, 0) = current;
                CHILD(root, 1) = other_half;
                
                return;
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

            if(SIZE(current) <= capacity) return; //Si no hay overflow, termina

            SIZE(current) = mid;
            other_half = new Node;
            SIZE(other_half) = degree - mid - 1;
            memcpy(&(KEY(other_half,0)), &(KEY(current,mid+1)), sizeof(Key)*(degree - mid - 1));
            memcpy(&(CHILD(other_half,0)), &(CHILD(current,mid+1)), sizeof(Pointer)*(degree - mid));
                //Se hace split y el centro sube, pero no se queda en el nodo hijo 
        }while(true);
    }

    std::vector<int> bfs(){
        std::vector<int> result;
        if(root == null) return result;
        std::queue<Pointer> q;
        q.push(root);
        while(!q.empty()){
            const Pointer current = q.front();
            if(CHILD(current, 0) != null)
            for(int i = 0; i<(SIZE(current)+1); ++i){
                if(CHILD(current, i) != null) q.push(CHILD(current, i));
            }
            for(int i = 0; i<SIZE(current); ++i){
                result.push_back( KEY(current, i) );
            }
            q.pop();
        }
        return result;
    }

    void print(){
        if(root == null) return;
        std::queue<Pointer> q;
        q.push(root);
        Key prev = UINT32_MAX;
        while(!q.empty()){
            const Pointer current = q.front();
            if(CHILD(current, 0) != null)
            for(int i = 0; i<(SIZE(current)+1); ++i){
                if(CHILD(current, i) != null) q.push(CHILD(current, i));
            }
            if(prev > KEY(current, 0)) std::cout << std::endl; else std::cout << "  ";
            std::cout << "[ ";
            for(int i = 0; i<SIZE(current); ++i){
                std::cout << KEY(current, i) << ' ';
            }
            std::cout << ']';
            //std::cout << std::endl;
            prev = KEY(current, SIZE(current) - 1);
            q.pop();
        }
        std::cout << std::endl;
    }


    void erase(Key const& key){
        if(root == null) return; //Si no hay root, termina

        
        int i;
        int level = 0; //Head del stack
        int child[max_lvl]; //Stack de childs
        Pointer parent[max_lvl]; //Stack de parents
        Pointer current = root;
        Pointer internal = null;
        int iinternal;
        while(IS_NOT_LEAF(current)){
            for(i = 0; i< SIZE(current); ++i){
                if(key < KEY(current, i)) break;
                else if(key == KEY(current, i)) {
                    internal = current;
                    iinternal = i;
                }
            }
            child[level] = i;
            parent[level++] = current;
            current = CHILD(current, i);
        } //Encontrar nodo hoja y el camino de parents

        level -= 1;
        int ileft = i - 1;
        int iright = i + 1;


        for(i = 0; i< SIZE(current); ++i){
            if(key == KEY(current, i)) break;
        } //Encontrar key en nodo hoja
        

        if(i == SIZE(current)) return; //Si no hay key, termina

        SIZE(current) -= 1;
        for(; i< SIZE(current); ++i){
           KEY(current, i) = KEY(current, i+1);
        } //Eliminar el key en nodo hoja
        
        

        if(current == root){
            if(SIZE(root) == 0) {
                delete root;
                root = null;
            } //Si el root está vacio, volver null
            return;
        } //Si solo esta el root, termina


        if(SIZE(current) >= min) {
            if(internal != null){
                eraseInternal(internal, iinternal);
            } // Si existe una copia interna
            return;
        } // Si no hay underflow, termina 


        Pointer sibling;
        if(ileft >= 0){
            sibling = CHILD(parent[level], ileft);
            if(CAN_GIVE(sibling)){
                for(int j = SIZE(current); j > 0; --j){
                    KEY(current, j) = KEY(current, j - 1);
                }
                SIZE(current) += 1;
                SIZE(sibling) -= 1;
                KEY(current, 0) = KEY(sibling, SIZE(sibling));
                KEY(parent[level], ileft) = KEY(current, 0);
                goto check_internal;
            }
        }
        if(iright <= SIZE(parent[level])){
            sibling = CHILD(parent[level], iright);
            if(CAN_GIVE(sibling)){
                KEY(current, SIZE(current)) = KEY(sibling, 0);
                SIZE(current) += 1;
                SIZE(sibling) -= 1;
                for(int j = 0; j < SIZE(sibling); ++j){
                    KEY(sibling, j) = KEY(sibling, j + 1);
                }
                KEY(parent[level], iright - 1) = KEY(sibling, 0);
                goto check_internal;
            }
        } //Si algun hermano puede prestar, presta, arregla al padre y termina

        Pointer deleted;
        if(ileft >= 0){
            sibling = CHILD(parent[level], ileft);
            for(int j = 0; j < SIZE(current); ++j){
                KEY(sibling, SIZE(sibling) + j) = KEY(current, j);
            }
            SIZE(sibling) += SIZE(current);
            NEXT(sibling) = NEXT(current);
            deleted = current;
            i = ileft;
            delete current;
        }
        else if(iright <= SIZE(parent[level])){
            sibling = CHILD(parent[level], iright);
            for(int j = 0; j < SIZE(sibling); ++j){
                KEY(current, SIZE(current) + j) = KEY(sibling, j);
            }
            SIZE(current) += SIZE(sibling);
            NEXT(current) = NEXT(sibling);
            deleted = sibling;
            i = iright - 1;
            delete sibling;
        } //Si existe algun hermano y ninguno puede prestar, se juntan

        do{
            current = parent[level];

            if(current == root && SIZE(current) == 1){
                root = (deleted == CHILD(current, 0)) ? CHILD(current, 1) : CHILD(current, 0);
                break;
            } // Si es root y tiene size 1, se cambia de root al hijo no eliminado y termina
            
            for(int j = i; j< SIZE(current); ++j){
                KEY(current, j) = KEY(current, j+1);
            } // Se elimina la llave
            
            int remove_child = 0;
            for(; remove_child <= SIZE(current); ++remove_child){
                if(CHILD(current, remove_child) == deleted) break;
            }
            for(; remove_child < SIZE(current); ++remove_child){
                CHILD(current, remove_child) =  CHILD(current, remove_child + 1);
            }
            CHILD(current, SIZE(current)) = null;
            SIZE(current) -= 1; //Se remueve el hijo eliminado

            if(current == root) break; //Si es root, termina

            if(SIZE(current) >= min) break; //Si no hace underflow, termina

            
            level -= 1;
            ileft = child[level] - 1;
            iright = child[level] + 1;
            
            if(ileft >= 0){
                sibling = CHILD(parent[level], ileft);
                if(CAN_GIVE(sibling)){
                    for(int j = SIZE(current); j > 0; --j){
                        KEY(current, j) = KEY(current, j - 1);
                    }
                    for(int j = SIZE(current) + 1; j > 0; --j){
                        CHILD(current, j) = CHILD(current, j - 1);
                    }
                    KEY(current, 0) = KEY(parent[level], ileft);
                    KEY(parent[level], ileft) = KEY(sibling, SIZE(sibling) - 1);
                    CHILD(current, 0) = CHILD(sibling, SIZE(sibling));
                    CHILD(sibling, SIZE(sibling)) = null;
                    SIZE(current) += 1;
                    SIZE(sibling) -= 1;
                    break;
                }
            }
            if(iright <= SIZE(parent[level])){
                sibling = CHILD(parent[level], iright);
                if(CAN_GIVE(sibling)){
                    KEY(current, SIZE(current)) = KEY(parent[level], iright - 1);
                    KEY(parent[level], iright - 1) = KEY(sibling, 0);
                    CHILD(current, SIZE(current) + 1) = CHILD(sibling, 0);
                    CHILD(sibling, SIZE(sibling) + 1) = null;
                    SIZE(current) += 1;
                    SIZE(sibling) -= 1;
                    for(int j = 0; j < SIZE(sibling); ++j){
                        KEY(sibling, j) = KEY(sibling, j + 1);
                    }
                    for(int j = 0; j <= SIZE(sibling); ++j){
                        CHILD(sibling, j) = CHILD(sibling, j + 1);
                    }
                    break;
                }
            } //Si algun hermano puede prestar, el padre presta al actual, aquel al padre, y termina

            if(ileft >= 0){
                sibling = CHILD(parent[level], ileft);
                KEY(sibling, SIZE(sibling)) = KEY(parent[level], ileft);
                for(int j = 0; j < SIZE(current); ++j){
                    KEY(sibling, SIZE(sibling) + j + 1) = KEY(current, j);
                }
                for(int j = 0; j < (SIZE(current) + 1); ++j){
                    CHILD(sibling, SIZE(sibling) + j + 1) = CHILD(current, j);
                }
                SIZE(sibling) += SIZE(current) + 1;
                deleted = current;
                i = ileft;
                delete current;
            }
            else if(iright <= SIZE(parent[level])){
                sibling = CHILD(parent[level], iright);
                KEY(current, SIZE(current)) = KEY(parent[level], iright - 1);
                for(int j = 0; j < SIZE(sibling); ++j){
                    KEY(current, SIZE(current) + j + 1) = KEY(sibling, j);
                }
                for(int j = 0; j < (SIZE(sibling) + 1); ++j){
                    CHILD(current, SIZE(current) + j + 1) = CHILD(sibling, j);
                }
                SIZE(current) += SIZE(sibling) + 1;
                deleted = sibling;
                i = iright - 1;
                delete sibling;
            } //Si existe algun hermano y ninguno puede prestar, se juntan

        }while(true);

        check_internal:
        if(internal != null){
            internal = root;
            bool found = false;
            while(IS_NOT_LEAF(internal)){
                for(i = 0; i< SIZE(internal); ++i){
                    if(key < KEY(internal, i)) break;
                    else if(key == KEY(internal, i)) {
                        found = true;
                        break;
                    }
                }
                if(found) break;
                internal = CHILD(internal, i);
            } //Encontrar nodo interno
            if(found) eraseInternal(internal, i); //Sustituirlo por el siguiente
        }
    }

    /* PARALLEL (FOR MULTIPLE LOOKUPS) */
    void contains_r(std::vector<Key>& keys, std::vector<int>& result, int from, int to, Pointer current){
        if(IS_NOT_LEAF(current)){
            if((to - from) > 1){
                int ndivisions = 0;
                int divisions[degree + 1];
                divisions[0] = from;
                for(int i = from; i<to;){
                    auto& _input_key = keys[i];
                    auto& _node_key = KEY(current, ndivisions);
                    if(keys[i] < KEY(current, ndivisions)) ++i;
                    else {
                        divisions[++ndivisions] = i;
                        if(ndivisions == SIZE(current)) break;
                    }
                }
                divisions[++ndivisions] = to;

                
                for(int i = 0; i<ndivisions; ++i){
                    const int child_from = divisions[i];
                    const int child_to = divisions[i+1];
                    if(child_from < child_to){
                        const Pointer child = CHILD(current, i);
                        contains_r(keys, result, child_from, child_to, child);
                    }
                }
                return;
            }

            do{
                int i = 0;
                for(; i< SIZE(current); ++i){
                    if(keys[to] < KEY(current, i)) break;
                }
                current = CHILD(current, i);
            }while(IS_NOT_LEAF(current));

        }

        int j = from;
        for(int i = 0; i<SIZE(current) && j<to;) {
            if(KEY(current, i) < keys[j]){
                i += 1;
            }
            else if(KEY(current, i) > keys[j]){
                j += 1;
            }
            else {
                result[j] = 1;
                i += 1;
                j += 1;
            }
        }
    }

    void contains_t(std::vector<Key>& keys, std::vector<int>& result, int from, int to, Pointer current){
        if(IS_NOT_LEAF(current)){
            if((to - from) > 1){
                int ndivisions = 0;
                int divisions[degree + 1];
                divisions[0] = from;
                int biggest_division = 0;
                int biggest_division_sz = 0;
                for(int i = from; i<to;){
                    auto& _input_key = keys[i];
                    auto& _node_key = KEY(current, ndivisions);
                    if(keys[i] < KEY(current, ndivisions)) ++i;
                    else {
                        const int division_sz = i - divisions[ndivisions];
                        if(division_sz > biggest_division_sz){
                            biggest_division = ndivisions;
                            biggest_division_sz = division_sz;
                        }
                        divisions[++ndivisions] = i;
                        if(ndivisions == SIZE(current)) break;
                    }
                }
                const int division_sz = to - divisions[ndivisions];
                if(division_sz > biggest_division_sz){
                    biggest_division = ndivisions;
                    biggest_division_sz = division_sz;
                }
                divisions[++ndivisions] = to;


                const unsigned initial_workers = free_workers;
                if(free_workers <= ndivisions){
                    int i = 0;
                    while(i<ndivisions){
                        std::vector<std::thread> threads;
                        while(free_workers && i < ndivisions){
                            const int child_from = divisions[i];
                            const int child_to = divisions[i+1];
                            const Pointer child = CHILD(current, i);
                            if(child_from < child_to){
                                threads.emplace_back([this, &keys, &result, child_from, child_to, child]{
                                    this->contains_r(keys, result, child_from, child_to, child);
                                });
                                --free_workers;
                            }
                            ++i;
                        }
                        for(auto& t : threads) t.join();
                        free_workers = initial_workers;
                    }
                }else{
                    std::vector<std::thread> threads;
                    int i = 0;
                    free_workers -= ndivisions;
                    for(; i<biggest_division; ++i){
                        const int child_from = divisions[i];
                        const int child_to = divisions[i+1];
                        const Pointer child = CHILD(current, i);
                        if(child_from < child_to){
                            threads.emplace_back([this, &keys, &result, child_from, child_to, child]{
                                this->contains_r(keys, result, child_from, child_to, child);
                            });
                        }
                    }
                    for(++i; i<ndivisions; ++i){
                        const int child_from = divisions[i];
                        const int child_to = divisions[i+1];
                        const Pointer child = CHILD(current, i);
                        if(child_from < child_to){
                            threads.emplace_back([this, &keys, &result, child_from, child_to, child]{
                                this->contains_r(keys, result, child_from, child_to, child);
                            });
                        }
                    }
                    const int child_from = divisions[biggest_division];
                    const int child_to = divisions[biggest_division+1];
                    const Pointer child = CHILD(current, biggest_division);
                    threads.emplace_back([this, &keys, &result, child_from, child_to, child]{
                        this->contains_t(keys, result, child_from, child_to, child);
                    });
                    for(auto& t : threads) t.join();
                }
                return;
            }

            do{
                int i = 0;
                for(; i< SIZE(current); ++i){
                    if(keys[to] < KEY(current, i)) break;
                }
                current = CHILD(current, i);
            }while(IS_NOT_LEAF(current));

        }

        int j = from;
        for(int i = 0; i<SIZE(current) && j<to;) {
            if(KEY(current, i) < keys[j]){
                i += 1;
            }
            else if(KEY(current, i) > keys[j]){
                j += 1;
            }
            else {
                result[j] = 1;
                i += 1;
                j += 1;
            }
        }

    }

    /* PARALLEL (FOR MULTIPLE LOOKUPS) */
    std::vector<int> containsMultiple(std::vector<Key>& keys) {
        std::vector<int> result(keys.size(),0);
        if(root == null) return result;
        free_workers = workers;
        if(free_workers > 1) contains_t(keys, result, 0, keys.size(), root);
        else contains_r(keys, result, 0, keys.size(), root);
        return result;
    }

    

    /* NOT PARALLEL (FOR SINGLE LOOKUP) */
    bool contains(Key const& key) {
        if(root == null) return false; //Si no hay root, termina
        
        Pointer current = root;
        while(IS_NOT_LEAF(current)){
            int i = 0;
            for(; i< SIZE(current); ++i){
                if(key < KEY(current, i)) break;
                else if(key == KEY(current, i)) return true;
            }
            current = CHILD(current, i);
            
        } //Encontrar nodo hoja

        for(int i = 0; i< SIZE(current); ++i){
            if(key == KEY(current, i)) return true;
        } //Encontrar key en nodo hoja
        

        return false; //Si no hay key, termina
    }


    void insert_r(std::vector<Key> const& keys, int from, int to, Pointer& sub_root){
        for(int _i = from; _i<to; ++_i){
            int size;
            const Key key = keys[_i];
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


    void insert_t(std::vector<Key> const& keys, int from, int to, Pointer& current){
        if(IS_NOT_LEAF(current) && ((to - from) > 1)){
            int ndivisions = 0;
            int divisions[degree + 1];
            divisions[0] = from;
            int biggest_division = 0;
            int biggest_division_sz = 0;
            for(int i = from; i<to;){
                auto& _input_key = keys[i];
                auto& _node_key = KEY(current, ndivisions);
                if(keys[i] < KEY(current, ndivisions)) ++i;
                else {
                    const int division_sz = i - divisions[ndivisions];
                    if(division_sz > biggest_division_sz){
                        biggest_division = ndivisions;
                        biggest_division_sz = division_sz;
                    }
                    divisions[++ndivisions] = i;
                    if(ndivisions == SIZE(current)) break;
                }
            }
            const int division_sz = to - divisions[ndivisions];
            if(division_sz > biggest_division_sz){
                biggest_division = ndivisions;
                biggest_division_sz = division_sz;
            }
            divisions[++ndivisions] = to;

            const unsigned initial_workers = free_workers;
            if(free_workers <= ndivisions){
                int i = 0;
                while(i<ndivisions){
                    std::vector<std::thread> threads;
                    while(free_workers && i < ndivisions){
                        const int child_from = divisions[i];
                        const int child_to = divisions[i+1];
                        if(child_from < child_to){
                            threads.emplace_back([this, &keys, child_from, child_to, current, i]{
                                this->insert_r(keys, child_from, child_to, CHILD(current, i));
                            });
                            --free_workers;
                        }
                        ++i;
                    }
                    for(auto& t : threads) t.join();
                    free_workers = initial_workers;
                }
            }else{
                std::vector<std::thread> threads;
                int i = 0;
                free_workers -= ndivisions;
                for(; i<biggest_division; ++i){
                    const int child_from = divisions[i];
                    const int child_to = divisions[i+1];
                    if(child_from < child_to){
                        threads.emplace_back([this, &keys, child_from, child_to, current, i]{
                            this->insert_r(keys, child_from, child_to, CHILD(current, i));
                        });
                    }
                }
                for(++i; i<ndivisions; ++i){
                    const int child_from = divisions[i];
                    const int child_to = divisions[i+1];
                    if(child_from < child_to){
                        threads.emplace_back([this, &keys, child_from, child_to, current, i]{
                                this->insert_r(keys, child_from, child_to, CHILD(current, i));
                        });
                    }
                }
                const int child_from = divisions[biggest_division];
                const int child_to = divisions[biggest_division+1];
                threads.emplace_back([this, &keys, child_from, child_to, current, biggest_division]{
                    this->insert_t(keys, child_from, child_to, CHILD(current, biggest_division));
                });
                for(auto& t : threads) t.join();
            }
        }
        else{
            insert_r(keys, from, to, current);
        }
    }


    /* PARALLEL INSERT */
    void insertMultiple(std::vector<Key> const& keys){
        free_workers = workers;
        if(workers <= 1 || root == null) {
            insert_r(keys, 0, keys.size(), root);
        }
        else {
            insert_t(keys, 0, keys.size(), root);
        }
    }


};

/* INVALID DEGREES (EMPTY CLASSES) */
template<>
class BPTree<0>{};
template<>
class BPTree<1>{};
template<>
class BPTree<2>{};