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

static json get_all_files();
static void download_file(const std::string& file_name);
static void upload_file(const std::string& path);
static void delete_file(const std::string& filename);
static int print_files();
static std::string zero_arg();
static std::string options();
static void keywords();
static std::string example();

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << zero_arg() << std::endl << options() << std::endl << example() << std::endl;

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

    if (command == "del" && argc == 3) {
        try {
            const json files = get_all_files().at("files");
            const int index = std::stoi(argv[2]) - 1;

            if (index < 0 || index >= static_cast<int>(files.size())) {
                throw std::out_of_range("Invalid file index");
            }

            const std::string filename = files.at(index).get<std::string>();

            delete_file(filename);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }

        return 0;
    }

    if ((command == "del" || command == "ls") && argc == 2) {
        return print_files();
    }

    if (command == "get" && argc == 3) {
        try {
            const json files = get_all_files().at("files");

            const int index = std::stoi(argv[2]) - 1;

            if (index < 0 || index >= static_cast<int>(files.size())) {
                throw std::out_of_range("Invalid file index");
            }

            const std::string file_name = files.at(index).get<std::string>();

            download_file(file_name);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }

        return 0;
    }

    std::cout << zero_arg() << std::endl << options() << std::endl << example() << std::endl;

    return 0;
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

static int print_files() {
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

    return 0;
}

static std::string zero_arg() {
    return "Usage: fscli <option>";
}

static std::string options() {
    const std::string opt1 = "ls";
    const std::string opt2 = "get <filename_index>";

    return std::format("Options: \n\t- {} \n\t- {}", opt1, opt2);
}

static void keywords() {
    std::vector<std::string> ls_keywords = {"get", "up", "del", "ls"};

    std::cout << "Keywords: \n" << std::endl;

    for (const std::string& keyword : ls_keywords) {
        std::cout << keyword << std::endl;
    }
}

static std::string example() {
    return "Example: \n\t > fscli ls\n\t > fscli get 1";
}
