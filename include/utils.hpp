#pragma once
#include <filesystem>
#include <string>

std::string make_http_req(std::string_view host, std::string_view path = "/");

std::vector<std::string> parse_sites_file(std::string_view sites_file_path,
                                          size_t avg_line_length = 64);