#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

static json get_all_files();
static std::string zero_arg();
static std::string options();
static std::string example();

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << zero_arg() << std::endl << options() << std::endl << example() << std::endl;

        return 0;
    }

    if (static_cast<std::string>(argv[1]) == "get") {
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