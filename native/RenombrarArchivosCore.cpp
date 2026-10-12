#include <cwchar>
#include <string>
#include <algorithm>
#include <cwctype>

#ifdef _WIN32
#define REN_EXPORT extern "C" __declspec(dllexport)
#define REN_CALL __cdecl
#else
#define REN_EXPORT extern "C"
#define REN_CALL
#endif

static bool is_space(wchar_t c) {
    return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n';
}

static bool is_separator(wchar_t c) {
    return c == L'-' || c == L'_' || c == L'.';
}

static std::wstring normalize_digits(std::wstring digits) {
    const auto firstNonZero = digits.find_first_not_of(L'0');
    digits = firstNonZero == std::wstring::npos ? L"0" : digits.substr(firstNonZero);
    if (digits.size() < 3) digits.insert(0, 3 - digits.size(), L'0');
    return digits;
}

// Reads the number from the filename itself, never from the row/list position.
// Handles both "8 - Title.mkv" and "Title008.mkv" -> "008 - Title.mkv".
// Returns 1 if it recognized a number; 0 means leave the name untouched.
REN_EXPORT int REN_CALL NormalizeNumberPrefix(
    const wchar_t* stem,
    wchar_t* output,
    int outputCount
) {
    if (!stem || !output || outputCount <= 0) return 0;
    output[0] = L'\0';
    std::wstring name(stem);
    if (name.empty()) return 0;

    // Case 1: number at the beginning, optionally followed by spaces and
    // a separator. Examples: "8 - Title", "008_Title", "25.Title".
    size_t begin = 0;
    while (begin < name.size() && is_space(name[begin])) ++begin;
    size_t endDigits = begin;
    while (endDigits < name.size() && name[endDigits] >= L'0' && name[endDigits] <= L'9') ++endDigits;

    std::wstring digits;
    std::wstring title;
    if (endDigits > begin && (endDigits == name.size() || is_space(name[endDigits]) || is_separator(name[endDigits]))) {
        digits = name.substr(begin, endDigits - begin);
        size_t rest = endDigits;
        while (rest < name.size() && is_space(name[rest])) ++rest;
        if (rest < name.size() && is_separator(name[rest])) {
            ++rest;
            while (rest < name.size() && is_space(name[rest])) ++rest;
        }
        title = name.substr(rest);
    } else {
        // Case 2: number at the end, even when attached to the title.
        // Examples: "Title8", "Title 008", "Title_25", "Title-103".
        size_t suffixStart = name.size();
        while (suffixStart > 0 && name[suffixStart - 1] >= L'0' && name[suffixStart - 1] <= L'9') --suffixStart;
        if (suffixStart == name.size() || suffixStart == 0) return 0;
        digits = name.substr(suffixStart);
        title = name.substr(0, suffixStart);
        while (!title.empty() && (is_space(title.back()) || is_separator(title.back()))) title.pop_back();
        // Don't turn a name made only of separators into a title.
        if (title.empty()) return 0;
    }

    digits = normalize_digits(digits);
    std::wstring result = digits;
    if (!title.empty()) {
        result += L" - ";
        result += title;
    }
    if (result.size() + 1 > static_cast<size_t>(outputCount)) return 0;
    std::wmemcpy(output, result.c_str(), result.size() + 1);
    return 1;
}
