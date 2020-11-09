#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

#include <afina/execute/Command.h>
#include <protocol/Parser.h>
#include <spdlog/logger.h>
#include <vector>
#include <atomic>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps, std::shared_ptr<spdlog::logger> pl) :
            _socket(s),
            pStorage(std::move(ps)),
            _logger(std::move(pl)) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        is_alive.store(true);
    }

    inline bool isAlive() const { return is_alive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class Worker;
    friend class ServerImpl;
    std::mutex _mutex;
    int _socket;
    struct epoll_event _event;


    std::size_t arg_remains;
    std::atomic<bool>is_alive;
    std::shared_ptr<spdlog::logger> _logger;


    // int buffer_size = 4096;
    char read_buffer[4096];
    int readed_bytes;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::shared_ptr<Afina::Storage> pStorage;//

    std::vector<std::string> answer_buf;

    int cur_position = 0;//
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
