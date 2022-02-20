#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <chrono>

#include "tsqueue.h"
#include "binary_serial.h"

namespace net {

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
        using pointer = std::shared_ptr<connection>;

        template<typename ... Ts>
        static pointer make(Ts && ... args) {
            return pointer(new connection(std::forward<Ts>(args) ... ));
        }

    private:
        connection(boost::asio::io_context &ctx, boost::asio::ip::tcp::socket &&socket)
            : m_socket(std::move(socket))
            , m_strand(ctx)
            , m_timer(ctx) {}

        connection(boost::asio::io_context &ctx)
            : m_socket(ctx)
            , m_strand(ctx)
            , m_timer(ctx) {}

    public:
        ~connection() {
            disconnect();
        }

        void connect(const std::string &host, uint16_t port, auto &&on_complete) {
            auto resolver = new boost::asio::ip::tcp::resolver(m_socket.get_executor());
            resolver->async_resolve(boost::asio::ip::tcp::v4(), host, std::to_string(port),
                [this,
                    self = this->shared_from_this(),
                    resolver = std::unique_ptr<boost::asio::ip::tcp::resolver>(resolver),
                    on_complete = std::move(on_complete)]
                (const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::results_type results) {
                    if (!ec) {
                        m_socket.async_connect(*results, std::move(on_complete));
                    } else {
                        on_complete(ec);
                    }
                });
        }
        
        bool connected() const {
            return m_socket.is_open();
        }

        void disconnect() {
            if (connected()) {
                m_socket.close();
            }
        }

        std::string address_string() const {
            auto endpoint = m_socket.remote_endpoint();
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        }
        
    public:
        bool incoming_messages() const {
            return !m_in_queue.empty();
        }

        InputMessage pop_message() {
            return m_in_queue.pop_front();
        }

        template<typename ... Ts>
        void push_message(Ts && ... args) {
            boost::asio::post(m_socket.get_executor(),
                boost::asio::bind_executor(m_strand,
                    [this,
                        self = this->shared_from_this(),
                        ... args = std::forward<Ts>(args)]
                    () mutable {
                        bool empty = m_out_queue.empty();
                        m_out_queue.push_back(wrap_message(std::forward<Ts>(args) ... ));
                        if (empty) {
                            write_next_message();
                        }
                    }));
        }

        void start() {
            start_reading();
        }

    private:
        void start_reading() {
            m_buffer.resize(sizeof(HeaderType));
            boost::asio::async_read(m_socket, boost::asio::buffer(m_buffer),
                [this, self = this->shared_from_this()](const boost::system::error_code &ec, size_t nbytes) {
                    if (!ec) {
                        HeaderType h = binary::deserialize<HeaderType>(m_buffer);
                        if (h.validate()) {
                            m_timer.expires_after(timeout);
                            m_timer.async_wait([this, self](const boost::system::error_code &ec) {
                                if (!ec) {
                                    disconnect();
                                }
                            });

                            m_buffer.resize(h.length);
                            boost::asio::async_read(m_socket, boost::asio::buffer(m_buffer),
                                [this, self = std::move(self)](const boost::system::error_code &ec, size_t nbytes) {
                                    m_timer.cancel();
                                    if (!ec) {
                                        try {
                                            m_in_queue.push_back(binary::deserialize<InputMessage>(m_buffer));
                                            start_reading();
                                        } catch (const binary::read_error &error) {
                                            disconnect();
                                        }
                                    } else {
                                        disconnect();
                                    }
                                });
                        } else {
                            disconnect();
                        }
                    } else {
                        disconnect();
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
            boost::asio::async_write(m_socket, boost::asio::buffer(m_out_queue.front()),
                [this, self = this->shared_from_this()](const boost::system::error_code &ec, size_t nbytes) {
                    if (!ec) {
                        m_out_queue.pop_front();
                        if (!m_out_queue.empty()) {
                            write_next_message();
                        }
                    } else {
                        disconnect();
                    }
                });
        }

    private:
        boost::asio::ip::tcp::socket m_socket;
        boost::asio::io_context::strand m_strand;

        boost::asio::basic_waitable_timer<std::chrono::system_clock> m_timer;

        util::tsqueue<InputMessage> m_in_queue;
        std::deque<std::vector<std::byte>> m_out_queue;

        std::vector<std::byte> m_buffer;
    };
}

#endif