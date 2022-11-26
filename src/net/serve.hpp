#ifndef __SERVE_HPP__
#define __SERVE_HPP__

class Server {
  public:
    Server(void);
    ~Server(void);
  private:
    void listen_loop(void);
    void process_request(int client_fd, std::string &request);
    
    int listen_fd;
};


#endif