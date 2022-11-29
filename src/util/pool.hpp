#ifndef __POOL_HPP__
#define __POOL_HPP__

#include <functional>
#include <mutex>
#include <vector>
#include <queue>
#include <utility>

#include <netinet/in.h> // struct sockaddr_in

typedef std::function<void(int, struct sockaddr_in)> job_ptr;

// holds info for one job
struct job_t {
  job_ptr func;
  int fd; 
  struct sockaddr_in client;
};

class thread_pool {
  public:
    thread_pool();
    ~thread_pool();

    void queue_job(const job_t &job);
    bool is_busy(void);
  private:
    void thread_loop(void);

    bool should_terminate = false;
    std::mutex queue_mutex;
    std::condition_variable mutex_condition;
    std::vector<std::thread> threads;
    std::queue<job_t> jobs;
};

#endif