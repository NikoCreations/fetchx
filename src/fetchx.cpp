#include <cstdlib>
#include <iostream>
#include <string>

#ifndef FETCHX_VERSION
#define FETCHX_VERSION "0.1.0"
#endif

static std::string quote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

static std::string filename_from_url(std::string url) {
    const auto query = url.find_first_of("?#");

    if (query != std::string::npos) {
        url.erase(query);
    }

    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }

    const auto slash = url.find_last_of('/');

    if (slash == std::string::npos || slash + 1 >= url.size()) {
        return "download";
    }

    return url.substr(slash + 1);
}

static void help() {
    std::cout
        << "fetchx " << FETCHX_VERSION << "\n"
        << "Fast terminal downloader.\n\n"
        << "Usage:\n"
        << "  fetchx <URL> [options]\n\n"
        << "Options:\n"
        << "  -o, --output <file>   Output file\n"
        << "  -r, --resume          Resume download\n"
        << "      --watch           Start watchurl after download\n"
        << "      --interval <sec>  watchurl interval\n"
        << "  -h, --help            Show help\n"
        << "  -v, --version         Show version\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        help();
        return 1;
    }

    std::string url;
    std::string output;

    bool resume = false;
    bool watch = false;
    int interval = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            help();
            return 0;
        }

        if (arg == "-v" || arg == "--version") {
            std::cout << "fetchx " << FETCHX_VERSION << "\n";
            return 0;
        }

        if (arg == "-r" || arg == "--resume") {
            resume = true;
            continue;
        }

        if (arg == "--watch") {
            watch = true;
            continue;
        }

        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "fetchx: missing output file\n";
                return 2;
            }

            output = argv[++i];
            continue;
        }

        if (arg == "--interval") {
            if (i + 1 >= argc) {
                std::cerr << "fetchx: missing interval\n";
                return 2;
            }

            interval = std::atoi(argv[++i]);

            if (interval < 1) {
                std::cerr << "fetchx: invalid interval\n";
                return 2;
            }

            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            std::cerr << "fetchx: unknown option: " << arg << "\n";
            return 2;
        }

        if (url.empty()) {
            url = arg;
        } else {
            std::cerr << "fetchx: only one URL is supported\n";
            return 2;
        }
    }

    if (url.empty()) {
        std::cerr << "fetchx: no URL provided\n";
        return 2;
    }

    if (
        url.rfind("http://", 0) != 0 &&
        url.rfind("https://", 0) != 0
    ) {
        std::cerr << "fetchx: URL must use http:// or https://\n";
        return 2;
    }

    if (output.empty()) {
        output = filename_from_url(url);
    }

    std::cout << "==> Downloading " << url << "\n";
    std::cout << "==> Saving to " << output << "\n";

    std::string command =
        "curl --fail --location --show-error "
        "--progress-bar --retry 3 --retry-delay 1 "
        "--connect-timeout 15 ";

    if (resume) {
        command += "--continue-at - ";
    }

    command +=
        "-o " + quote(output) +
        " " + quote(url);

    const int status = std::system(command.c_str());

    if (status != 0) {
        std::cerr << "fetchx: download failed\n";
        return 1;
    }

    std::cout << "==> Download complete\n";

    if (watch) {
        std::string watch_command =
            "watchurl " +
            quote(url) +
            " --output " +
            quote(output) +
            " --interval " +
            std::to_string(interval);

        std::cout << "==> Starting watchurl\n";

        const int watch_status =
            std::system(watch_command.c_str());

        if (watch_status != 0) {
            return 1;
        }
    }

    return 0;
}
