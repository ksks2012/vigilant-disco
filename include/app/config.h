#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <stdexcept>

// Configuration error type
class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Application configuration loaded from JSON
struct Config {
    int logger_level = 1; // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR

    struct Window {
        int width  = 1280;
        int height = 720;
        std::string title = "Pin - Perler Bead Simulator";
    } window;

    void loadFromFile(const std::string& path);
};

#endif // CONFIG_H
