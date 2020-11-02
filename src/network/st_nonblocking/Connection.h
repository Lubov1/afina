#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <queue>
#include <utility>
#include <spdlog/logger.h>
#include <afina/execute/Command.h>
#include <protocol/Parser.h>
#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps, std::shared_ptr<spdlog::logger> pl) : _socket(s), pStorage(std::move(ps)), _logger(std::move(pl)) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        is_alive = true;
    }

    inline bool isAlive() const { return is_alive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;
    std::shared_ptr<spdlog::logger> _logger;
    int _socket;
    struct epoll_event _event;
    std::size_t arg_remains;

    bool is_alive ;
    char read_buffer[4096];
    int readed_bytes = 0;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::shared_ptr<Afina::Storage> pStorage;//

    std::vector<std::string> answer_buf;

    int cur_position = 0;//

};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
