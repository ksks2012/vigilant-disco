#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <string>

// C++ wrappers around tinyfiledialogs native file dialogs.
namespace FileDialog {

// Open a single file.  Returns empty string on cancel.
std::string openFile(const char* title,
                     const char* defaultPath,
                     int numFilters,
                     const char* const* filterPatterns,
                     const char* filterDescription);

// Save file dialog.  Returns empty string on cancel.
std::string saveFile(const char* title,
                     const char* defaultPath,
                     int numFilters,
                     const char* const* filterPatterns,
                     const char* filterDescription);

} // namespace FileDialog

#endif // FILE_DIALOG_H
