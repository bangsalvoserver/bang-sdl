#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include <fmt/core.h>

#include <vector>
#include <memory>
#include <chrono>

#include "tsqueue.h"
#include "binary_serial.h"
#include "enum_error_code.h"

namespace net {

    enum class connection_state : uint8_t {
        disconnected,
        error,
        resolving,
        connecting,
        connected
    };

    DEFINE_ENUM_ERROR_CODE(connection_error,
        (no_error,              "No Error")
        (timeout_expired,       "Timeout Expired")
        (validation_failure,    "Validation Failure")
        (invalid_message,       "Invalid Message")
    )

    constexpr auto timeout = std::chrono::seconds(5);

    template<typename T>
    concept header = requires (const T value) {
        requires binary::serializable<T>;
        requires binary::deserializable<T>;
        requires std::is_trivially_copyable_v<T>;
        { value.validate() } -> std::convertible_to<bool>;
        { value.length } -> std::convertible_to<size_t>;
    };

    template<binary::deserializable InputMessage, binary::serializable OutputMessage, header HeaderType>
    class connection : public std::enable_shared_from_this<connection<InputMessage, OutputMessage, HeaderType>> {
    public:
        using std::enable_shared_from_this<connection<InputMessage, OutputMessage, HeaderType>>::shared_from_this;
        using pointer = std::shared_ptr<connection>;

        template<typename ... Ts>
        static pointer make(Ts && ... args) {
            return pointer(new connection(std::forward<Ts>(args) ... ));
        }

    private:
        connection(boost::asio::io_context &ctx, boost::asio::ip::tcp::socket &&socket)
            : m_socket(std::move(socket))
            , m_strand(ctx)
            , m_timer(ctx)
            , m_state(connection_state::connecting)
        {
            auto endpoint = m_socket.remote_endpoint();
            m_address = fmt::format("{}:{}", endpoint.address().to_string(), endpoint.port());
        }

        connection(boost::asio::io_context &ctx)
            : m_socket(ctx)
            , m_strand(ctx)
            , m_timer(ctx)
            , m_state(connection_state::disconnected) {}

    public:
        void connect(const std::string &host, uint16_t port, auto &&on_complete) {
            auto self(shared_from_this());
            auto resolver = new boost::asio::ip::tcp::resolver(m_socket.get_executor());
            m_state = connection_state::resolving;
            resolver->async_resolve(boost::asio::ip::tcp::v4(), host, std::to_string(port),
                [this, self, host = fmt::format("{}:{}", host, port),
                    resolver = std::unique_ptr<boost::asio::ip::tcp::resolver>(resolver),
                    on_complete = std::move(on_complete)]
                (const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::results_type results) mutable {
                    if (m_state != connection_state::resolving) {
                        on_complete(boost::asio::error::operation_aborted);
                    } else if (ec) {
                        m_ec = ec;
                        m_state = connection_state::error;
                        on_complete(ec);
                    } else {
                        m_state = connection_state::connecting;
                        m_socket.async_connect(*results,
                            [this, self, host = std::move(host), on_complete = std::move(on_complete)]
                            (const boost::system::error_code &ec) {
                                if (!ec) {
                                    m_address = host;
                                } else {
                                    m_ec = ec;
                                    m_state = connection_state::error;
                                    m_socket.close();
                                }
                                on_complete(ec);
                            });
                    }
                });
        }

        connection_state state() const {
            return m_state;
        }

        std::string error_message() const {
            return m_ec.message();
        }

        void disconnect(const std::error_code &ec = {}) {
            switch (state()) {
            case connection_state::connecting:
            case connection_state::connected:
                if (m_socket.is_open()) {
                    boost::asio::post(m_socket.get_executor(),
                        [this, self = shared_from_this()]{
                            boost::system::error_code ec;
                            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                            m_socket.close(ec);
                        });
                }
            [[fallthrough]];
            default:
                m_in_queue.clear();
                m_out_queue.clear();
                if (!ec) {
                    m_state = connection_state::disconnected;
                } else {
                    m_ec = ec;
                    m_state = connection_state::error;
                }
            }
        }

        const std::string &address_string() const {
            return m_address;
        }
        
    public:
        std::optional<InputMessage> pop_message() {
            return m_in_queue.pop_front();
        }

        template<typename ... Ts>
        void push_message(Ts && ... args) {
            auto self(shared_from_this());
            boost::asio::post(m_socket.get_executor(),
                boost::asio::bind_executor(m_strand,
                    [this, self, ... args = std::forward<Ts>(args)] () mutable {
                        bool empty = m_out_queue.empty();
                        m_out_queue.push_back(wrap_message(std::forward<Ts>(args) ... ));
                        if (empty) {
                            write_next_message();
                        }
                    }));
        }

        void start() {
            if (m_state == connection_state::connecting) {
                m_state = connection_state::connected;
                read_next_message();
            }
        }

    private:
        void read_next_message() {
            auto self(shared_from_this());
            m_buffer.resize(sizeof(HeaderType));
            boost::asio::async_read(m_socket, boost::asio::buffer(m_buffer),
                [this, self](const boost::system::error_code &ec, size_t nbytes) {
                    if (!ec) {
                        HeaderType h = binary::deserialize<HeaderType>(m_buffer);
                        if (h.validate()) {
                            m_timer.expires_after(timeout);
                            m_timer.async_wait([this](const boost::system::error_code &ec) {
                                if (!ec) {
                                    m_state = connection_state::error;
                                    m_ec = connection_error::timeout_expired;
                                    m_socket.cancel();
                                }
                            });

                            m_buffer.resize(h.length);
                            boost::asio::async_read(m_socket, boost::asio::buffer(m_buffer),
                                [this, self](const boost::system::error_code &ec, size_t nbytes) {
                                    m_timer.cancel();
                                    if (!ec) {
                                        try {
                                            m_in_queue.push_back(binary::deserialize<InputMessage>(m_buffer));
                                            read_next_message();
                                        } catch (const binary::read_error &error) {
                                            m_ec = error.code();
                                            m_state = connection_state::error;
                                            m_socket.close();
                                        }
                                    } else {
                                        handle_io_error(ec);
                                    }
                                });
                        } else {
                            m_ec = connection_error::validation_failure;
                            m_state = connection_state::error;
                            m_socket.close();
                        }
                    } else {
                        handle_io_error(ec);
                    }
                });
        }

        template<typename ... Ts>
        std::vector<std::byte> wrap_message(Ts && ... args) {
            const OutputMessage msg(std::forward<Ts>(args) ... );
            
            HeaderType h;
            h.length = binary::get_size(msg);

            std::vector<std::byte> data;
            data.reserve(sizeof(h) + h.length);

            binary::serializer<HeaderType>{}(h, data);
            binary::serializer<OutputMessage>{}(msg, data);

            return data;
        }

        void write_next_message() {
            auto self(shared_from_this());
            boost::asio::async_write(m_socket, boost::asio::buffer(m_out_queue.front()),
                [this, self](const boost::system::error_code &ec, size_t nbytes) {
                    if (!ec) {
                        m_out_queue.pop_front();
                        if (!m_out_queue.empty()) {
                            write_next_message();
                        }
                    } else {
                        handle_io_error(ec);
                    }
                });
        }

        void handle_io_error(const boost::system::error_code &ec) {
            switch (state()) {
            case connection_state::disconnected:
                break;
            default:
                m_ec = ec;
                m_state = ec == boost::asio::error::eof ? connection_state::disconnected : connection_state::error;
                [[fallthrough]];
            case connection_state::error:
                m_socket.close();
                break;
            }
        }

    private:
        boost::asio::ip::tcp::socket m_socket;
        boost::asio::io_context::strand m_strand;
        boost::asio::basic_waitable_timer<std::chrono::system_clock> m_timer;
        
        std::atomic<connection_state> m_state;
        std::error_code m_ec;

        util::tsqueue<InputMessage> m_in_queue;
        std::deque<std::vector<std::byte>> m_out_queue;

        std::vector<std::byte> m_buffer;
        std::string m_address;
    };
}

#endif