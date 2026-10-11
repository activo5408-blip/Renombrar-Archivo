#include <cwchar>
#include <string>
#include <algorithm>

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

// Returns 1 when a numeric prefix was found, 0 when the stem should stay unchanged.
// The number comes from the original filename; list position is never used.
REN_EXPORT int REN_CALL NormalizeNumberPrefix(
    const wchar_t* stem,
    wchar_t* output,
    int outputCount
) {
    if (!stem || !output || outputCount <= 0) return 0;
    output[0] = L'\0';

    const wchar_t* p = stem;
    while (is_space(*p)) ++p;
    const wchar_t* digitsStart = p;
    while (*p >= L'0' && *p <= L'9') ++p;
    if (p == digitsStart) return 0;

    std::wstring digits(digitsStart, p);
    // A numeric prefix must end at the stem or be separated from the title.
    if (*p != L'\0' && !is_space(*p) && !is_separator(*p)) return 0;

    const wchar_t* rest = p;
    while (is_space(*rest)) ++rest;
    if (is_separator(*rest)) {
        ++rest;
        while (is_space(*rest)) ++rest;
    }

    // Normalize leading zeroes, then pad to at least three digits without
    // converting to an integer (so very large episode numbers remain safe).
    const auto firstNonZero = digits.find_first_not_of(L'0');
    digits = firstNonZero == std::wstring::npos ? L"0" : digits.substr(firstNonZero);
    if (digits.size() < 3) digits.insert(0, 3 - digits.size(), L'0');

    std::wstring result = digits;
    if (*rest != L'\0') {
        result += L" - ";
        result += rest;
    }

    if (result.size() + 1 > static_cast<size_t>(outputCount)) return 0;
    std::wmemcpy(output, result.c_str(), result.size() + 1);
    return 1;
}
