#pragma once
#include "RefCounted.h"
#include "RuntimeError.h"
#include "lib/libgit2/include/git2.h"

namespace Git {

using Id = git_oid;

using Repo = RefCounted<git_repository*, git_repository_free>;
using Tree = RefCounted<git_tree*, git_tree_free>;
using RevWalk = RefCounted<git_revwalk*, git_revwalk_free>;
using Index = RefCounted<git_index*, git_index_free>;
using Buf = RefCounted<git_buf, git_buf_dispose>;

struct Commit : RefCounted<git_commit*, git_commit_free> {
    using RefCounted::RefCounted;
    const Id* id() const { return git_commit_id(*(*this)); }
    bool operator==(const Commit& x) const { return git_oid_cmp(id(), x.id())==0; }
    bool operator!=(const Commit& x) const { return !(*this==x); }
    bool operator<(const Commit& x) const { return git_oid_cmp(id(), x.id())<0; }
//    operator git_commit*() { return *(*this); }
//    operator const git_commit*() const { return *(*this); }
    
    static Commit Lookup(Repo repo, const Id& commitId) {
        git_commit* x = nullptr;
        int ir = git_commit_lookup(&x, *repo, &commitId);
        if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
        return x;
    }

    static Commit Lookup(Repo repo, std::string_view commitIdStr) {
        Id commitId;
        int ir = git_oid_fromstr(&commitId, commitIdStr.data());
        if (ir) throw RuntimeError("git_oid_fromstr failed: %s", git_error_last()->message);
        return Lookup(repo, commitId);
    }
    
    Tree tree() const {
        git_tree* t = nullptr;
        int ir = git_commit_tree(&t, *this);
        if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
        assert(!ir);
        return t;
    }
    
    Commit parent(unsigned int n=0) const {
        git_commit* c = nullptr;
        int ir = git_commit_parent(&c, *this, n);
        if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
        return c;
    }
};

struct Object : RefCounted<git_object*, git_object_free> {
    using RefCounted::RefCounted;
    const Id* id() const { return git_object_id(*(*this)); }
    bool operator==(const Object& x) const { return git_oid_cmp(id(), x.id())==0; }
    bool operator!=(const Object& x) const { return !(*this==x); }
    bool operator<(const Object& x) const { return git_oid_cmp(id(), x.id())<0; }
//    operator const git_object*() const { return *(*this); }
};

struct Reference : RefCounted<git_reference*, git_reference_free> {
    using RefCounted::RefCounted;
    bool operator==(const Reference& x) const { return git_reference_cmp(*(*this), *x)==0; }
    bool operator!=(const Reference& x) const { return !(*this==x); }
    bool operator<(const Reference& x) const { return git_reference_cmp(*(*this), *x)<0; }
//    operator git_reference*() { return *(*this); }
//    operator const git_reference*() const { return *(*this); }
};

struct Branch : Reference {
    using Reference::Reference;
    
    static Branch Lookup(Repo repo, std::string_view name) {
        git_reference* x;
        int ir = git_branch_lookup(&x, *repo, name.data(), GIT_BRANCH_LOCAL);
        if (ir) throw RuntimeError("git_branch_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    static Branch Create(Repo repo, std::string_view name, Commit commit, bool force=false) {
        git_reference* x = nullptr;
        int ir = git_branch_create(&x, *repo, name.data(), commit, force);
        if (ir) throw RuntimeError("git_branch_create failed: %s", git_error_last()->message);
        return x;
    }
    
    const char* name() const {
        const char* x = nullptr;
        int ir = git_branch_name(&x, *this);
        if (ir) throw RuntimeError("git_branch_name failed: %s", git_error_last()->message);
        return x;
    }
    
    Branch upstream() const {
        git_reference* x;
        int ir = git_branch_upstream(&x, *this);
        if (ir) throw RuntimeError("git_branch_upstream failed: %s", git_error_last()->message);
        return x;
    }
    
    void upstreamSet(Branch upstream) {
        int ir = git_branch_set_upstream(*this, upstream.name());
        if (ir) throw RuntimeError("git_branch_upstream_name failed: %s", git_error_last()->message);
    }
    
    Commit commit() const {
        git_commit* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *this, GIT_OBJECT_COMMIT);
        assert(!ir);
        return x;
    }
};

inline std::string Str(const git_oid& oid) {
    char str[16];
    sprintf(str, "%02x%02x%02x%02x", oid.id[0], oid.id[1], oid.id[2], oid.id[3]);
    str[7] = 0;
    return str;
}

inline std::string Str(git_time_t t) {
    const std::time_t tmp = t;
    std::tm tm;
    localtime_r(&tmp, &tm);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%a %b %d %R");
    return ss.str();
}

inline Repo RepoOpen(std::string_view path) {
    git_repository* x = nullptr;
    int ir = git_repository_open(&x, path.data());
    if (ir) throw RuntimeError("git_repository_open failed: %s", git_error_last()->message);
    return x;
}

inline RevWalk RevWalkCreate(Repo repo) {
    git_revwalk* x = nullptr;
    int ir = git_revwalk_new(&x, *repo);
    if (ir) throw RuntimeError("git_revwalk_new failed: %s", git_error_last()->message);
    return x;
}

inline std::string RepoCurrentBranchNameGet(Repo repo) {
    Reference head;
    {
        git_reference* x = nullptr;
        int ir = git_repository_head(&x, *repo);
        if (ir) return {};
        head = x;
    }
    return git_reference_shorthand(*head);
}

inline Index TreesMerge(Repo repo, Tree ancestorTree, Tree dstTree, Tree srcTree) {
    git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
    git_index* i = nullptr;
    int ir = git_merge_trees(&i, *repo, *ancestorTree, *dstTree, *srcTree, &mergeOpts);
    if (ir) throw RuntimeError("git_merge_trees failed: %s", git_error_last()->message);
    return i;
}

inline Tree IndexWrite(Repo repo, Index index) {
    git_oid treeId;
    int ir = git_index_write_tree_to(&treeId, *index, *repo);
    if (ir) throw RuntimeError("git_index_write_tree_to failed: %s", git_error_last()->message);
    
    git_tree* newTree = nullptr;
    ir = git_tree_lookup(&newTree, *repo, &treeId);
    if (ir) throw RuntimeError("git_tree_lookup failed: %s", git_error_last()->message);
    return newTree;
}

// Creates a new child commit of `parent` with the tree `tree`, using the metadata from `metadata`
inline Commit CommitAttach(Repo repo, Commit parent, Tree tree, Commit metadata) {
    git_oid commitId;
    const git_commit* parents[] = {*parent};
    int ir = git_commit_create(
        &commitId,
        *repo,
        nullptr,
        git_commit_author(*metadata),
        git_commit_committer(*metadata),
        git_commit_message_encoding(*metadata),
        git_commit_message(*metadata),
        *tree,
        std::size(parents),
        parents
    );
    if (ir) throw RuntimeError("git_commit_create failed: %s", git_error_last()->message);
    return Commit::Lookup(repo, commitId);
}

inline Commit CherryPick(Repo repo, Commit dst, Commit src) {
    const git_oid* dstId = git_commit_id(*dst);
    const git_oid* srcId = git_commit_id(*src);
//    const git_oid* dstTargetId = git_reference_target(basket);
//    if (!dstTargetId) throw RuntimeError("git_reference_target returned null failed: %s", git_error_last()->message);
    
    Tree srcTree = src.tree();
    Tree dstTree = dst.tree();
    
    git_oid baseId;
    int ir = git_merge_base(&baseId, *repo, srcId, dstId);
    if (ir) throw RuntimeError("git_merge_base failed: %s", git_error_last()->message);
    
    Tree ancestorTree = src.parent().tree();
    Index mergedTreesIndex = TreesMerge(repo, ancestorTree, dstTree, srcTree);
    Tree newTree = IndexWrite(repo, mergedTreesIndex);
    Commit newCommit = CommitAttach(repo, dst, newTree, src);
//    printf("New commit: %s\n", git_oid_tostr_s(git_commit_id(*newCommit)));
    return newCommit;
}

} // namespace Git
