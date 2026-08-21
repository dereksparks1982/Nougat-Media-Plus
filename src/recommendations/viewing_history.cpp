#include "viewing_history.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace reddmedia {
namespace {

struct sqlite3;
struct sqlite3_stmt;

constexpr int kSqliteOk = 0;
constexpr int kSqliteRow = 100;
constexpr int kSqliteDone = 101;
constexpr int kSqliteOpenReadWrite = 0x00000002;
constexpr int kSqliteOpenCreate = 0x00000004;
constexpr int kSqliteOpenFullMutex = 0x00010000;

using OpenV2 = int (*)(const char*, sqlite3**, int, const char*);
using CloseV2 = int (*)(sqlite3*);
using Exec = int (*)(sqlite3*, const char*, int (*)(void*, int, char**, char**), void*, char**);
using Free = void (*)(void*);
using ErrMsg = const char* (*)(sqlite3*);
using PrepareV2 = int (*)(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
using BindText = int (*)(sqlite3_stmt*, int, const char*, int, void (*)(void*));
using BindInt = int (*)(sqlite3_stmt*, int, int);
using Step = int (*)(sqlite3_stmt*);
using Finalize = int (*)(sqlite3_stmt*);
using ColumnText = const unsigned char* (*)(sqlite3_stmt*, int);
using ColumnInt = int (*)(sqlite3_stmt*, int);
using ColumnInt64 = long long (*)(sqlite3_stmt*, int);

struct SqliteApi {
    OpenV2 open_v2 = nullptr;
    CloseV2 close_v2 = nullptr;
    Exec exec = nullptr;
    Free free_memory = nullptr;
    ErrMsg errmsg = nullptr;
    PrepareV2 prepare_v2 = nullptr;
    BindText bind_text = nullptr;
    BindInt bind_int = nullptr;
    Step step = nullptr;
    Finalize finalize = nullptr;
    ColumnText column_text = nullptr;
    ColumnInt column_int = nullptr;
    ColumnInt64 column_int64 = nullptr;
};

SqliteApi& api() {
    static SqliteApi value;
    return value;
}

template <typename Function>
bool load_symbol(void* library, const char* name, Function& function) {
    function = reinterpret_cast<Function>(dlsym(library, name));
    return function != nullptr;
}

bool load_api(void* library) {
    SqliteApi& value = api();
    return load_symbol(library, "sqlite3_open_v2", value.open_v2) &&
           load_symbol(library, "sqlite3_close_v2", value.close_v2) &&
           load_symbol(library, "sqlite3_exec", value.exec) &&
           load_symbol(library, "sqlite3_free", value.free_memory) &&
           load_symbol(library, "sqlite3_errmsg", value.errmsg) &&
           load_symbol(library, "sqlite3_prepare_v2", value.prepare_v2) &&
           load_symbol(library, "sqlite3_bind_text", value.bind_text) &&
           load_symbol(library, "sqlite3_bind_int", value.bind_int) &&
           load_symbol(library, "sqlite3_step", value.step) &&
           load_symbol(library, "sqlite3_finalize", value.finalize) &&
           load_symbol(library, "sqlite3_column_text", value.column_text) &&
           load_symbol(library, "sqlite3_column_int", value.column_int) &&
           load_symbol(library, "sqlite3_column_int64", value.column_int64);
}

std::string parent_directory(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return ".";
    return slash == 0U ? "/" : path.substr(0, slash);
}

bool ensure_directory(const std::string& path) {
    if (path.empty()) return false;
    std::string current = path.front() == '/' ? "/" : "";
    std::size_t start = path.front() == '/' ? 1U : 0U;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::string part = path.substr(
            start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty()) {
            if (current.size() > 1U && current.back() != '/') current.push_back('/');
            current += part;
            if (mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) return false;
        }
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    return true;
}

std::string joined_genres(const std::vector<std::string>& genres) {
    std::ostringstream value;
    for (std::size_t index = 0; index < genres.size(); ++index) {
        if (index > 0U) value << '|';
        value << genres[index];
    }
    return value.str();
}

std::vector<std::string> split_genres(const std::string& text) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t separator = text.find('|', start);
        const std::string value = text.substr(
            start, separator == std::string::npos ? std::string::npos : separator - start);
        if (!value.empty()) result.push_back(value);
        if (separator == std::string::npos) break;
        start = separator + 1U;
    }
    return result;
}

const char* column_string(sqlite3_stmt* statement, int column) {
    const unsigned char* value = api().column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : "";
}

} // namespace

ViewingHistory::ViewingHistory(std::string database_path)
    : database_path_(std::move(database_path)) {
    if (!database_path_.empty()) return;
    const char* override_path = std::getenv("REDDMEDIA_HISTORY_DATABASE");
    if (override_path && *override_path) {
        database_path_ = override_path;
        return;
    }
    const char* home = std::getenv("HOME");
    database_path_ = std::string(home ? home : ".") +
        "/.local/share/reddmedia/history/viewing_history.sqlite3";
}

ViewingHistory::~ViewingHistory() {
    close_database();
}

bool ViewingHistory::open_database(std::string& error) {
    if (database_) return true;
    if (!ensure_directory(parent_directory(database_path_))) {
        error = "ReddMedia could not create its private viewing-history folder.";
        return false;
    }
    const char* libraries[] = {"libsqlite3.so.0", "libsqlite3.so", nullptr};
    for (int index = 0; libraries[index] && !sqlite_library_; ++index) {
        sqlite_library_ = dlopen(libraries[index], RTLD_NOW | RTLD_LOCAL);
    }
    if (!sqlite_library_ || !load_api(sqlite_library_)) {
        error = "SQLite is unavailable; viewing history cannot be stored.";
        close_database();
        return false;
    }
    sqlite3* database = nullptr;
    if (api().open_v2(database_path_.c_str(), &database,
                      kSqliteOpenReadWrite | kSqliteOpenCreate | kSqliteOpenFullMutex,
                      nullptr) != kSqliteOk || !database) {
        error = "ReddMedia could not open its private viewing-history database.";
        if (database) api().close_v2(database);
        close_database();
        return false;
    }
    database_ = database;
    chmod(database_path_.c_str(), 0600);
    const char* schema =
        "CREATE TABLE IF NOT EXISTS viewing_history("
        "item_id TEXT NOT NULL, media_type INTEGER NOT NULL, title TEXT NOT NULL,"
        "overview TEXT NOT NULL DEFAULT '', genres TEXT NOT NULL DEFAULT '',"
        "local_path TEXT NOT NULL DEFAULT '', tmdb_id TEXT NOT NULL DEFAULT '',"
        "year INTEGER NOT NULL DEFAULT 0, last_watched INTEGER NOT NULL,"
        "play_count INTEGER NOT NULL DEFAULT 1,"
        "completed INTEGER NOT NULL DEFAULT 0,"
        "PRIMARY KEY(item_id, media_type));"
        "CREATE INDEX IF NOT EXISTS viewing_history_recent "
        "ON viewing_history(media_type, last_watched DESC);";
    char* message = nullptr;
    if (api().exec(database, schema, nullptr, nullptr, &message) != kSqliteOk) {
        error = message ? message : "ReddMedia could not initialize viewing history.";
        if (message) api().free_memory(message);
        close_database();
        return false;
    }
    message = nullptr;
    if (api().exec(database, "ALTER TABLE viewing_history ADD COLUMN completed INTEGER NOT NULL DEFAULT 0;",
                   nullptr, nullptr, &message) != kSqliteOk) {
        const std::string migration_message = message ? message : "";
        if (message) api().free_memory(message);
        if (migration_message.find("duplicate column") == std::string::npos) {
            error = migration_message.empty() ? "ReddMedia could not upgrade viewing history." : migration_message;
            close_database();
            return false;
        }
    }
    return true;
}

void ViewingHistory::close_database() {
    if (database_ && api().close_v2) api().close_v2(static_cast<sqlite3*>(database_));
    database_ = nullptr;
    if (sqlite_library_) dlclose(sqlite_library_);
    sqlite_library_ = nullptr;
}

bool ViewingHistory::record_started(const MediaDescriptor& item, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (item.id.empty() || item.title.empty()) {
        error = "A playable library item is required before recording history.";
        return false;
    }
    if (!open_database(error)) return false;
    const char* sql =
        "INSERT INTO viewing_history(item_id,media_type,title,overview,genres,local_path,"
        "tmdb_id,year,last_watched,play_count,completed) VALUES(?,?,?,?,?,?,?,?,strftime('%s','now'),1,0) "
        "ON CONFLICT(item_id,media_type) DO UPDATE SET title=excluded.title,"
        "overview=excluded.overview,genres=excluded.genres,local_path=excluded.local_path,"
        "tmdb_id=excluded.tmdb_id,year=excluded.year,last_watched=excluded.last_watched,"
        "play_count=viewing_history.play_count+1,completed=0";
    sqlite3_stmt* statement = nullptr;
    sqlite3* database = static_cast<sqlite3*>(database_);
    if (api().prepare_v2(database, sql, -1, &statement, nullptr) != kSqliteOk) {
        error = api().errmsg(database);
        return false;
    }
    const std::string genres = joined_genres(item.genres);
    const auto bind = [statement](int index, const std::string& value) {
        return api().bind_text(statement, index, value.c_str(), -1,
                               reinterpret_cast<void (*)(void*)>(-1));
    };
    bool ok = bind(1, item.id) == kSqliteOk &&
              api().bind_int(statement, 2,
                  item.media_type == RecommendationMediaType::Movie ? 0 : 1) == kSqliteOk &&
              bind(3, item.title) == kSqliteOk && bind(4, item.overview) == kSqliteOk &&
              bind(5, genres) == kSqliteOk && bind(6, item.local_path) == kSqliteOk &&
              bind(7, item.tmdb_id) == kSqliteOk &&
              api().bind_int(statement, 8, item.year) == kSqliteOk &&
              api().step(statement) == kSqliteDone;
    if (!ok) error = api().errmsg(database);
    api().finalize(statement);
    return ok;
}

bool ViewingHistory::record_completed(const MediaDescriptor& item, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (item.id.empty() || item.title.empty()) {
        error = "A playable library item is required before marking history complete.";
        return false;
    }
    if (!open_database(error)) return false;
    const char* sql =
        "INSERT INTO viewing_history(item_id,media_type,title,overview,genres,local_path,"
        "tmdb_id,year,last_watched,play_count,completed) VALUES(?,?,?,?,?,?,?,?,strftime('%s','now'),1,1) "
        "ON CONFLICT(item_id,media_type) DO UPDATE SET title=excluded.title,"
        "overview=excluded.overview,genres=excluded.genres,local_path=excluded.local_path,"
        "tmdb_id=excluded.tmdb_id,year=excluded.year,last_watched=excluded.last_watched,completed=1";
    sqlite3_stmt* statement = nullptr;
    sqlite3* database = static_cast<sqlite3*>(database_);
    if (api().prepare_v2(database, sql, -1, &statement, nullptr) != kSqliteOk) {
        error = api().errmsg(database);
        return false;
    }
    const std::string genres = joined_genres(item.genres);
    const auto bind = [statement](int index, const std::string& value) {
        return api().bind_text(statement, index, value.c_str(), -1,
                               reinterpret_cast<void (*)(void*)>(-1));
    };
    const bool ok = bind(1, item.id) == kSqliteOk &&
        api().bind_int(statement, 2, item.media_type == RecommendationMediaType::Movie ? 0 : 1) == kSqliteOk &&
        bind(3, item.title) == kSqliteOk && bind(4, item.overview) == kSqliteOk &&
        bind(5, genres) == kSqliteOk && bind(6, item.local_path) == kSqliteOk &&
        bind(7, item.tmdb_id) == kSqliteOk && api().bind_int(statement, 8, item.year) == kSqliteOk &&
        api().step(statement) == kSqliteDone;
    if (!ok) error = api().errmsg(database);
    api().finalize(statement);
    return ok;
}

bool ViewingHistory::recent(RecommendationMediaType type,
                            std::vector<ViewingRecord>& records,
                            std::string& error,
                            int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!open_database(error)) return false;
    const char* sql =
        "SELECT item_id,title,overview,genres,local_path,tmdb_id,year,last_watched,play_count,completed "
        "FROM viewing_history WHERE media_type=? ORDER BY last_watched DESC LIMIT ?";
    sqlite3_stmt* statement = nullptr;
    sqlite3* database = static_cast<sqlite3*>(database_);
    if (api().prepare_v2(database, sql, -1, &statement, nullptr) != kSqliteOk) {
        error = api().errmsg(database);
        return false;
    }
    api().bind_int(statement, 1, type == RecommendationMediaType::Movie ? 0 : 1);
    api().bind_int(statement, 2, limit > 0 ? limit : 100);
    std::vector<ViewingRecord> loaded;
    int status = kSqliteRow;
    while ((status = api().step(statement)) == kSqliteRow) {
        ViewingRecord record;
        record.item.id = column_string(statement, 0);
        record.item.title = column_string(statement, 1);
        record.item.overview = column_string(statement, 2);
        record.item.genres = split_genres(column_string(statement, 3));
        record.item.local_path = column_string(statement, 4);
        record.item.tmdb_id = column_string(statement, 5);
        record.item.year = api().column_int(statement, 6);
        record.item.media_type = type;
        record.last_watched = api().column_int64(statement, 7);
        record.play_count = api().column_int(statement, 8);
        record.completed = api().column_int(statement, 9) != 0;
        loaded.push_back(std::move(record));
    }
    const bool ok = status == kSqliteDone;
    if (!ok) error = api().errmsg(database);
    api().finalize(statement);
    if (ok) records = std::move(loaded);
    return ok;
}

} // namespace reddmedia
