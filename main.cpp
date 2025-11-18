#include "boost/asio/awaitable.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/registered_buffer.hpp"
#include "boost/asio/this_coro.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "utils.hpp"
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace this_coro = asio::this_coro;
using namespace asio::experimental::awaitable_operators;
using asio::awaitable;
using asio::buffer;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;
// using boost::system::error_code;

awaitable<void> health_check(std::string_view site) {
  auto ex = co_await this_coro::executor;

  tcp::resolver resolver(ex);
  tcp::socket socket(ex);

  std::string request = make_http_req(site);
  std::string response;
  const size_t MAX_HEADER_SIZE = 8192;
  response.reserve(MAX_HEADER_SIZE);

  asio::steady_timer timer(ex);

  auto resolver_res = co_await resolver.async_resolve(
      site, "80", tcp::resolver::numeric_service, use_awaitable);

  timer.expires_after(std::chrono::seconds(5));
  auto connection_res =
      co_await (asio::async_connect(socket, resolver_res, use_awaitable) ||
                timer.async_wait(use_awaitable));
  if (connection_res.index() == 1) {
    socket.close();
    co_return;
  }
  timer.cancel();

  co_await asio::async_write(socket, buffer(request), use_awaitable);

  timer.expires_after(std::chrono::seconds(5));
  auto read_res =
      co_await (asio::async_read_until(
                    socket, asio::dynamic_buffer(response, MAX_HEADER_SIZE),
                    "\r\n\r\n", use_awaitable) ||
                timer.async_wait(use_awaitable));
  if (read_res.index() == 1) {
    socket.close();
    co_return;
  };
  timer.cancel();

  std::cout << response << std::endl;
  socket.close();
}

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