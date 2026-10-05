#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifndef FETCHX_VERSION
#define FETCHX_VERSION "0.1.0"
#endif

struct Meta {
    std::string etag;
    std::string last_modified;
    std::string content_length;

    bool valid() const {
        return
            !etag.empty() ||
            !last_modified.empty() ||
            !content_length.empty();
    }
};

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

static std::string trim(std::string value) {
    while (!value.empty()) {
        char c = value.back();

        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            value.pop_back();
        } else {
            break;
        }
    }

    size_t start = 0;

    while (
        start < value.size() &&
        (value[start] == ' ' || value[start] == '\t')
    ) {
        ++start;
    }

    return value.substr(start);
}

static std::string capture(const std::string& command, int& status) {
    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe) {
        status = -1;
        return {};
    }

    std::string result;
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

    status = pclose(pipe);
    return result;
}

static Meta get_metadata(const std::string& url) {
    int status = 0;

    const std::string command =
        "curl --silent --show-error --head "
        "--location --connect-timeout 15 " +
        quote(url) +
        " 2>/dev/null";

    const std::string response =
        capture(command, status);

    if (status != 0) {
        return {};
    }

    Meta meta;

    std::istringstream input(response);
    std::string line;

    while (std::getline(input, line)) {
        line = trim(line);

        const auto colon = line.find(':');

        if (colon == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = trim(line.substr(colon + 1));

        for (char& c : key) {
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }

        if (key == "etag") {
            meta.etag = value;
        } else if (key == "last-modified") {
            meta.last_modified = value;
        } else if (key == "content-length") {
            meta.content_length = value;
        }
    }

    return meta;
}

static bool same(const Meta& a, const Meta& b) {
    return
        a.etag == b.etag &&
        a.last_modified == b.last_modified &&
        a.content_length == b.content_length;
}

static void save_state(const std::string& file, const Meta& meta) {
    std::ofstream out(file);

    out << "etag=" << meta.etag << "\n";
    out << "last_modified=" << meta.last_modified << "\n";
    out << "content_length=" << meta.content_length << "\n";
}

static bool load_state(const std::string& file, Meta& meta) {
    std::ifstream in(file);

    if (!in) {
        return false;
    }

    std::string line;

    while (std::getline(in, line)) {
        const auto equal = line.find('=');

        if (equal == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, equal);
        const std::string value = line.substr(equal + 1);

        if (key == "etag") {
            meta.etag = value;
        } else if (key == "last_modified") {
            meta.last_modified = value;
        } else if (key == "content_length") {
            meta.content_length = value;
        }
    }

    return true;
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
        << "watchurl " << FETCHX_VERSION << "\n"
        << "Monitor a URL for changes.\n\n"
        << "Usage:\n"
        << "  watchurl <URL> [options]\n\n"
        << "Options:\n"
        << "  -o, --output <file>   File to download when changed\n"
        << "      --interval <sec>  Check interval\n"
        << "      --once            Check once and exit\n"
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
    int interval = 60;
    bool once = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            help();
            return 0;
        }

        if (arg == "-v" || arg == "--version") {
            std::cout << "watchurl " << FETCHX_VERSION << "\n";
            return 0;
        }

        if (arg == "--once") {
            once = true;
            continue;
        }

        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "watchurl: missing output file\n";
                return 2;
            }

            output = argv[++i];
            continue;
        }

        if (arg == "--interval") {
            if (i + 1 >= argc) {
                std::cerr << "watchurl: missing interval\n";
                return 2;
            }

            interval = std::atoi(argv[++i]);

            if (interval < 1) {
                std::cerr << "watchurl: invalid interval\n";
                return 2;
            }

            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            std::cerr
                << "watchurl: unknown option: "
                << arg << "\n";
            return 2;
        }

        if (url.empty()) {
            url = arg;
        } else {
            std::cerr
                << "watchurl: only one URL is supported\n";
            return 2;
        }
    }

    if (url.empty()) {
        std::cerr << "watchurl: no URL provided\n";
        return 2;
    }

    if (
        url.rfind("http://", 0) != 0 &&
        url.rfind("https://", 0) != 0
    ) {
        std::cerr
            << "watchurl: URL must use http:// or https://\n";
        return 2;
    }

    if (output.empty()) {
        output = filename_from_url(url);
    }

    const std::string state_file =
        "/tmp/fetchx-watchurl.state";

    Meta previous;
    const bool had_state =
        load_state(state_file, previous);

    std::cout
        << "watchurl: monitoring "
        << url
        << " every "
        << interval
        << " seconds\n";

    while (true) {
        const Meta current =
            get_metadata(url);

        if (!current.valid()) {
            std::cerr
                << "watchurl: unable to read URL metadata\n";
        } else if (!had_state) {
            save_state(state_file, current);
            previous = current;

            std::cout
                << "[watchurl] Initial state saved.\n";
        } else if (!same(previous, current)) {
            std::cout
                << "[watchurl] Change detected.\n";

            const std::string command =
                "curl --fail --location --show-error "
                "--progress-bar --retry 3 "
                "-o " +
                quote(output) +
                " " +
                quote(url);

            const int status =
                std::system(command.c_str());

            if (status == 0) {
                previous = current;
                save_state(state_file, previous);

                std::cout
                    << "[watchurl] File updated.\n";
            } else {
                std::cerr
                    << "watchurl: download failed\n";
            }
        } else {
            std::cout
                << "[watchurl] No change.\n";
        }

        if (once) {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(interval)
        );
    }

    return 0;
}
