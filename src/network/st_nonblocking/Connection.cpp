#include "Connection.h"

#include <iostream>

namespace Afina {
namespace Network {
    namespace STnonblock {

        // See Connection.h
        void Connection::Start(std::shared_ptr<spdlog::logger> logger)
        {
            _event.events = mask_r;
            _event.data.fd = _socket;
            _event.data.ptr = this;
            _logger = logger;
        }

        // See Connection.h
        void Connection::OnError()
        {
            _alive = false;
        }

        // See Connection.h
        void Connection::OnClose()
        {
            _alive = false;
        }

        // See Connection.h
        void Connection::DoRead()
        {
            try {
                int read_bytes_new = -1;
                while ((read_bytes_new = read(_socket, client_buffer + read_bytes, sizeof(client_buffer) - read_bytes)) > 0) {
                    read_bytes += read_bytes_new;
                    while (read_bytes > 0) {
                        // There is no command yet
                        if (!command_to_execute) {
                            std::size_t parsed = 0;
                            if (parser.Parse(client_buffer, read_bytes, parsed)) {
                                // No command to be launched, continue to parse input stream
                                command_to_execute = parser.Build(arg_remains);
                                if (arg_remains > 0) {
                                    arg_remains += 2;
                                }
                            }

                            // Parsed might fails to consume any bytes from input stream
                            if (parsed == 0) {
                                break;
                            } else {
                                std::memmove(client_buffer, client_buffer + parsed, read_bytes - parsed);
                                read_bytes -= parsed;
                            }
                        }

                        // There is command, but we still wait for argument to come
                        if (command_to_execute && arg_remains > 0) {
                            // There is some parsed command, and now we are reading argument
                            std::size_t to_read = std::min(arg_remains, std::size_t(read_bytes));
                            argument_for_command.append(client_buffer, to_read);

                            std::memmove(client_buffer, client_buffer + to_read, read_bytes - to_read);
                            arg_remains -= to_read;
                            read_bytes -= to_read;
                        }

                        // Thre is command & argument - RUN!
                        if (command_to_execute && arg_remains == 0) {
                            std::string result;
                            command_to_execute->Execute(*pStorage, argument_for_command, result);
                            result += "\n";

                            // Save response
                            _answers.push_back(result);

                            _event.events = mask_rw;

                            // Prepare for the next command
                            command_to_execute.reset();
                            argument_for_command.resize(0);
                            parser.Reset();
                        }
                    } // while (read_bytes)
                }
            } catch (std::runtime_error& ex) {
                _logger->error("Failed to process connection on descriptor {} : {}", _socket, ex.what());
            }
        }

        // See Connection.h
        void Connection::DoWrite()
        {
            struct iovec iovecs[_answers.size()];
            for (int i = 0; i < _answers.size(); i++) {
                iovecs[i].iov_len = _answers[i].size();
                iovecs[i].iov_base = &(_answers[i][0]);
            }
            iovecs[0].iov_base = (char*)(iovecs[0].iov_base) + _position;
            // iovecs[0].iov_base = static_cast<char*>(iovecs[0].iov_base) + _position;

            assert(iovecs[0].iov_len > _position);
            iovecs[0].iov_len -= _position;

            int written;
            if ((written = writev(_socket, iovecs, _answers.size())) <= 0) {
                _logger->error("Failed to send response");
            }
            _position += written;

            int i;
            for (i = 0; i < _answers.size() && (_position - iovecs[i].iov_len) >= 0; i++) {
                _position -= iovecs[i].iov_len;
            }

            _answers.erase(_answers.begin(), _answers.begin() + i);
            if (_answers.empty()) {
                _event.events = mask_r;
            }
        }
        // поставить интерес на запись, когда очередь стала не пустая. когда пустая - снять его
    } // namespace STnonblock
} // namespace Network
} // namespace Afina
