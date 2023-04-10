#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <boost/array.hpp>

#include "common/boost_wrap.h"

#include "cc_demo3/Message.h"
#include "cc_demo3/input/ServerInputMessages.h"
#include "cc_demo3/frame/Frame.h"

namespace cc_demo3
{

namespace server
{

class Session
{
public:
    using Socket = boost::asio::ip::tcp::socket;
    using TermCallback = std::function<void ()>;

    explicit Session(common::boost_wrap::io& io, Socket&& sock) 
      : m_io(io),
        m_socket(std::move(sock)),
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
        cc_demo3::Message<
            comms::option::ReadIterator<const std::uint8_t*>,
            comms::option::Handler<Session>,
            comms::option::NameInterface
        >;

    CC_DEMO3_ALIASES_FOR_SERVER_INPUT_MESSAGES_DEFAULT_OPTIONS(In,,InputMsg);

    void handle(InConnect& msg);
    void handle(InMsg1& msg);
    void handle(InputMsg&);

private:

    using OutputMsg = 
        cc_demo3::Message<
            comms::option::WriteIterator<std::back_insert_iterator<std::vector<std::uint8_t> > >,
            comms::option::LengthInfoInterface,
            comms::option::IdInfoInterface,
            comms::option::NameInterface
        >;

    using AllInputMessages = cc_demo3::input::ServerInputMessages<InputMsg>;

    using Frame = cc_demo3::frame::Frame<InputMsg, AllInputMessages>;

    void terminateSession();
    void processInput();
    void sendMessage(const OutputMsg& msg);

    common::boost_wrap::io& m_io;
    Socket m_socket;
    TermCallback m_termCb;    
    boost::array<std::uint8_t, 1024> m_readBuf;
    std::vector<std::uint8_t> m_inputBuf;
    Frame m_frame;
    Socket::endpoint_type m_remote;
}; 

using SessionPtr = std::unique_ptr<Session>;

} // namespace server

} // namespace cc_demo3
