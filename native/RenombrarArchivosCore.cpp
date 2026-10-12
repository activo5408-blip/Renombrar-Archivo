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

    // Case 0: descriptive Spanish/English episode names, e.g.
    // "Ver You - Temporada 2 Episodio 3 - EntrePeliculasySeries"
    // "Ver You- Temporada 2 Capitulo 1 Online Latino HD - Pelisplus".
    // Extract the number that follows an episode/chapter word, regardless of
    // whether the season number appears before it. Keep the complete original
    // title so source/site information is not lost.
    {
        std::wstring lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
            [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        const wchar_t* episodeWords[] = {
            L"episodio", L"episodios", L"capitulo", L"capítulo", L"capitulos", L"capítulos",
            L"episode", L"episodes", L"chapter", L"chapters"
        };
        size_t bestNumberStart = std::wstring::npos;
        for (const wchar_t* word : episodeWords) {
            size_t pos = 0;
            const std::wstring needle(word);
            while ((pos = lowerName.find(needle, pos)) != std::wstring::npos) {
                size_t p = pos + needle.size();
                while (p < lowerName.size() && is_space(lowerName[p])) ++p;
                if (p < lowerName.size() && (lowerName[p] == L'.' || lowerName[p] == L'#' || lowerName[p] == L':')) {
                    ++p;
                    while (p < lowerName.size() && is_space(lowerName[p])) ++p;
                }
                size_t numberStart = p;
                while (p < lowerName.size() && lowerName[p] >= L'0' && lowerName[p] <= L'9') ++p;
                if (p > numberStart) {
                    bestNumberStart = numberStart;
                    break;
                }
                pos += needle.size();
            }
            if (bestNumberStart != std::wstring::npos) break;
        }
        if (bestNumberStart != std::wstring::npos) {
            size_t numberEnd = bestNumberStart;
            while (numberEnd < name.size() && name[numberEnd] >= L'0' && name[numberEnd] <= L'9') ++numberEnd;
            std::wstring digits = normalize_digits(name.substr(bestNumberStart, numberEnd - bestNumberStart));
            std::wstring result = digits + L" - " + name;
            if (result.size() + 1 <= static_cast<size_t>(outputCount)) {
                std::wmemcpy(output, result.c_str(), result.size() + 1);
                return 1;
            }
            return 0;
        }
    }

    // Case 1: TV episode names such as "S03E01 [Personalizar AVI 320x240 XVID]".
    // Use the episode number (E01 -> 001), omit the season/episode marker,
    // and preserve the remaining title. This keeps separate episodes distinct.
    size_t episodeStart = 0;
    while (episodeStart < name.size() && is_space(name[episodeStart])) ++episodeStart;
    if (episodeStart < name.size() &&
        (name[episodeStart] == L'S' || name[episodeStart] == L's')) {
        size_t seasonEnd = episodeStart + 1;
        while (seasonEnd < name.size() && name[seasonEnd] >= L'0' && name[seasonEnd] <= L'9') ++seasonEnd;
        if (seasonEnd > episodeStart + 1 && seasonEnd < name.size() &&
            (name[seasonEnd] == L'E' || name[seasonEnd] == L'e')) {
            size_t episodeEnd = seasonEnd + 1;
            while (episodeEnd < name.size() && name[episodeEnd] >= L'0' && name[episodeEnd] <= L'9') ++episodeEnd;
            if (episodeEnd > seasonEnd + 1 &&
                (episodeEnd == name.size() || is_space(name[episodeEnd]) || is_separator(name[episodeEnd]) ||
                 name[episodeEnd] == L'[' || name[episodeEnd] == L'(')) {
                std::wstring episodeDigits = name.substr(seasonEnd + 1, episodeEnd - seasonEnd - 1);
                std::wstring episodeTitle = name.substr(episodeEnd);
                while (!episodeTitle.empty() &&
                       (is_space(episodeTitle.front()) || is_separator(episodeTitle.front())))
                    episodeTitle.erase(episodeTitle.begin());
                std::wstring episodeResult = normalize_digits(episodeDigits);
                if (!episodeTitle.empty()) {
                    episodeResult += L" - ";
                    episodeResult += episodeTitle;
                }
                if (episodeResult.size() + 1 > static_cast<size_t>(outputCount)) return 0;
                std::wmemcpy(output, episodeResult.c_str(), episodeResult.size() + 1);
                return 1;
            }
        }
    }

    // Case 2: number at the beginning, optionally followed by spaces and
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
        // Case 3: number at the end, even when attached to the title.
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
