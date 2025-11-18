#include "boost/asio/awaitable.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/registered_buffer.hpp"
#include "utils.hpp"
#include <boost/asio.hpp>
#include <exception>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace this_coro = asio::this_coro;
using asio::awaitable;
using asio::buffer;
using asio::co_spawn;
using asio::detached;
using asio::ip::tcp;
// using boost::system::error_code;
//  using namespace asio::experimental::awaitable_operators;

awaitable<void> health_check(std::string_view site) { co_return; }

int main() {
  try {
    asio::io_context ctx;
    auto sites = parse_sites_file("sites.txt");

    for (const auto &site : sites) {
      co_spawn(ctx, health_check(site), detached);
    }

    std::vector<std::jthread> threads;
    for (auto i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
      threads.emplace_back([&ctx] { ctx.run(); });
    }

  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}