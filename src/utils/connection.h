#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <boost/asio.hpp>
#include <vector>
#include <memory>

#include "tsqueue.h"
#include "binary_serial.h"

namespace net {

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
            pointer ret(new connection(std::forward<Ts>(args) ... ));
            ret->start_reading();
            return ret;
        }

    private:
        connection(boost::asio::ip::tcp::socket &&socket)
            : m_socket(std::move(socket)) {}

        connection(boost::asio::io_context &ctx, const std::string &host, uint16_t port)
            : m_socket(ctx)
        {
            boost::asio::ip::tcp::resolver resolver(ctx);
            auto result = resolver.resolve(boost::asio::ip::tcp::v4(), host, std::to_string(port));
            
            m_socket.connect(*result);
        }

    public:
        ~connection() {
            disconnect();
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
            boost::asio::post(m_socket.get_executor(), [this, self = this->shared_from_this(), ... args = std::forward<Ts>(args)] () mutable {
                bool empty = m_out_queue.empty();
                m_out_queue.emplace_back(std::forward<Ts>(args) ... );
                if (empty) {
                    write_next_message();
                }
            });
        }

    private:
        void start_reading() {
            m_buffer.resize(sizeof(HeaderType));
            boost::asio::async_read(m_socket, boost::asio::buffer(m_buffer),
                [this, self = this->shared_from_this()](const boost::system::error_code &ec, size_t nbytes) {
                    if (!ec) {
                        HeaderType h = binary::deserialize<HeaderType>(m_buffer);
                        if (h.validate()) {
                            m_buffer.resize(h.length);
                            boost::asio::async_read(m_socket, boost::asio::buffer(m_buffer),
                                [this, self = std::move(self)](const boost::system::error_code &ec, size_t nbytes) {
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

        void write_next_message() {
            const auto &msg = m_out_queue.front();
            
            HeaderType h;
            h.length = binary::get_size(msg);

            std::vector<std::byte> data;
            data.reserve(sizeof(h) + h.length);

            binary::serializer<HeaderType>{}(h, data);
            binary::serializer<std::remove_cvref_t<decltype(msg)>>{}(msg, data);

            boost::asio::async_write(m_socket, boost::asio::buffer(data),
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

        util::tsqueue<InputMessage> m_in_queue;
        util::tsqueue<OutputMessage> m_out_queue;

        std::vector<std::byte> m_buffer;
    };
}

#endif