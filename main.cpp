#include "blockingconcurrentqueue.h"
#include "utils.hpp"
#include <boost/asio.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace this_coro = asio::this_coro;
using namespace asio::experimental::awaitable_operators;
using asio::awaitable;
using asio::buffer;
using asio::co_spawn;
using asio::detached;
using asio::ip::tcp;
using boost::system::error_code;
using std::chrono::steady_clock;
constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);

awaitable<void> timeout(steady_clock::duration duration) {
  asio::steady_timer timer(co_await this_coro::executor);
  timer.expires_after(duration);
  co_await timer.async_wait(use_nothrow_awaitable);
}

template <typename Queue>
awaitable<void> health_check(std::string_view site, Queue &queue,
                             std::atomic<int> &active_coro) {
  auto ex = co_await this_coro::executor;
  struct Decrement {
    std::atomic<int> &ref;
    ~Decrement() { --ref; }
  } dec{active_coro};

  tcp::resolver resolver(ex);
  tcp::socket socket(ex);

  std::string request = make_http_req(site);
  std::string response;
  const size_t MAX_HEADER_SIZE = 8192;
  response.reserve(MAX_HEADER_SIZE);

  auto [resolver_err, resolver_res] = co_await resolver.async_resolve(
      site, "80", tcp::resolver::numeric_service, use_nothrow_awaitable);

  auto con_tuple = co_await (
      asio::async_connect(socket, resolver_res, use_nothrow_awaitable) ||
      timeout(std::chrono::seconds(3)));

  // timed out
  if (con_tuple.index() == 1) {
    socket.close();
    queue.enqueue(make_log_entry(site, "Conection to socket timed out"));
    co_return;
  }

  auto [con_err, con_res] = std::get<0>(con_tuple);
  if (con_err) {
    socket.close();
    queue.enqueue(make_log_entry(site, "Conection to socket error"));
    co_return;
  }

  auto [write_err, write_res] = co_await asio::async_write(
      socket, buffer(request), use_nothrow_awaitable);
  if (write_err) {
    socket.close();
    queue.enqueue(make_log_entry(site, "Write to socket error"));
    co_return;
  }

  auto read_tuple =
      co_await (asio::async_read_until(
                    socket, asio::dynamic_buffer(response, MAX_HEADER_SIZE),
                    "\r\n\r\n", use_nothrow_awaitable) ||
                timeout(std::chrono::seconds(5)));

  // timed out
  if (read_tuple.index() == 1) {
    socket.close();
    queue.enqueue(make_log_entry(site, "Read from socket timed out"));
    co_return;
  };

  auto [read_err, read_res] = std::get<0>(read_tuple);
  if (read_err) {
    socket.close();
    queue.enqueue(make_log_entry(site, "Read from socket error"));
    co_return;
  }

  queue.enqueue(make_log_entry(site, response));
  socket.close();
}

template <typename Queue>
void health_results_writer(Queue &queue, std::atomic<int> &active_coro) {
  std::ofstream log("results.txt", std::ios::out | std::ios::trunc);
  if (!log.is_open())
    throw std::runtime_error("failed to open results file");

  while (true) {
    std::string res;
    if (queue.wait_dequeue_timed(res, std::chrono::milliseconds(200))) {
      log << res << std::endl;
      continue;
    }
    if (active_coro == 0 && queue.size_approx() == 0)
      break;
  }
};

int main() {
  try {
    asio::io_context ctx;
    auto sites = parse_sites_file("sites.txt");

    moodycamel::BlockingConcurrentQueue<std::string> queue;

    std::atomic<int> active_coro = sites.size();
    for (const auto &site : sites) {
      co_spawn(ctx, health_check(site, queue, active_coro), detached);
    }

    std::vector<std::jthread> threads;
    for (auto i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
      threads.emplace_back([&ctx] { ctx.run(); });
    }

    std::jthread writer([&] { health_results_writer(queue, active_coro); });

  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}