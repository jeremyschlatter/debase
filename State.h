#pragma once
#include <set>
#include <fstream>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/nlohmann/json.h"
#include "History.h"
#include "Git.h"

template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::deque<T>& out);
template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::set<T>& out);
template <typename T_Key, typename T_Val>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::map<T_Key,T_Val>& out);

struct RefState {
    Git::Commit head;
    std::set<Git::Commit> selection;
    std::set<Git::Commit> selectionPrev;
    
    bool operator==(const RefState& x) const {
        if (head != x.head) return false;
        if (selection != x.selection) return false;
        if (selectionPrev != x.selectionPrev) return false;
        return true;
    }
    
    bool operator!=(const RefState& x) const {
        return !(*this==x);
    }
};

using RefHistory = T_History<RefState>;

// MARK: - Ref Serialization
inline void from_json(const nlohmann::json& j, Git::Repo repo, Git::Ref& out) {
    std::string name;
    j.get_to(name);
    out = repo.refLookup(name);
}

// MARK: - Commit Serialization
inline void from_json(const nlohmann::json& j, Git::Repo repo, Git::Commit& out) {
    std::string id;
    j.get_to(id);
    out = repo.commitLookup(id);
}

// MARK: - RefState Serialization
inline void to_json(nlohmann::json& j, const RefState& out) {
    j = {
        {"head", out.head},
        {"selection", out.selection},
        {"selectionPrev", out.selectionPrev},
    };
}

inline void from_json(const nlohmann::json& j, Git::Repo repo, RefState& out) {
    ::from_json(j.at("head"), repo, out.head);
    ::from_json(j.at("selection"), repo, out.selection);
    ::from_json(j.at("selectionPrev"), repo, out.selectionPrev);
}

// MARK: - RefHistory Serialization
inline void to_json(nlohmann::json& j, const RefHistory& out) {
    j = {
        {"prev", out._prev},
        {"next", out._next},
        {"current", out._current},
    };
}

inline void from_json(const nlohmann::json& j, Git::Repo repo, RefHistory& out) {
    ::from_json(j.at("prev"), repo, out._prev);
    ::from_json(j.at("next"), repo, out._next);
    ::from_json(j.at("current"), repo, out._current);
}

template <typename T>
inline void from_json_vector(const nlohmann::json& j, Git::Repo repo, T& out) {
    using namespace nlohmann;
    using T_Elm = typename T::value_type;
    std::vector<json> elms;
    j.get_to(elms);
    for (const json& j : elms) {
        try {
            T_Elm elm;
            ::from_json(j, repo, elm);
            out.insert(out.end(), elm);
        } catch (...) {}
    }
}

template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::deque<T>& out) {
    from_json_vector(j, repo, out);
}

template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::set<T>& out) {
    from_json_vector(j, repo, out);
}

template <typename T_Key, typename T_Val>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::map<T_Key,T_Val>& out) {
    using namespace nlohmann;
    std::map<json,json> elms;
    j.get_to(elms);
    for (const auto& i : elms) {
        try {
            T_Key key;
            T_Val val;
            ::from_json(i.first, repo, key);
            ::from_json(i.second, repo, val);
            out[key] = val;
        } catch (...) {}
    }
}

class RepoState {
private:
    using _Path = std::filesystem::path;
    using _Json = nlohmann::json;
    _Path _dir;
    Git::Repo _repo;
    
    std::map<Git::Ref,RefHistory> _refHistorysPrev;
    std::map<Git::Ref,RefHistory> _refHistorys;
    
public:
    RepoState() {}
    RepoState(_Path dir, Git::Repo repo, const std::set<Git::Ref>& refs) : _dir(dir), _repo(repo) {
        std::map<Git::Ref,std::map<Git::Commit,RefHistory>> refHistorys;
        try {
            _Path fpath = _dir / "RefHistory";
            std::ifstream f(fpath);
            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            nlohmann::json j;
            f >> j;
            
            ::from_json(j, _repo, refHistorys);
        
        // Ignore deserialization errors (eg file not existing)
        } catch (...) {}
        
        // Populate _refHistory by looking up the RefHistory for each ref,
        // by looking at its current commit.
        // We also delete the matching commit->RevHistory entry after copying it
        // into `_refHistory`, because during this session we may modify the
        // ref, and so that entry will be stale (and if we didn't prune them,
        // the state file would grow indefinitely.)
        for (const Git::Ref& ref : refs) {
            Git::Commit refCommit = ref.commit();
            std::map<Git::Commit,RefHistory>& refHistoryMap = refHistorys[ref];
            RefHistory& refHistory = _refHistorys[ref];
            if (auto find=refHistoryMap.find(refCommit); find!=refHistoryMap.end()) {
                refHistory = find->second;
            } else {
                refHistory.clear();
                refHistory.set(RefState{
                    .head = refCommit,
                });
            }
            
            _refHistorysPrev[ref] = refHistory;
        }
    }
    
    void write() {
        _Path fpath = _dir / "RefHistory";
        
        // refHistory: intentional loose '_Json' typing because we want to maintain all
        // of its data, even if, eg we fail to construct a Ref because it doesn't exist.
        // If we had strong typing, and we couldn't construct the Ref, we'd throw out the
        // entry and the data would be lost when we write the json file below.
        std::map<_Json,std::map<_Json,_Json>> refHistorysJson;
        try {
            std::ifstream f(fpath);
            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            nlohmann::json j;
            f >> j;
            j.get_to(refHistorysJson);
        
        // Ignore deserialization errors (eg file not existing)
        } catch (...) {}
        
        for (const auto& i : _refHistorysPrev) {
            Git::Ref ref = _repo.refReload(i.first);
            const RefHistory& refHistory = _refHistorys.at(ref);
            const RefHistory& refHistoryPrev = i.second;
            Git::Commit commit = ref.commit();
            // Ignore entries that didn't change
            if (refHistory == refHistoryPrev) continue;
            // The ref was modified, so erase the old Commit->RefHistory entry, and insert the new one.
            std::map<_Json,_Json>& refHistoryJson = refHistorysJson[ref];
            refHistoryJson.erase(refHistoryPrev.get().head);
            refHistoryJson[commit] = _refHistorys.at(ref);
        }
        
        std::filesystem::create_directories(_dir);
        std::ofstream f(fpath);
        f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        f << std::setw(4) << refHistorysJson;
    }
    
    RefHistory& refHistory(Git::Ref ref) {
        return _refHistorys.at(ref);
    }
    
    Git::Repo repo() const {
        return _repo;
    }
};

class State {
private:
    using _Path = std::filesystem::path;
    static constexpr uint32_t _Version = 0;
    _Path _dir;
    
//        static constexpr uint32_t _Version = 0;
//        static uint32_t _StoredStateVersion() {
//            try {
//                _Path fpath = RepoStateDir(_repo) / "RefHistory";
//                std::ifstream f(fpath);
//                f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
//                nlohmann::json j;
//                f >> j;
//                
//                ::from_json(j, _repo, refHistorys);
//            
//            // Ignore deserialization errors (eg file not existing)
//            } catch (...) {}
//        }
    
    static _Path _RepoStateDirPath(_Path dir, Git::Repo repo) {
        std::string name = std::filesystem::canonical(repo.path());
        std::replace(name.begin(), name.end(), '/', '-'); // Replace / with ~
        return dir / "Repo" / name;
    }
    
    static _Path _VersionFilePath(_Path dir) {
        return dir / "Version";
    }
    
    static std::optional<uint32_t> _VersionRead(_Path dir) {
        _Path p = _VersionFilePath(dir);
        if (!std::filesystem::exists(p)) return std::nullopt;
        
        std::ifstream f(_VersionFilePath(dir));
        f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        nlohmann::json j;
        f >> j;
        uint32_t version = 0;
        version = j;
        return version;
    }
    
    static void _VersionWrite(_Path dir, uint32_t version) {
        std::filesystem::create_directories(dir);
        std::ofstream f(_VersionFilePath(dir));
        f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        nlohmann::json j = version;
        f << j;
    }
    
public:
    State(const _Path& dir) : _dir(dir) {
        std::optional<uint32_t> version = _VersionRead(_dir);
        if (version && *version!=_Version)
            throw Toastbox::RuntimeError(
            "version of debase state on disk (v%ju) is newer than this version of debase (v%ju);\n"
            "please use a newer version of debase, or remove:\n"
            "  %s", (uintmax_t)*version, (uintmax_t)_Version, _dir.c_str());
        
        // Delete the state directory to ensure we start with a clean slate
        if (!version) {
            // Sanity check to ensure we never delete anything that doesn't contain the string 'debase'.
            // This is mainly a safeguard against situations where we might accidentally pass a truncated
            // path to this function, like "/" or /Users/dave/Library.
            assert(_dir.filename().string().find("debase") != std::string::npos);
            std::filesystem::remove_all(_dir);
        }
        
        _VersionWrite(_dir, _Version);
    }
    
    RepoState repoStateCreate(Git::Repo repo, const std::set<Git::Ref>& refs) {
        return RepoState(_RepoStateDirPath(_dir, repo), repo, refs);
    }
};
