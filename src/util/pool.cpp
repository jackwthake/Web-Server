#include "pool.hpp"

#include <thread>

#include "log.hpp"


/*
 * Createes the thread pool, populates the pool with as many threads as possible
*/
thread_pool::thread_pool() {
  const uint32_t num_threads = std::thread::hardware_concurrency(); // get max number of threads
  this->threads.resize(num_threads); // resize threads vector

  log_info("THREAD POOl: Creating thread pool of size: %u", num_threads);
  for (auto &thread : this->threads) {
    thread = std::thread(&thread_pool::thread_loop, this); // initialise every thread
  }
}


/*
 * Close all open threads, letting them finish each job first
*/
thread_pool::~thread_pool() {
  { // after the mutex goes out of scope it is released
    std::unique_lock<std::mutex> lock(this->queue_mutex); // prevent data races
    this->should_terminate = true;
  }

  mutex_condition.notify_all();
  for (auto &thread : this->threads) {
    thread.join();
  }
  
  log_info("THREAD POOL: Thread pool cleared.");
  threads.clear();
}


/*
 * Enqueue a job to the thread pool
*/
void thread_pool::queue_job(const job_t &job) {
  { // after the mutex goes out of scope it is released
    std::unique_lock<std::mutex> lock(this->queue_mutex); // prevent data races
    this->jobs.push(job);
  }
  mutex_condition.notify_one();
}


/*
 * Return if the pool is currently completing jobs
*/
bool thread_pool::is_busy(void) {
  bool pool_busy;
  { // after the mutex goes out of scope it is released
    std::unique_lock<std::mutex> lock(this->queue_mutex); // prevent data races
    pool_busy = !jobs.empty();
  }

  return pool_busy;
}


/*
 * Main function for each thread. The thread waits for a job to become available then executes
*/
void thread_pool::thread_loop(void) {
  for (;;) {
    job_t job;

    { // look for next job
      std::unique_lock<std::mutex> lock(this->queue_mutex); // prevent data races
      mutex_condition.wait(lock, [this] {
        return !this->jobs.empty() || this->should_terminate;
      });

      if (should_terminate)
        return;
      
      job = jobs.front(); // get the next job
      jobs.pop(); // dequeue current job
    }

    // run the job
    job.func(job.info);
  }
}