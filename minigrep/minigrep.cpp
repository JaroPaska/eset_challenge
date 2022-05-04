#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

namespace minigrep {

constexpr int border_size = 3; /**< The number of characters to show for the prefix and suffix. */
constexpr int chunk_size = 1000000; /**< The maximum amount of characters that a single async task can process. */

/**
 * Data structure that represents a half-open interval.
 */
struct Range {
    int begin; /**< Beginning of range (inclusive). */
    int end; /**< End of range (exclusive). */

    /**
     * Clamps this range to fit inside the half-open interval [min, max).
     * @param min The minimum of the clamped range.
     * @param max The maximum of the clamped range.
     * @return The clamped range.
     */
    [[nodiscard]] constexpr Range clamp(int min, int max) const {
        return Range{std::max(begin, min), std::min(end, max)};
    }

    /**
     * Extends this range in both directions.
     * @param amount The amount to extend the range by.
     * @return The extended range.
     */
    [[nodiscard]] constexpr Range extend(int amount) const {
        return Range{begin - amount, end + amount};
    }

    /**
     * Size of this range.
     * @return The size of this range.
     */
    [[nodiscard]] constexpr int size() const { return end - begin; }

    /**
     * Splits this chunk into two smaller chunks if it is large enough.
     * @return The two smaller chunks, or std::nullopt the chunk is small enough.
     */
    [[nodiscard]] constexpr std::optional<std::pair<Range, Range>> split() const {
        if (size() <= chunk_size)
            return std::nullopt;
        int mid = begin + chunk_size;
        return std::make_pair(Range{begin, mid}, Range{mid, end});
    }
};

/**
 * Checks two ranges for equality.
 * @param l left hand side
 * @param r right hand side
 * @return Whether the ranges are equal.
 */
[[nodiscard]] constexpr bool operator==(const Range &l, const Range &r) { return l.begin == r.begin && l.end == r.end; }

/**
 * A file on disk.
 */
struct File {
    std::string path; /**< The path to the file. */
    int size; /**< The size of the file. */

    /**
     * Constructs a file using the specified path to determine the size.
     * @param path Path of the file.
     */
    File(const std::string &path) : path(path) {
        std::ifstream is(path);
        is.seekg(0, is.end);
        size = is.tellg();
    }
};

/**
 * A segment of a file that is to be searched.
 */
struct FileChunk {
    File file; /**< The file to be searched. */
    Range search; /**< The range to be searched. */
    Range read; /**< The range that has to be read (this may be larger to properly output the prefix/suffix). */
    std::string contents; /**< The contents corresponding to the read range. */

    /**
     * Constructs a chunk.
     * @param file The file to be searched.
     * @param search The range to be searched.
     */
    constexpr FileChunk(const File &file, const Range &search)
        : file(file), search(search), read(search.extend(border_size).clamp(0, file.size)) {}

    /**
     * Reads the corresponding segment of the file into @see #contents.
     */
    void fetch_contents() {
        std::ifstream is(file.path);
        is.seekg(read.begin);
        std::vector<char> buffer(read.size());
        is.read(buffer.data(), buffer.size());
        contents = std::string(buffer.begin(), buffer.end());
    }
};

/**
 * Data relevant to an occurrence of the searched string.
 */
struct Match {
    std::string path; /**< The path to the file in which the match occurred. */
    int position; /**< The offset of the match from the start of the file. */
    std::string prefix; /**< The characters before the match. */
    std::string suffix; /**< The characters after the match. */
};

/**
 * Outputs match to stream.
 * @param os The stream to output to.
 * @param m The match to output.
 * @return The stream.
 */
std::ostream &operator<<(std::ostream &os, const Match &m) {
    return os << m.path << "(" << m.position << "):" << m.prefix << "..." << m.suffix;
}

/**
 * The prefix of a match.
 * @param contents The text in which the match was found.
 * @param index The index of the start of the match.
 * @return The characters before the match.
 */
[[nodiscard]] constexpr std::string prefix(const std::string &contents, int index) {
    int offset = std::max(index - border_size, 0);
    int count = index - offset;
    return contents.substr(offset, count);
}

/**
 * The suffix of a match.
 * @param contents The text in which the match was found.
 * @param index The first index past the end of the match.
 * @return The characters after the match.
 */
[[nodiscard]] constexpr std::string suffix(const std::string &contents, int index) {
    return contents.substr(index, border_size);
}

/**
 * Formats string to eliminate whitespace.
 * @param string String to be formatted.
 * @return A string with whitespace replaced.
 */
[[nodiscard]] constexpr std::string transform(const std::string &string) {
    std::string result;
    for (const auto &c : string)
        switch (c) {
        case '\n':
            result += "\\n";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result.push_back(c);
            break;
        }
    return result;
}

/**
 * Finds all the occurrences of a string in the portion of a file.
 * @param chunk The portion of a file to be searched.
 * @param string The string to search for.
 * @return All the matches.
 */
[[nodiscard]] std::vector<Match> matches(const FileChunk &chunk, const std::string &string) {
    std::vector<Match> result;
    auto to_index = [&](int pos) { return pos - chunk.read.begin; };
    for (std::size_t pos = chunk.contents.find(string, to_index(chunk.search.begin));
         pos != std::string::npos && pos < to_index(chunk.search.end); pos = chunk.contents.find(string, pos + 1)) {
        result.push_back(Match{chunk.file.path, static_cast<int>(pos), transform(prefix(chunk.contents, to_index(pos))),
                               transform(suffix(chunk.contents, to_index(pos + string.size())))});
    }
    return result;
}

/**
 * Computes which files are to be searched.
 * @param path Path to the directory or file to be searched.
 * @return All files to be searched, or std::nullopt if the given path is not valid.
 */
[[nodiscard]] std::optional<std::vector<File>> files(const std::string &path) {
    if (std::filesystem::is_regular_file(path))
        return std::vector<File>{File(path)};

    if (!std::filesystem::is_directory(path))
        return std::nullopt;

    std::vector<File> result;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
        if (entry.is_regular_file())
            result.emplace_back(entry.path().string());
    return result;
}

/**
 * Splits the file into chunks.
 * @param file File to be split.
 * @return Chunks that correspond to the file as a whole.
 */
[[nodiscard]] std::vector<FileChunk> chunks(const File &file) {
    std::vector<FileChunk> result{FileChunk(file, Range{0, file.size})};
    std::optional<std::pair<Range, Range>> split_chunks;
    while ((split_chunks = result.back().search.split())) {
        result.back() = FileChunk(file, split_chunks.value().first);
        result.emplace_back(file, split_chunks.value().second);
    }
    return result;
}

/**
 * Searches the chunk for matches and prints them to stdout.
 * @param chunk Chunk to be searched.
 * @param string String to search for.
 */
void search(FileChunk &chunk, const std::string &string) {
    chunk.fetch_contents();
    auto all_matches = matches(chunk, string);
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto &match : all_matches)
        std::cout << match << "\n";
}

namespace test {

static_assert(Range{1, 3}.clamp(0, 2) == Range{1, 2});
static_assert(Range{1, 3}.extend(2) == Range{-1, 5});
static_assert(Range{1, 3}.size() == 2);
static_assert(Range{0, chunk_size + 100}.split().value() ==
              std::make_pair(Range{0, chunk_size}, Range{chunk_size, chunk_size + 100}));
static_assert(prefix("abcd", 0) == "");
static_assert(prefix("abcd", 2) == "ab");
static_assert(suffix("abcd", 0) == "abc");
static_assert(suffix("abcd", 2) == "cd");
static_assert(transform("abcd") == "abcd");
static_assert(transform("\t\n") == "\\t\\n");

} // namespace test

} // namespace minigrep

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: minigrep <directory|file> <search string>\n";
        return EXIT_FAILURE;
    }

    const auto files = minigrep::files(argv[1]);
    if (!files) {
        std::cerr << "Argument 1 must be a directory or a file\n";
        return EXIT_FAILURE;
    }

    std::vector<minigrep::FileChunk> all_chunks;
    for (const auto &file : files.value()) {
        const auto chunks = minigrep::chunks(file);
        all_chunks.insert(all_chunks.end(), chunks.begin(), chunks.end());
    }

    std::vector<std::future<void>> futures;
    for (auto &chunk : all_chunks)
        futures.push_back(std::async(std::launch::async, minigrep::search, std::ref(chunk), argv[2]));
}