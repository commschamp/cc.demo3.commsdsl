#include "Client.h"

#include <iostream>
#include <sstream>

#include "demo3/MsgId.h"
#include "comms/units.h"
#include "comms/process.h"

namespace demo3
{

namespace client
{

namespace
{

template <typename TField>
void printVersionDependentField(const TField& f)
{
    std::cout << "\t\t" << f.field().name() << " = ";
    if (f.isMissing())  {
        std::cout << "(missing)";
    }
    else {
        std::cout << (unsigned)f.field().value();
    }
    std::cout << '\n';
}    

} // namespace 

Client::Client(
        common::boost_wrap::io& io, 
        const std::string& server,
        std::uint16_t port)    
  : m_io(io),
    m_socket(io),
    m_server(server),
    m_port(port)
{
    if (m_server.empty()) {
        m_server = "localhost";
    }
}

bool Client::start()
{
    boost::asio::ip::tcp::resolver resolver(m_io);
    auto query = boost::asio::ip::tcp::resolver::query(m_server, std::to_string(m_port));

    boost::system::error_code ec;
    auto iter = resolver.resolve(query, ec);
    if (ec) {
        std::cerr << "ERROR: Failed to resolve \"" << m_server << ':' << m_port << "\" " <<
            "with error: " << ec.message() << std::endl; 
        return false;
    }

    auto endpoint = iter->endpoint();
    m_socket.connect(endpoint, ec);
    if (ec) {
        std::cerr << "ERROR: Failed to connect to \"" << endpoint << "\" " <<
            "with error: " << ec.message() << std::endl; 
        return false;
    }

    std::cout << "INFO: Connected to " << m_socket.remote_endpoint() << std::endl;
    readDataFromServer();
    readDataFromStdin();
    return true;
}

void Client::handle(InputMsg&)
{
    std::cerr << "WARNING: Unexpected message received" << std::endl;
}

void Client::readDataFromServer()
{
    m_socket.async_read_some(
        boost::asio::buffer(m_readBuf),
        [this](const boost::system::error_code& ec, std::size_t bytesCount)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            if (ec) {
                std::cerr << "ERROR: Failed to read with error: " << ec.message() << std::endl;
                m_io.stop();
                return;
            }

            std::cout << "<-- " << std::hex;
            std::copy_n(m_readBuf.begin(), bytesCount, std::ostream_iterator<unsigned>(std::cout, " "));
            std::cout << std::dec << std::endl;

            m_inputBuf.insert(m_inputBuf.end(), m_readBuf.begin(), m_readBuf.begin() + bytesCount);
            processInput();
            readDataFromServer();            
        });
}

void Client::readDataFromStdin()
{
    std::cout << "\nEnter (new) version to send: " << std::endl;
    m_sentVersion = std::numeric_limits<decltype(m_sentVersion)>::max();

    do {
        // Unfortunatelly Windows doesn't provide an easy way to 
        // asynchronously read from stdin with boost::asio,
        // read synchronously. DON'T COPY-PASTE TO PRODUCTION CODE!!!
        std::cin >> m_sentVersion;
        if (!std::cin.good()) {
            std::cerr << "ERROR: Unexpected input, use numeric value" << std::endl;
            std::cin.clear();
            std::cin.ignore();
            break;
        }

        sendConnect();
        sendMsg1();
    } while (false);

    common::boost_wrap::post(
        m_io,
        [this]()
        {
            readDataFromStdin();
        });
}

void Client::sendConnect()
{
    demo3::message::Connect<OutputMsg> msg;
    msg.field_version().value() = m_sentVersion;
    sendMessage(msg);
}

void Client::sendMsg1()
{
    demo3::message::Msg1<OutputMsg> msg;
    msg.transportField_version().value() = m_sentVersion;
    msg.doRefresh();
    sendMessage(msg);
}

void Client::sendMessage(const OutputMsg& msg)
{
    std::vector<std::uint8_t> outputBuf;
    outputBuf.reserve(m_frame.length(msg));
    auto iter = std::back_inserter(outputBuf);
    auto es = m_frame.write(msg, iter, outputBuf.max_size());
    if (es == comms::ErrorStatus::UpdateRequired) {
        auto updateIter = &outputBuf[0];
        es = m_frame.update(updateIter, outputBuf.size());
    }

    if (es != comms::ErrorStatus::Success) {
        assert(!"Unexpected error");
        return;
    }

    std::cout << "INFO: Sending message: " << msg.name() << '\n';
    std::cout << "--> " << std::hex;
    std::copy(outputBuf.begin(), outputBuf.end(), std::ostream_iterator<unsigned>(std::cout, " "));
    std::cout << std::dec << std::endl;
    m_socket.send(boost::asio::buffer(outputBuf));
}

void Client::processInput()
{
    if (!m_inputBuf.empty()) {
        auto consumed = comms::processAllWithDispatch(&m_inputBuf[0], m_inputBuf.size(), m_frame, *this);
        m_inputBuf.erase(m_inputBuf.begin(), m_inputBuf.begin() + consumed);
    }
}

} // namespace client

} // namespace demo3
