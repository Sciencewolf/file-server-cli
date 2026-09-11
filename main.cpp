#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace Color {
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* RESET = "\033[0m";
}


static json get_all_files() {
    const std::string url = "https://files.martonaron.dev/all";

    const cpr::Response res = cpr::Get(cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("HTTP error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(std::format("HTTP status error: {}", res.status_code));
    }

    return json::parse(res.text);
}

static void download_file(const std::string& file_name) {
    const std::string url = std::format("https://files.martonaron.dev/get/{}", file_name);

    const std::filesystem::path project_root = PROJECT_ROOT;
    const std::filesystem::path download_dir = project_root / "download";

    std::filesystem::create_directories(download_dir);

    const std::filesystem::path safe_file_name = std::filesystem::path(file_name).filename();

    if (safe_file_name.empty()) {
        throw std::runtime_error("Invalid file name");
    }

    const std::filesystem::path file_path = download_dir / safe_file_name;

    std::ofstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Error opening file: " + file_path.string());
    }

    const cpr::Response res = cpr::Download(file, cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("HTTP error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(std::format("HTTP status error: {}", res.status_code));
    }

    std::cout << "File downloaded: " << file_path.string() << std::endl;
}

static void upload_file(const std::string& path) {
    const std::string url = "https://files.martonaron.dev/upload";

    cpr::Response res = cpr::Post(cpr::Url{url}, cpr::Multipart{{"file", cpr::File{path}}});

    if (res.error) {
        throw std::runtime_error("HTTP error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(std::format("HTTP status error: {}", res.status_code));
    }

    std::cout << json::parse(res.text).at("info") << std::endl;

}

static void delete_file(const std::string& filename) {
    const std::string url = std::format("https://files.martonaron.dev/delete/{}", filename);

    const cpr::Response res = cpr::Delete(cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("HTTP error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(
            std::format(
                "HTTP status error: {} - {}",
                res.status_code,
                res.text
            )
        );
    }

    const json response = json::parse(res.text);

    std::cout << response.at("info").get<std::string>() << std::endl;
}

static void rename_file(const std::string old_name_index, const std::string new_name) {
    const json files = get_all_files().at("files");

    const std::string old_name = files.at(std::stoi(old_name_index) - 1).at("name").get<std::string>();

    std::string new_name_sanitized = std::format("{}.{}", new_name, old_name.substr(old_name.find_last_of('.') + 1));

    const std::string url = std::format("https://files.martonaron.dev/rename/{}?val={}", old_name, new_name_sanitized);

    const cpr::Response res = cpr::Get(cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("HTTP error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(
            std::format(
                "HTTP status error: {} - {}",
                res.status_code,
                res.text
            )
        );
    }

    const json response = json::parse(res.text);
    std::cout << response.at("info").get<std::string>() << std::endl;
}

static int print_files() {
    try {
        int cnt = 1;
        const json files = get_all_files().at("files");

        for (const auto& file : files) {
            std::cout << cnt++ << ": " << file["name"] << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

static void zero_arg() {
    std::cout << Color::BLUE << "Usage: fscli <option>" << Color::RESET;
}

static void options() {
    const std::string opt1 = "ls";
    const std::string opt2 = "get <filename_index>";
    const std::string opt3 = "up <path_to_file>";
    const std::string opt4 = "del <filename_index>";
    const std::string opt5 = "rn <old_filename_index> <new_filename>";

    std::cout << Color::GREEN << "\nOptions: " << std::endl;
    std::cout << "\t- " << opt1 << std::endl;
    std::cout << "\t- " << opt2 << std::endl;
    std::cout << "\t- " << opt3 << std::endl;
    std::cout << "\t- " << opt4 << std::endl;
    std::cout << "\t- " << opt5 << Color::RESET << std::endl;
}

static void keywords() {
    const std::vector<std::string> ls_keywords = {"get", "up", "del", "ls", "rn", "words"};

    std::cout << Color::GREEN << "Keywords: \n" << std::endl;

    for (const std::string& keyword : ls_keywords) {
        std::cout << keyword << std::endl;
    }

    std::cout << Color::RESET << std::endl;
}

static void example() {
    std::cout << Color::YELLOW << "Example: \n\t > fscli ls\n\t > fscli get 1\n\t > fscli rn 1 'new_file.txt'" << Color::RESET << std::endl;
}

static const void version() {
    std::cout << Color::MAGENTA << "v1.1(2026.09.11)\n\n" << Color::RESET;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        version();
        zero_arg(); 
        options();
        example();

        return 0;
    }

    const std::string command = argv[1];

    if (command == "words" && argc == 2) {
        keywords();

        return 0;
    }

    if (command == "up" && argc == 3) {
        upload_file(argv[2]);

        return 0;
    }

    if (command == "rn" && argc == 4) {
        rename_file(argv[2], argv[3]);

        return 0;
    }

    if (command == "del" && argc == 3) {
        try {
            const json files = get_all_files().at("files");

            const int index = std::stoi(argv[2]) - 1;

            if (index < 0 || index >= static_cast<int>(files.size())) {
                throw std::out_of_range("Invalid file index");
            }

            const std::string filename = files.at(index).at("name").get<std::string>();

            delete_file(filename);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }

        return 0;
    }

    if ((command == "del" || command == "ls" || command == "rn" || command == "get") && argc == 2) {
        return print_files();
    }

    if (command == "get" && argc == 3) {
        try {
            const json files = get_all_files().at("files");

            for(const auto& file : files) {
                std::cout << file["name"] << std::endl;
            }

            const int index = std::stoi(argv[2]) - 1;

            if (index < 0 || index >= static_cast<int>(files.size())) {
                throw std::out_of_range("Invalid file index");
            }

            const std::string file_name = files.at(index).at("name").get<std::string>();

            download_file(file_name);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }

        return 0;
    }

    zero_arg();
    options();
    example();

    return 0;
}
