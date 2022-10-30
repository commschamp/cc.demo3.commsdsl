#pragma once

#include <cstdint>
#include <string>
#include <iterator>
#include <vector>

#include <boost/array.hpp>

#include "common/boost_wrap.h"

#include "cc_demo3/Message.h"
#include "cc_demo3/input/ClientInputMessages.h"
#include "cc_demo3/frame/Frame.h"

namespace cc_demo3
{

namespace client    
{

class Client
{
public:
    Client(common::boost_wrap::io& io, const std::string& server, std::uint16_t port);

    bool start();

    using InputMsg = 
        cc_demo3::Message<
            comms::option::ReadIterator<const std::uint8_t*>,
            comms::option::Handler<Client>,
            comms::option::NameInterface
        >;

    void handle(InputMsg&);

private:
    using Socket = boost::asio::ip::tcp::socket;

    using OutputMsg = 
        cc_demo3::Message<
            comms::option::WriteIterator<std::back_insert_iterator<std::vector<std::uint8_t> > >,
            comms::option::LengthInfoInterface,
            comms::option::IdInfoInterface,
            comms::option::NameInterface
        >;

    using AllInputMessages = cc_demo3::input::ClientInputMessages<InputMsg>;

    using Frame = cc_demo3::frame::Frame<InputMsg, AllInputMessages>;


    void readDataFromServer();
    void readDataFromStdin();
    void sendConnect();
    void sendMsg1();
    void sendMessage(const OutputMsg& msg);
    void processInput();

    common::boost_wrap::io& m_io;
    Socket m_socket;
    std::string m_server;
    std::uint16_t m_port = 0U;
    Frame m_frame;
    unsigned m_sentVersion = std::numeric_limits<unsigned>::max();
    boost::array<std::uint8_t, 32> m_readBuf;
    std::vector<std::uint8_t> m_inputBuf;
};

} // namespace client

} // namespace cc_demo3
