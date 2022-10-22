#include "Session.h"

#include <iostream>
#include <iomanip>
#include <iterator>

#include "comms/process.h"

namespace cc_demo3
{

namespace server
{

namespace
{

template <typename TField>
void printVersionDependentField(const TField& f)
{
    std::cout << '\t' << f.field().name() << " = ";
    if (f.isMissing())  {
        std::cout << "(missing)";
    }
    else {
        std::cout << static_cast<unsigned>(f.field().value());
    }
    std::cout << '\n';
}    

} // namespace 

void Session::start()
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
                terminateSession();
                return;
            }

            std::cout << "<-- " << std::hex;
            std::copy_n(m_readBuf.begin(), bytesCount, std::ostream_iterator<unsigned>(std::cout, " "));
            std::cout << std::dec << std::endl;

            m_inputBuf.insert(m_inputBuf.end(), m_readBuf.begin(), m_readBuf.begin() + bytesCount);
            processInput();
            start();            
        });
}

void Session::handle(InConnect& msg)
{
    std::cout << 
        '\t' << msg.field_version().name() << " = " << 
        static_cast<unsigned>(msg.field_version().value()) << '\n' <<
        std::endl;

    // Treat all future incoming messages as having specified version.
    m_frame.layer_version().pseudoField().value() = msg.field_version().value();
}

void Session::handle(InMsg1& msg)
{
    std::cout << 
        '\t' << msg.field_f1().name() << " = " << 
        static_cast<unsigned>(msg.field_f1().value()) << '\n';
        printVersionDependentField(msg.field_f2());
        printVersionDependentField(msg.field_f3());
        printVersionDependentField(msg.field_f4());
    std::cout << std::endl;
}

void Session::handle(InputMsg&)
{
    std::cerr << "WARNING: Unexpected message received" << std::endl;
}

void Session::terminateSession()
{
    std::cout << "Terminating session to " << m_remote << std::endl;
    if (m_termCb) {
        common::boost_wrap::post(
            m_io,
            [this]()
            {
                m_termCb();
            });
    }
}

void Session::processInput()
{
    if (!m_inputBuf.empty()) {
        auto consumed = comms::processAllWithDispatch(&m_inputBuf[0], m_inputBuf.size(), m_frame, *this);
        m_inputBuf.erase(m_inputBuf.begin(), m_inputBuf.begin() + consumed);
    }
}

void Session::sendMessage(const OutputMsg& msg)
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
        static constexpr bool Unexpected_error = false;
        static_cast<void>(Unexpected_error);
        assert(Unexpected_error);
        return;
    }

    std::cout << "INFO: Sending back: " << msg.name() << '\n' ;
    std::cout << "--> " << std::hex;
    std::copy(outputBuf.begin(), outputBuf.end(), std::ostream_iterator<unsigned>(std::cout, " "));
    std::cout << std::dec << std::endl;
    m_socket.send(boost::asio::buffer(outputBuf));
}

} // namespace server

} // namespace cc_demo3
