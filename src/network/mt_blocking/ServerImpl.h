#ifndef AFINA_NETWORK_MT_BLOCKING_SERVER_H
#define AFINA_NETWORK_MT_BLOCKING_SERVER_H

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <afina/network/Server.h>

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace MTblocking {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void OnRun();
    void _func(uint32_t number, int client_socket);

private:
    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publish changes cross thread
    // bounds
    std::atomic<bool> running;

    // Server socket to accept connections on
    int _server_socket;

    // Thread to run network on
    std::thread _thread;

    // Max amount of threads
    uint32_t _workers_amount = 1;
    std::mutex _workers_mutex;

    struct Worker {
        std::thread _w_thread;
        bool _is_busy;

        Worker() : _w_thread(), _is_busy(false) {}
    };

    std::vector<Worker> _w_vector;

    int64_t _free_worker();
};

} // namespace MTblocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_BLOCKING_SERVER_H
