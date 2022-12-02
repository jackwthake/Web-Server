#ifndef __POOL_HPP__
#define __POOL_HPP__

#include <functional>
#include <mutex>
#include <vector>
#include <queue>
#include <utility>

#include <openssl/ssl.h> // SSL structure
#include <netinet/in.h> // struct sockaddr_in

// holds info for one job
struct job_t {
  struct info_t {
    const class https_server *server;
    struct sockaddr_in client_addr;

    SSL *ssl = NULL;
    int client_fd = 0;
  };

  job_t::info_t info;
  std::function<void(job_t::info_t)> func;
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