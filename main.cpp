#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <shlobj.h>
#endif

using json = nlohmann::json;

#if defined(_WIN32)
static void enable_ansi_colors() {
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (stdout_handle == INVALID_HANDLE_VALUE || !GetConsoleMode(stdout_handle, &mode)) {
        return;
    }

    SetConsoleMode(stdout_handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#endif

namespace Color {
    constexpr const char* RED = "\033[1;91m";
    constexpr const char* GREEN = "\033[1;92m";
    constexpr const char* YELLOW = "\033[1;93m";
    constexpr const char* BLUE = "\033[1;94m";
    constexpr const char* MAGENTA = "\033[1;95m";
    constexpr const char* CYAN = "\033[1;96m";
    constexpr const char* RESET = "\033[0m";
}

constexpr const char* VERSION = "v1.4.5";
constexpr const char* DESCRIPTION = "File Server CLI - A simple command line interface for file management";
constexpr const char* GITHUB_URL = "GitHub: https://github.com/Sciencewolf/file-server-cli\n";

static std::string url_encode(const std::string& value) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(value.size());

    for (const unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            encoded.push_back(static_cast<char>(ch));
        }
        else {
            encoded.push_back('%');
            encoded.push_back(hex[ch >> 4]);
            encoded.push_back(hex[ch & 0x0F]);
        }
    }

    return encoded;
}

static std::size_t parse_file_index(const std::string& index_arg, const json& files) {
    if (index_arg.empty() || !std::ranges::all_of(index_arg, [](const unsigned char ch) { return std::isdigit(ch); })) {
        throw std::invalid_argument("Invalid file index");
    }

    const unsigned long long index = std::stoull(index_arg);

    if (index < 1 || index > static_cast<unsigned long long>(files.size())) {
        throw std::out_of_range("Invalid file index");
    }

    return static_cast<std::size_t>(index - 1);
}

static std::filesystem::path get_downloads_dir() {
#if defined(_WIN32)
    PWSTR path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path);

    if (FAILED(result)) {
        throw std::runtime_error("Could not resolve the Downloads folder");
    }

    const std::filesystem::path downloads_dir(path);
    CoTaskMemFree(path);

    return downloads_dir;
#else
    if (const char* xdg_download_dir = std::getenv("XDG_DOWNLOAD_DIR")) {
        return std::filesystem::path(xdg_download_dir);
    }

    const char* home = std::getenv("HOME");

    if (!home) {
        throw std::runtime_error("HOME environment variable is not set");
    }

    return std::filesystem::path(home) / "Downloads";
#endif
}

static bool server_state() {
    const std::string url = "https://files.martonaron.dev/connection";

    try {
        const cpr::Response res = cpr::Get(cpr::Url{url});

        if (res.error) {
            throw std::runtime_error("Server connection error: " + res.error.message);
        }

        if (res.status_code < 200 || res.status_code >= 300) {
            throw std::runtime_error("Server is down");
        }
    }
    catch (const std::exception& e) {
        return false;
    }

    return true;
}

static json get_all_files() {
    const std::string url = "https://files.martonaron.dev/all";

    const cpr::Response res = cpr::Get(cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("Server connection error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(std::format("Server is down: {}", res.status_code));
    }

    return json::parse(res.text);
}

static void download_file(const std::string& file_name) {
    std::cout << Color::BLUE << "Downloading file..." << Color::RESET << std::flush;
    
    const std::string url = std::format("https://files.martonaron.dev/get/{}", url_encode(file_name));

    const std::filesystem::path download_dir = get_downloads_dir();

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
        throw std::runtime_error("Server connection error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(std::format("Server is down: {}", res.status_code));
    }

    std::cout << "\r\033[2K" << std::flush;

    std::cout << Color::BLUE << "File downloaded: " << file_path.string() <<  Color::RESET << std::endl;
}

static void upload_file(const std::string& path) {
    std::cout << Color::BLUE << "Uploading file..." << Color::RESET << std::flush;

    const std::filesystem::path file_path(path);

    if (!std::filesystem::is_regular_file(file_path)) {
        throw std::runtime_error("File does not exist or is not a regular file: " + path);
    }

    const std::string url = "https://files.martonaron.dev/upload";

    cpr::Response res = cpr::Post(cpr::Url{url}, cpr::Multipart{{"file", cpr::File{path}}});

    if (res.error) {
        throw std::runtime_error("Server connection error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(std::format("Server is down: {}", res.status_code));
    }

    std::cout << "\r\033[2K" << std::flush;

    std::cout << Color::BLUE << json::parse(res.text).at("info") << Color::RESET << std::endl;

}

static void delete_file(const std::string& filename) {
    std::cout << Color::BLUE << "Deleting file..." << Color::RESET << std::flush;

    const std::string url = std::format("https://files.martonaron.dev/delete/{}", url_encode(filename));

    const cpr::Response res = cpr::Delete(cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("Server connection error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(
            std::format(
                "Server is down: {} - {}",
                res.status_code,
                res.text
            )
        );
    }

    const json response = json::parse(res.text);

    std::cout << "\r\033[2K" << std::flush;

    std::cout << Color::GREEN << response.at("info").get<std::string>() << Color::RESET << std::endl;
}

static void rename_file(const std::string old_name_index, const std::string new_name) {
    std::cout << Color::BLUE << "Renaming file..." << Color::RESET << std::flush;

    const json files = get_all_files().at("files");

    const std::size_t index = parse_file_index(old_name_index, files);
    const std::string old_name = files.at(index).at("name").get<std::string>();

    if (new_name.empty()) {
        throw std::invalid_argument("New file name cannot be empty");
    }

    std::string new_name_sanitized = new_name;
    const std::size_t extension_pos = old_name.find_last_of('.');

    if (extension_pos != std::string::npos && extension_pos + 1 < old_name.size()) {
        new_name_sanitized = std::format("{}.{}", new_name, old_name.substr(extension_pos + 1));
    }

    const std::string url = std::format(
        "https://files.martonaron.dev/rename/{}?val={}",
        url_encode(old_name),
        url_encode(new_name_sanitized)
    );

    const cpr::Response res = cpr::Get(cpr::Url{url});

    if (res.error) {
        throw std::runtime_error("Server connection error: " + res.error.message);
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        throw std::runtime_error(
            std::format(
                "Server is down: {} - {}",
                res.status_code,
                res.text
            )
        );
    }

    const json response = json::parse(res.text);

    std::cout << "\r\033[2K" << std::flush;

    std::cout << Color::BLUE << response.at("info").get<std::string>() << Color::RESET << std::endl;
}


static int print_preview_url(const std::string& index_arg) {
    std::cout << Color::BLUE << "Fetching file list..." << Color::RESET << std::flush;

    const json files = get_all_files().at("files");
    const std::size_t file_index = parse_file_index(index_arg, files);
    const std::string filename = files.at(file_index).at("name").get<std::string>();

    const std::string url = std::format("https://files.martonaron.dev/data/{}", url_encode(filename));

    std::cout << "\r\033[2K" << std::flush;

    std::cout << Color::YELLOW << "WebViewLink: " << url << Color::RESET << std::endl;

    return 0;
}

static int print_files() {
    try {
        std::cout << Color::BLUE << "Fetching file list..." << Color::RESET << std::flush;

        int cnt = 1;
        const json files = get_all_files().at("files");

        std::cout << "\r\033[2K" << std::flush;

        std::cout << Color::YELLOW << std::endl;

        for (const auto& file : files) {
            std::cout << cnt++ << ": " << file.at("name").get<std::string>() << std::endl;
        }

        std::cout << Color::RESET << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "\r\033[2K" << std::flush;

        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << '\n';
        return 1;
    }

    return 0;
}

static void zero_arg() {
    std::cout << Color::BLUE << "Usage: fscli <option>\n" << Color::RESET;
}

static void options() {
    const std::string opt1 = "[ls, -l]";
    const std::string opt2 = "[get, -g] <filename_index>";
    const std::string opt3 = "[up, -u] <path_to_file>";
    const std::string opt4 = "[del, -d] <filename_index>";
    const std::string opt5 = "[rn, -r] <old_filename_index> <new_filename>";
    const std::string opt6 = "[prev, -p] <filename_index>";
    const std::string opt7 = "[words, -w]";

    std::cout << Color::GREEN << "\nOptions: " << std::endl;
    std::cout << "\t> " << opt1 << std::endl;
    std::cout << "\t> " << opt2 << std::endl;
    std::cout << "\t> " << opt3 << std::endl;
    std::cout << "\t> " << opt4 << std::endl;
    std::cout << "\t> " << opt5 << std::endl;
    std::cout << "\t> " << opt6 << std::endl;
    std::cout << "\t> " << opt7 << Color::RESET << std::endl;
}

static void keywords() {
    const std::vector<std::string> ls_keywords = {"get", "up", "del", "ls", "rn", "prev", "words"};

    std::cout << Color::GREEN << "Keywords: \n" << std::endl;

    for (const std::string& keyword : ls_keywords) {
        std::cout << keyword << std::endl;
    }

    std::cout << Color::RESET << std::endl;
}

static void example() {
    const std::string ex1 = "\t> fscli ls\n";
    const std::string ex2 = "\t> fscli get 1\n";
    const std::string ex3 = "\t> fscli rn 1 'new_file'\n";
    const std::string ex4 = "\t> fscli up 'path_to_file'\n";
    const std::string ex5 = "\t> fscli del 1\n";
    const std::string ex6 = "\t> fscli prev 1\n";

    std::cout << Color::YELLOW << "Example: \n" << ex1 << ex2 << ex3 << ex4 << ex5 << ex6 << Color::RESET;
}

static void about() {
    std::cout << Color::MAGENTA << VERSION << "\n\n" << Color::RESET;
    std::cout << Color::CYAN << DESCRIPTION << Color::RESET << std::endl;
    std::cout << Color::CYAN << GITHUB_URL << Color::RESET;
}

static int handle_list(const std::vector<std::string>&) {
    return print_files();
}

static int handle_words(const std::vector<std::string>&) {
    keywords();
    return 0;
}

static int handle_preview(const std::vector<std::string>& args) {
    try {
        return print_preview_url(args[0]);
    }
    catch (const std::exception& e) {
        std::cout << "\r\033[2K" << std::flush;

        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << '\n';
        return 1;
    }
}

static int handle_upload(const std::vector<std::string>& args) {
    try {
        upload_file(args[0]);
    }
    catch (const std::exception& e) {
        std::cout << "\r\033[2K" << std::flush;

        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << '\n';
        return 1;
    }

    return 0;
}

static int handle_rename(const std::vector<std::string>& args) {
    try {
        rename_file(args[0], args[1]);
    }
    catch (const std::exception& e) {
        std::cout << "\r\033[2K" << std::flush;

        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << '\n';
        return 1;
    }

    return 0;
}

static int handle_delete(const std::vector<std::string>& args) {
    try {
        const json files = get_all_files().at("files");

        const std::size_t index = parse_file_index(args[0], files);
        const std::string filename = files.at(index).at("name").get<std::string>();

        delete_file(filename);
    }
    catch (const std::exception& e) {
        std::cout << "\r\033[2K" << std::flush;
        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << '\n';
        return 1;
    }

    return 0;
}

static int handle_download(const std::vector<std::string>& args) {
    try {
        const json files = get_all_files().at("files");

        const std::size_t index = parse_file_index(args[0], files);
        const std::string file_name = files.at(index).at("name").get<std::string>();

        download_file(file_name);
    }
    catch (const std::exception& e) {
        std::cout << "\r\033[2K" << std::flush;

        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << '\n';
        return 1;
    }

    return 0;
}

using CommandHandler = std::function<int(const std::vector<std::string>&)>;

struct CommandVariant {
    size_t arg_count;
    CommandHandler handler;
};

static const std::unordered_map<std::string, std::vector<CommandVariant>> commands = {
    {"ls",    {{0, handle_list}}},
    {"get",   {{0, handle_list}, {1, handle_download}}},
    {"del",   {{0, handle_list}, {1, handle_delete}}},
    {"rn",    {{0, handle_list}, {2, handle_rename}}},
    {"up",    {{1, handle_upload}}},
    {"prev", {{1, handle_preview}}},
    {"words", {{0, handle_words}}},

    // - command versions
    {"-l",    {{0, handle_list}}},
    {"-g",   {{0, handle_list}, {1, handle_download}}},
    {"-d",   {{0, handle_list}, {1, handle_delete}}},
    {"-r",    {{0, handle_list}, {2, handle_rename}}},
    {"-u",    {{1, handle_upload}}},
    {"-p", {{1, handle_preview}}},
    {"-w", {{0, handle_words}}},
};

static void print_usage() {
    zero_arg();
    options();
    example();
}

int main(int argc, char** argv) {
#if defined(_WIN32)
    enable_ansi_colors();
#endif

    const std::vector<std::string> args(argv + 1, argv + argc);

    if (!server_state()) {
        std::cerr << Color::RED << "Server is offline\n" << Color::RESET << std::endl;
        about();

        return 1;
    }

    if (args.empty()) {
        std::cout << Color::GREEN << "Server is online" << Color::RESET << std::endl;
        about();
        print_usage();
        return 0;
    }

    const std::string& command = args[0];
    const std::vector<std::string> command_args(args.begin() + 1, args.end());

    const auto command_it = commands.find(command);

    if (command_it != commands.end()) {
        const std::vector<CommandVariant>& variants = command_it->second;

        const auto variant_it = std::find_if(
            variants.begin(),
            variants.end(),
            [&](const CommandVariant& variant) { return variant.arg_count == command_args.size(); }
        );

        if (variant_it != variants.end()) {
            return variant_it->handler(command_args);
        }
    }

    print_usage();
    return 1;
}