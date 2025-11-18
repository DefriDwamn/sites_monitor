#include "utils.hpp"
#include <filesystem>
#include <format>
#include <fstream>

std::string make_htt_req(std::string_view host, std::string_view path) {
  return std::format("{} {} HTTP/1.1\r\n"
                     "Host: {}\r\n"
                     "User-Agent: HealthChecker\r\n"
                     "Accept: */*\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     "GET", path, host);
}

std::vector<std::string> parse_sites_file(std::string_view sites_file_path,
                                          size_t avg_line_length) {
  std::filesystem::path path(sites_file_path);
  std::ifstream ifs(path);

  if (!std::filesystem::exists(path))
    throw std::logic_error(std::filesystem::absolute(path).generic_string() +
                           " not found");
  if (!ifs.is_open())
    throw std::runtime_error("failed to open " + path.generic_string());

  std::vector<std::string> sites;
  size_t maybe_lines_count = std::filesystem::file_size(path) / avg_line_length;
  sites.reserve(maybe_lines_count);

  std::string site;
  if (std::getline(ifs, site)) {
    if (!site.empty())
      sites.push_back(std::move(site));
  }

  return sites;
};