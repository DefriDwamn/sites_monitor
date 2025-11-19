#pragma once
#include <string>
#include <vector>

std::string make_http_req(std::string_view host, std::string_view path = "/");

std::vector<std::string> parse_sites_file(std::string_view sites_file_path,
                                          size_t avg_line_length = 64);

std::string make_log_entry(std::string_view site, std::string_view response);
