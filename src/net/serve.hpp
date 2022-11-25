#ifndef __SERVE_HPP__
#define __SERVE_HPP__

class Server {
  public:
    Server(void);
    ~Server(void);
  private:
    void listen_loop(void);

    int listen_fd;
};


#endif