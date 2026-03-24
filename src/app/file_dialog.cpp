#include "app/file_dialog.h"
#include <tinyfiledialogs.h>

namespace FileDialog {

std::string openFile(const char* title,
                     const char* defaultPath,
                     int numFilters,
                     const char* const* filterPatterns,
                     const char* filterDescription) {
    char* result = tinyfd_openFileDialog(
        title,
        defaultPath ? defaultPath : "",
        numFilters,
        filterPatterns,
        filterDescription,
        0);   // single select
    return result ? std::string(result) : std::string();
}

std::string saveFile(const char* title,
                     const char* defaultPath,
                     int numFilters,
                     const char* const* filterPatterns,
                     const char* filterDescription) {
    char* result = tinyfd_saveFileDialog(
        title,
        defaultPath ? defaultPath : "",
        numFilters,
        filterPatterns,
        filterDescription);
    return result ? std::string(result) : std::string();
}

} // namespace FileDialog
