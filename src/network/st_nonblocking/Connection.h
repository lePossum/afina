#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <iostream>
#include <string>
#include <sys/uio.h>
#include <vector>

#include "protocol/Parser.h"
#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <spdlog/logger.h>
#include <sys/epoll.h>

namespace Afina {
namespace Network {
    namespace STnonblock {

        class Connection {
        public:
            Connection(int s)
                : _socket(s)
            {
                std::memset(&_event, 0, sizeof(struct epoll_event));
                _event.data.ptr = this;
            }

            inline bool isAlive() const { return _alive; }

            void Start(std::shared_ptr<spdlog::logger> logger);

        protected:
            void OnError();
            void OnClose();
            void DoRead();
            void DoWrite();

        private:
            friend class ServerImpl;

            static const int mask_r = EPOLLIN | EPOLLRDHUP | EPOLLERR;
            static const int mask_rw = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;

            std::shared_ptr<spdlog::logger> _logger;
            std::shared_ptr<Afina::Storage> pStorage;

            int _socket;
            bool _alive; //
            struct epoll_event _event;

            std::size_t arg_remains;
            Protocol::Parser parser;
            std::string argument_for_command;
            std::unique_ptr<Execute::Command> command_to_execute;

            int read_bytes = 0;
            char client_buffer[4096];

            std::vector<std::string> _answers;
            int _position = 0;
        };

    } // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
