#pragma once

#include <boost/version.hpp>
#include <boost/asio.hpp>

namespace demo3
{

namespace common
{

namespace boost_wrap
{

#define DEMO3_BOOST_MAJOR (BOOST_VERSION / 100000)
#define DEMO3_BOOST_MINOR ((BOOST_VERSION / 100) % 1000)

#if (DEMO3_BOOST_MAJOR != 1)
#warning "Unexpected boost major version"
#endif

#if (DEMO3_BOOST_MINOR < 66)

using io = boost::asio::io_service;

template <typename TFunc>
void post(io& i, TFunc&& func)
{
    i.post(std::forward<TFunc>(func));
}

#else // #if (DEMO3_BOOST_MINOR < 66)

using io = boost::asio::io_context;

template <typename TFunc>
void post(io& i, TFunc&& func)
{
    boost::asio::post(i, std::forward<TFunc>(func));
}

#endif // #if (DEMO3_BOOST_MINOR < 66)

} // namespace boost_wrap


} // namespace common


} // namespace demo3
