#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>

// By José Ignacio Huby Ochoa


class Scheduler{
        std::function<void(void)> *queue_jobs;
        unsigned queue_front = 0;
        unsigned queue_back = 0;
        unsigned queue_size = 0;
        const unsigned queue_capacity;
        std::mutex queue_lock;
        std::condition_variable queue_not_full;
        std::condition_variable queue_not_empty;

        unsigned nworkers;
        unsigned syncpoint_counter;
        std::mutex syncpoint_lock;
        std::condition_variable syncpoint_cv_1;
        std::condition_variable syncpoint_cv_2;

        public:
        const std::function<void(void)> SyncPointWait = [this]{
                std::unique_lock<std::mutex> ul(syncpoint_lock);
                syncpoint_cv_1.wait(ul, [this]{return syncpoint_counter < nworkers;});
                syncpoint_counter -= 1;
                ul.unlock();
                syncpoint_cv_2.notify_one();
        };
        const std::function<void(void)> SyncPointWake = [this]{
                {
                        std::lock_guard<std::mutex> lg(syncpoint_lock);
                        syncpoint_counter -= 1;
                }
                syncpoint_cv_1.notify_all();

                std::unique_lock<std::mutex> ul(syncpoint_lock);
                syncpoint_cv_2.wait(ul, [this]{return syncpoint_counter == 1;});
                syncpoint_counter = nworkers;
        };
        inline void schedule(const std::function<void(void)>& job){
                std::unique_lock<std::mutex> ul(queue_lock);
                queue_not_full.wait(ul, [this](){return queue_size != queue_capacity;});
                queue_jobs[queue_back] = job;
                queue_back = (queue_back + 1) % queue_capacity;
                ++queue_size;
                ul.unlock();
                queue_not_empty.notify_one();
        }
        inline void scheduleSyncPoint(){
                for(unsigned i = 1; i<nworkers; ++i)
                        schedule(SyncPointWait);
                schedule(SyncPointWake);
        }
        inline void scheduleNotConcurrent(const std::function<void(void)>& job){
                for(unsigned i = 1; i<nworkers; ++i)
                        schedule(SyncPointWait);
                schedule(job);
                schedule(SyncPointWake);
        }
        inline void work(){
                std::unique_lock<std::mutex> ul(queue_lock);
                queue_not_empty.wait(ul, [this](){return queue_size != 0;});
                auto job = queue_jobs[queue_front];
                queue_front = (queue_front + 1) % queue_capacity;
                --queue_size;
                ul.unlock();
                queue_not_full.notify_one();
                job();
        }
        Scheduler(unsigned queue_capacity, unsigned threads, bool mainThreadWillWork): queue_capacity{queue_capacity}{
                queue_jobs = new std::function<void(void)>[queue_capacity];
                nworkers = threads;
                for (unsigned i = 0; i < nworkers; ++i){
                        std::thread worker([this] () {
                                while (true) this->work();
                        });
                        worker.detach();
                }
                if(mainThreadWillWork) nworkers += 1;
                syncpoint_counter = nworkers;
        }
        ~Scheduler(){
            delete[] queue_jobs;
        }
        unsigned amountOfWorkers() const {return nworkers;}
        
};