#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <fstream>

using json = nlohmann::json;

static json get_all_files();
static void get_file(const std::string& file_name);
static std::string zero_arg();
static std::string options();
static std::string example();

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << zero_arg() << std::endl << options() << std::endl << example() << std::endl;

        return 0;
    }

    if (static_cast<std::string>(argv[1]) == "get" && argc == 2) {
        try {
            int cnt = 1;
            const json files = get_all_files().at("files");

            for (const auto& file : files) {
                std::cout << cnt++ << ": " << file << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }
    }

    if (static_cast<std::string>(argv[1]) == "get" && argc == 3) {
        try {
            const json files = get_all_files().at("files");

            const int index = std::stoi(argv[2]) - 1;

            if (index < 0 || index >= static_cast<int>(files.size())) {
                throw std::out_of_range("Invalid file index");
            }

            const std::string file_name = files.at(index).get<std::string>();

            get_file(file_name);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }
    }

    return 0;
}

static json get_all_files() {
    const std::string url = "https://files.martonaron.dev/all";

    const cpr::Response res = cpr::Get(cpr::Url{url});

    if (res.error)
    {
        throw std::runtime_error(
            "HTTP error: " + res.error.message
        );
    }

    return json::parse(res.text);
}

static void get_file(const std::string& file_name) {
    const std::string url = std::format("https://files.martonaron.dev/get/{}", file_name);
    std::ofstream file(file_name, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Error opening file: " + file_name);
    }

    const cpr::Response res = cpr::Download(file, cpr::Url{url});
    if (res.error) {
        throw std::runtime_error("HTTP error: " + res.error.message);
    }

    std::cout << "File downloaded " << file_name << std::endl;
}

static std::string zero_arg() {
    return "Usage: fscli.exe <option>";
}

static std::string options() {
    std::string opt1 = "get";
    std::string opt2 = "get <filename_index>";

    return std::format("Options: \n\t- {} \n\t- {}", opt1, opt2);
}

static std::string example() {
    return std::format("Example: \n\t > file_server_cli.exe get\n\t > file_server_cli.exe get 1");
}