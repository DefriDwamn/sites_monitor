#include "utils.hpp"
#include <filesystem>
#include <format>
#include <fstream>

std::string make_http_req(std::string_view host, std::string_view path) {
  return std::format("{} {} HTTP/1.1\r\n"
                     "Host: {}\r\n"
                     "User-Agent: HealthChecker\r\n"
                     "Accept: */*\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     "HEAD", path, host);
}

std::vector<std::string> parse_sites_file(std::string_view sites_file_path,
                                          size_t avg_line_length) {
  std::filesystem::path path(sites_file_path);
  if (!std::filesystem::exists(path))
    throw std::logic_error(std::filesystem::absolute(path).generic_string() +
                           " not found");

  std::ifstream ifs(path);
  if (!ifs.is_open())
    throw std::runtime_error("failed to open " + path.generic_string());

  std::vector<std::string> sites;
  size_t maybe_lines_count = std::filesystem::file_size(path) / avg_line_length;
  sites.reserve(maybe_lines_count);

  std::string site;
  while (std::getline(ifs, site)) {
    if (!site.empty())
      sites.push_back(site);
  }

  return sites;
};

std::string make_log_entry(std::string_view site, std::string_view response) {
  size_t pos = response.find("\r\n");
  std::string_view status_line;
  if (pos != std::string_view::npos) {
    status_line = response.substr(0, pos);
  } else {
    status_line = response;
  }

  std::string combined = std::string(site) + " " + std::string(status_line);
  return combined;
}