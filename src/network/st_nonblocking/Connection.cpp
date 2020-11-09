#include "Connection.h"
#include <sys/uio.h>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>
namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() { 
    _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    _event.data.fd = _socket;
    _event.data.ptr = this;
}

// See Connection.h
void Connection::OnError() { 
    is_alive = false;

}

// See Connection.h
void Connection::OnClose() { 
    is_alive = false;


}

// See Connection.h
void Connection::DoRead() {
    _logger->debug("Do read on {} socket", _socket);
    try {
        int cur_readed_bytes = -1;
        while ((cur_readed_bytes = read(_socket, read_buffer + readed_bytes, sizeof(read_buffer) - readed_bytes)) > 0) {
            _logger->debug("Got {} bytes from socket", cur_readed_bytes);
            readed_bytes += cur_readed_bytes;

            // Single block of data readed from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            while (readed_bytes > 0) {
                _logger->debug("Process {} bytes", readed_bytes);
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    try{
                        if (parser.Parse(read_buffer, readed_bytes, parsed)) {
                            // There is no command to be launched, continue to parse input stream
                            // Here we are, current chunk finished some command, process it
                            _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                            command_to_execute = parser.Build(arg_remains);
                            if (arg_remains > 0) {
                                arg_remains += 2;
                            }
                        }
                    } catch (std::runtime_error &ex) {
                        answer_buf.push_back("(?^u:ERROR)");
                        _event.events = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLOUT;
                        throw std::runtime_error(ex.what());
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(read_buffer, read_buffer + parsed, readed_bytes - parsed);
                        readed_bytes -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                    argument_for_command.append(read_buffer, to_read);

                    std::memmove(read_buffer, read_buffer + to_read, readed_bytes - to_read);
                    arg_remains -= to_read;
                    readed_bytes -= to_read;
                }

                // Thre is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (argument_for_command.size()) {
                            argument_for_command.resize(argument_for_command.size() - 2);
                        }
                    command_to_execute->Execute(*pStorage, argument_for_command, result);

                    // Send response
                    result += "\r\n";
                    bool add_EPOLLOUT = answer_buf.empty();

                    answer_buf.push_back(result);
                    if (add_EPOLLOUT)
                        _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;

                    // Prepare for the next command
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();
                }
            } // while (readed_bytes)
        }
        if (readed_bytes == 0) {
            _logger->debug("Connection closed");
        } else {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    } catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
    }


}

// See Connection.h
void Connection::DoWrite() {
    _logger->debug("Do write on {} socket", _socket);

    struct iovec iovecs[answer_buf.size()];
    for (int i = 0; i < answer_buf.size(); i++) {
        iovecs[i].iov_len = answer_buf[i].size();
        iovecs[i].iov_base = &(answer_buf[i][0]);
    }
    iovecs[0].iov_base = static_cast<char *>(iovecs[0].iov_base) + cur_position;

    int written;
    if ((written = writev(_socket, iovecs, answer_buf.size())) <= 0) {
        is_alive = false;
        return;
    }

    cur_position += written;

    int i = 0;
    while ((i < answer_buf.size()) && ((cur_position - iovecs[i].iov_len) >= 0)) {
        i++;
        cur_position -= iovecs[i].iov_len;
    }

    answer_buf.erase(answer_buf.begin(), answer_buf.begin() + i);
    if (answer_buf.empty()) {
        _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR; // без записи
    }
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
