#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "demo3/Message.h"
#include "demo3/ServerInputMessages.h"
#include "demo3/frame/Frame.h"

namespace demo3
{

namespace server
{

class Session
{
public:
    using Socket = boost::asio::ip::tcp::socket;
    using TermCallback = std::function<void ()>;

    explicit Session(Socket&& sock) 
      : m_socket(std::move(sock)),
        m_remote(m_socket.remote_endpoint()) 
    {
    };

    template <typename TFunc>
    void setTerminateCallback(TFunc&& func)
    {
        m_termCb = std::forward<TFunc>(func);
    }

    void start();

    using InputMsg = 
        demo3::Message<
            comms::option::ReadIterator<const std::uint8_t*>,
            comms::option::Handler<Session>,
            comms::option::NameInterface
        >;

    using InConnect = demo3::message::Connect<InputMsg>;
    using InMsg1 = demo3::message::Msg1<InputMsg>;

    void handle(InConnect& msg);
    void handle(InMsg1& msg);
    void handle(InputMsg&);

private:

    using OutputMsg = 
        demo3::Message<
            comms::option::WriteIterator<std::back_insert_iterator<std::vector<std::uint8_t> > >,
            comms::option::LengthInfoInterface,
            comms::option::IdInfoInterface,
            comms::option::NameInterface
        >;

    using AllInputMessages = demo3::ServerInputMessages<InputMsg>;

    using Frame = demo3::frame::Frame<InputMsg, AllInputMessages>;

    void terminateSession();
    void processInput();
    void sendMessage(const OutputMsg& msg);

    Socket m_socket;
    TermCallback m_termCb;    
    boost::array<std::uint8_t, 1024> m_readBuf;
    std::vector<std::uint8_t> m_inputBuf;
    Frame m_frame;
    Socket::endpoint_type m_remote;
}; 

using SessionPtr = std::unique_ptr<Session>;

} // namespace server

} // namespace demo3