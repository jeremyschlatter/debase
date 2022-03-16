#pragma once
#include "RefCounted.h"
#include "lib/Toastbox/RuntimeError.h"
#include "lib/libgit2/include/git2.h"

namespace Git {
using namespace Toastbox;

// Macros to handle checking for nullptr before comparing underlying objects
#define _Equal(a, b, cmp) ({                \
    bool r = ((bool)(a) == (bool)(b));      \
    if ((bool)(a) && (bool)(b)) r = (cmp);  \
    r;                                      \
})

#define _Less(a, b, cmp) ({                 \
    bool r = ((bool)(a) < (bool)(b));       \
    if ((bool)(a) && (bool)(b)) r = (cmp);  \
    r;                                      \
})

using Id = git_oid;
using Tree = RefCounted<git_tree*, git_tree_free>;
using Index = RefCounted<git_index*, git_index_free>;

struct Signature : RefCounted<git_signature*, git_signature_free> {
    using RefCounted::RefCounted;
    static Signature Create(std::string_view name, std::string_view email, git_time_t time, int offset) {
        git_signature* x = nullptr;
        int ir = git_signature_new(&x, name.data(), email.data(), time, offset);
        if (ir) throw RuntimeError("git_signature_new failed: %s", git_error_last()->message);
        return x;
    }
    
    std::string name() { return (*get())->name; }
    std::string email() { return (*get())->email; }
};

static void _BufDelete(git_buf& buf) { git_buf_dispose(&buf); }
using Buf = RefCounted<git_buf, _BufDelete>;

struct Object : RefCounted<git_object*, git_object_free> {
    using RefCounted::RefCounted;
    const Id& id() const { return *git_object_id(*get()); }
    bool operator==(const Object& x) const { return _Equal(*this, x, git_oid_cmp(&id(), &x.id())==0); }
    bool operator!=(const Object& x) const { return !(*this==x); }
    bool operator<(const Object& x) const { return _Less(*this, x, git_oid_cmp(&id(), &x.id())<0); }
};

struct Config : RefCounted<git_config*, git_config_free> {
    using RefCounted::RefCounted;
    std::optional<std::string> stringGet(std::string_view key) {
        Buf buf;
        {
            git_buf x = GIT_BUF_INIT;
            int ir = git_config_get_string_buf(&x, *get(), key.data());
            if (ir == GIT_ENOTFOUND) return std::nullopt;
            if (ir) throw RuntimeError("git_config_get_string_buf failed: %s", git_error_last()->message);
            buf = x;
        }
        return buf->ptr;
    }
};

struct Commit : Object {
    using Object::Object;
    Commit(const git_commit* x) : Object((git_object*)x) {}
    const git_commit** get() const { return (const git_commit**)Object::get(); }
    const git_commit*& operator*() const { return *get(); }
    
//    operator const git_object**() const { return get(); }
//    operator const git_commit**() const { return (const git_commit**)get(); }
    
    static Commit FromObject(Object obj) {
        git_commit* x = nullptr;
        int ir = git_object_peel((git_object**)&x, *obj, GIT_OBJECT_COMMIT);
        if (ir) throw RuntimeError("git_object_peel failed: %s", git_error_last()->message);
        return x;
    }
    
//    Object object() const {
//        git_object* x = nullptr;
//        int ir = git_object_dup(&x, (git_object*)*get());
//        if (ir) throw RuntimeError("git_reference_dup failed: %s", git_error_last()->message);
//        return x;
//    }
    
    Tree tree() const {
        git_tree* x = nullptr;
        int ir = git_commit_tree(&x, *get());
        if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
        return x;
    }
    
    Commit parent(unsigned int n=0) const {
        git_commit* x = nullptr;
        int ir = git_commit_parent(&x, *get(), n);
        if (ir == GIT_ENOTFOUND) return nullptr; // `this` is the root commit -> no parent exists
        if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
        return x;
    }
    
    void printId() const {
        printf("%s\n", git_oid_tostr_s(&id()));
    }
};

struct TagAnnotation : Object {
    using Object::Object;
    TagAnnotation(const git_tag* x) : Object((git_object*)x) {}
    const git_tag** get() const { return (const git_tag**)Object::get(); }
    const git_tag*& operator*() const { return *get(); }
    
    Signature author() const {
        const git_signature* author = git_tag_tagger(*get());
        if (!author) return nullptr;
        git_signature* x = nullptr;
        int ir = git_signature_dup(&x, author);
        if (ir) throw RuntimeError("git_signature_dup failed: %s", git_error_last()->message);
        return x;
    }
    
    std::string name() const {
        return git_tag_name(*get());
    }
    
    std::string message() const {
        return git_tag_message(*get());
    }
    
//    static Tag FromRef(Ref ref) {
//        if (!ref.isTag()) return nullptr;
//        git_reference_target_peel(<#const git_reference *ref#>)
//        
//        git_tag_lookup(<#git_tag **out#>, <#git_repository *repo#>, <#const git_oid *id#>)
//        
//        git_reference* x = nullptr;
//        int ir = git_reference_dup(&x, *ref);
//        if (ir) throw RuntimeError("git_reference_dup failed: %s", git_error_last()->message);
//        return x;
//    }
//    
//    static Commit FromObject(Object obj) {
//        git_commit* x = nullptr;
//        int ir = git_object_peel((git_object**)&x, *obj, GIT_OBJECT_COMMIT);
//        if (ir) throw RuntimeError("git_object_peel failed: %s", git_error_last()->message);
//        return x;
//    }
};

struct Ref : RefCounted<git_reference*, git_reference_free> {
    using RefCounted::RefCounted;
    bool operator==(const Ref& x) const { return _Equal(*this, x, fullName()==x.fullName()); }
    bool operator!=(const Ref& x) const { return !(*this==x); }
    bool operator<(const Ref& x) const { return _Less(*this, x, fullName()<x.fullName()); }
    
    std::string name() const {
        return git_reference_shorthand(*get());
    }
    
    std::string fullName() const {
        return git_reference_name(*get());
    }
    
    bool isBranch() const {
        return git_reference_is_branch(*get());
    }
    
    bool isTag() const {
        return git_reference_is_tag(*get());
    }
    
    Commit commit() const {
        git_commit* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_COMMIT);
        if (ir) throw RuntimeError("git_reference_peel failed: %s", git_error_last()->message);
        return x;
    }
    
//    Tag tag() const {
//        git_tag* x = nullptr;
//        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_TAG);
//        if (ir) throw RuntimeError("git_reference_peel failed: %s", git_error_last()->message);
//        return x;
//    }
    
    Tree tree() const {
        git_tree* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_TREE);
        if (ir) throw RuntimeError("git_reference_peel failed: %s", git_error_last()->message);
        return x;
    }
};

//struct Tag : RefCounted<git_tag*, git_tag_free> {
//    using RefCounted::RefCounted;
//    const Id& id() const { return *git_object_id(*get()); }
//    bool operator==(const Object& x) const { return _Equal(*this, x, git_oid_cmp(&id(), &x.id())==0); }
//    bool operator!=(const Object& x) const { return !(*this==x); }
//    bool operator<(const Object& x) const { return _Less(*this, x, git_oid_cmp(&id(), &x.id())<0); }
//    
//    const char* name() const {
//        return git_tag_name(*get());
//    }
//};

//struct Tag : Ref {
//    using Ref::Ref;
//    
//    static Tag FromRef(Ref ref) {
//        if (!ref.isTag()) return nullptr;
//        git_reference* x = nullptr;
//        int ir = git_reference_dup(&x, *ref);
//        if (ir) throw RuntimeError("git_reference_dup failed: %s", git_error_last()->message);
//        return x;
//    }
//};

struct Branch : Ref {
    using Ref::Ref;
    
    static Branch FromRef(Ref ref) {
        if (!ref.isBranch()) throw RuntimeError("ref isn't a branch");
        git_reference* x = nullptr;
        int ir = git_reference_dup(&x, *ref);
        if (ir) throw RuntimeError("git_reference_dup failed: %s", git_error_last()->message);
        return x;
    }
    
    std::string name() const {
        const char* x = nullptr;
        int ir = git_branch_name(&x, *get());
        if (ir) throw RuntimeError("git_branch_name failed: %s", git_error_last()->message);
        return x;
    }
    
    Branch upstream() const {
        git_reference* x = nullptr;
        int ir = git_branch_upstream(&x, *get());
        switch (ir) {
        case 0:             return x;
        case GIT_ENOTFOUND: return nullptr;
        default:            throw RuntimeError("git_branch_upstream failed: %s", git_error_last()->message);
        }
    }
    
    void upstreamSet(Branch upstream) {
        int ir = git_branch_set_upstream(*get(), git_reference_shorthand(*upstream));
        if (ir) throw RuntimeError("git_branch_upstream_name failed: %s", git_error_last()->message);
    }
};

struct Tag : Ref {
    using Ref::Ref;
    
    static Tag FromRef(Ref ref) {
        if (!ref.isTag()) throw RuntimeError("ref isn't a tag");
        git_reference* x = nullptr;
        int ir = git_reference_dup(&x, *ref);
        if (ir) throw RuntimeError("git_reference_dup failed: %s", git_error_last()->message);
        return x;
    }
    
    TagAnnotation annotation() const {
        git_tag* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_TAG);
        // Allow failures, since we use this method to determine whether the tag is an annotated tag or not
        if (!ir) return x;
        return nullptr;
    }
};

// A Rev holds a mandatory commit, along with an optional reference (ie a branch/tag)
// that targets that commit
class Rev {
public:
    // Invalid Rev
    Rev() {}
    
    Rev(Commit c) : commit(c) {}
    
    Rev(Ref r) : ref(r) {
        commit = ref.commit();
        assert(commit); // Verify assumption: if we have a ref, it points to a valid commit
    }
    
    operator bool() const { return (bool)commit; }
    
    bool operator==(const Rev& x) const {
        if (commit != x.commit) return false;
        if (ref != x.ref) return false;
        return true;
    }
    
    bool operator!=(const Rev& x) const { return !(*this==x); }
    
    bool operator<(const Rev& x) const {
        // Stage 1: compare commits
        if (commit != x.commit) return commit<x.commit;
        // Stage 2: commits are the same; compare the refs
        return ref < x.ref;
    }
    
    inline bool isMutable() const {
        assert(*this);
        // Only ref-backed revs are mutable
        if (!ref) return false;
        return ref.isBranch() || ref.isTag();
    }
    
    Commit commit; // Mandatory
    Ref ref;       // Optional
};

struct Repo : RefCounted<git_repository*, git_repository_free> {
    using RefCounted::RefCounted;
    
    static Repo Open(std::string_view path) {
        git_repository* x = nullptr;
        int ir = git_repository_open(&x, path.data());
        if (ir) throw RuntimeError("git_repository_open failed: %s", git_error_last()->message);
        return x;
    }
    
    Ref head() const {
        git_reference* x = nullptr;
        int ir = git_repository_head(&x, *get());
        if (ir) throw RuntimeError("git_repository_head failed: %s", git_error_last()->message);
        return x;
    }
    
    void headDetach() const {
        int ir = git_repository_detach_head(*get());
        if (ir) throw RuntimeError("git_repository_detach_head failed: %s", git_error_last()->message);
    }
    
    void checkout(std::string_view name) const {
        Ref ref = refFullNameLookup(name);
        const git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        int ir = git_checkout_tree(*get(), (git_object*)*ref.tree(), &opts);
        if (ir) throw RuntimeError("git_checkout_tree failed: %s", git_error_last()->message);
        ir = git_repository_set_head(*get(), name.data());
        if (ir) throw RuntimeError("git_repository_set_head failed: %s", git_error_last()->message);
    }
    
    Index treesMerge(Tree ancestorTree, Tree dstTree, Tree srcTree) const {
        git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
        git_index* x = nullptr;
        int ir = git_merge_trees(&x, *get(), (ancestorTree ? *ancestorTree : nullptr), *dstTree, *srcTree, &mergeOpts);
        if (ir) throw RuntimeError("git_merge_trees failed: %s", git_error_last()->message);
        return x;
    }
    
    Tree indexWrite(Index index) const {
        git_oid treeId;
        int ir = git_index_write_tree_to(&treeId, *index, *get());
        if (ir) throw RuntimeError("git_index_write_tree_to failed: %s", git_error_last()->message);
        
        git_tree* x = nullptr;
        ir = git_tree_lookup(&x, *get(), &treeId);
        if (ir) throw RuntimeError("git_tree_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    // Creates a new child commit of `parent` with the tree `tree`, using the metadata from `metadata`
    Commit commitAttach(Commit parent, Tree tree, Commit metadata) const {
        git_oid id;
        // `parent` can be null if we're creating a root commit
        const git_commit* parents[] = {(parent ? *parent : nullptr)};
        const size_t parentsLen = (parent ? 1 : 0);
        int ir = git_commit_create(
            &id,
            *get(),
            nullptr,
            git_commit_author(*metadata),
            git_commit_committer(*metadata),
            git_commit_message_encoding(*metadata),
            git_commit_message(*metadata),
            *tree,
            parentsLen,
            parents
        );
        if (ir) throw RuntimeError("git_commit_create failed: %s", git_error_last()->message);
        return commitLookup(id);
    }
    
    // commitAttach: attaches (cherry-picks) `src` onto `dst` and returns the result
    Commit commitAttach(Commit dst, Commit src) const {
        Tree srcTree = src.tree();
        Tree newTree;
        // `dst` can be null if we're making `src` a root commit
        if (dst) {
            Tree dstTree = dst.tree();
            Commit srcParent = src.parent();
            Tree ancestorTree = srcParent ? srcParent.tree() : nullptr;
            Index mergedTreesIndex = treesMerge(ancestorTree, dstTree, srcTree);
            newTree = indexWrite(mergedTreesIndex);
        } else {
            newTree = src.tree();
        }
        
        return commitAttach(dst, newTree, src);
    }
    
    // commitIntegrate: adds the content of `src` into `dst` and returns the result
    Commit commitIntegrate(Commit dst, Commit src) const {
        Tree srcTree = src.tree();
        Tree dstTree = dst.tree();
        Tree ancestorTree = src.parent().tree();
        Index mergedTreesIndex = treesMerge(ancestorTree, dstTree, srcTree);
        Tree newTree = indexWrite(mergedTreesIndex);
        
        // Combine the commit messages
        std::stringstream msg;
        msg << git_commit_message(*dst);
        msg << "\n";
        msg << git_commit_message(*src);
        
        git_oid id;
        int ir = git_commit_amend(&id, *dst, nullptr, nullptr, nullptr, git_commit_message_encoding(*dst), msg.str().c_str(), *newTree);
        if (ir) throw RuntimeError("git_commit_amend failed: %s", git_error_last()->message);
        return commitLookup(id);
    }
    
    Commit commitAmend(Commit commit, Signature author, std::string_view msg) const {
        git_oid id;
        int ir = git_commit_amend(&id, *commit, nullptr, *author, nullptr,
            nullptr, (!msg.empty() ? msg.data() : nullptr), nullptr);
        if (ir) throw RuntimeError("git_commit_amend failed: %s", git_error_last()->message);
        return commitLookup(id);
    }
    
    Commit commitLookup(const Id& id) const {
        git_commit* x = nullptr;
        int ir = git_commit_lookup(&x, *get(), &id);
        if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    Commit commitLookup(std::string_view idStr) const {
        Id id;
        int ir = git_oid_fromstr(&id, idStr.data());
        if (ir) throw RuntimeError("git_oid_fromstr failed: %s", git_error_last()->message);
        return commitLookup(id);
    }
    
    Ref refReplace(Ref ref, Commit commit) const {
        if (ref.isBranch()) {
            return branchReplace(Branch::FromRef(ref), commit);
        
        } else if (ref.isTag()) {
            return tagReplace(Tag::FromRef(ref), commit);
//            return refReload(ref);
//            return tagReplace(ref.tag(), commit);
        
        } else {
            throw RuntimeError("unknown ref type");
        }
    }
    
    Branch branchReplace(Branch branch, Commit commit) const {
        Branch upstream = branch.upstream();
        Branch newBranch = branchCreate(branch.name(), commit, true);
        Branch newBranchUpstream = newBranch.upstream();
        
        if (upstream && !newBranchUpstream) {
            newBranch.upstreamSet(upstream);
        }
        
        return newBranch;
    }
    
    Tag tagReplace(Tag tag, Commit commit) const {
        if (TagAnnotation ann = tag.annotation()) {
            return tagCreateAnnotated(tag.name(), commit, ann.author(), ann.message(), true);
        }
        return tagCreate(tag.name(), commit, true);
//        } else {
//            return tagCreate(tag.name(), commit, true);
//        }
//        
//    Tag tagCreate(std::string_view name, Commit commit, Signature author, std::string_view message, bool force=false) const {
//        return tagCreate(tag.fullName(), commit, tag.author(), tag.message(), true);
//        git_tag_create(<#git_oid *oid#>, <#git_repository *repo#>, <#const char *tag_name#>, <#const git_object *target#>, <#const git_signature *tagger#>, <#const char *message#>, <#int force#>)
//        Branch upstream = branch.upstream();
//        Branch newBranch = branchCreate(branch.name(), commit, true);
//        Branch newBranchUpstream = newBranch.upstream();
//        
//        if (upstream && !newBranchUpstream) {
//            newBranch.upstreamSet(upstream);
//        }
//        
//        return newBranch;
    }
    
    Ref refLookup(std::string_view name) const {
        git_reference* x = nullptr;
        int ir = git_reference_dwim(&x, *get(), name.data());
        if (ir) throw RuntimeError("git_reference_dwim failed: %s", git_error_last()->message);
        return x;
    }
    
    Ref refFullNameLookup(std::string_view name) const {
        git_reference* x = nullptr;
        int ir = git_reference_lookup(&x, *get(), name.data());
        if (ir) throw RuntimeError("git_reference_dwim failed: %s", git_error_last()->message);
        return x;
    }
    
    Ref refReload(Ref ref) const {
        return refFullNameLookup(ref.fullName());
    }
    
    Rev revLookup(std::string_view str) {
        Object obj;
        Ref ref;
        
        // Determine whether `str` is a ref or commit
        {
            git_object* po = nullptr;
            git_reference* pr = nullptr;
            int ir = git_revparse_ext(&po, &pr, *get(), str.data());
            if (ir) throw RuntimeError("git_revparse_ext failed: %s", git_error_last()->message);
            obj = po;
            ref = pr;
        }
        
        // If we have a ref, construct this Rev as a ref
        if (ref) return Rev(ref);
        return Rev(Commit::FromObject(obj));
    }
    
    Branch branchLookup(std::string_view name) const {
        git_reference* x = nullptr;
        int ir = git_branch_lookup(&x, *get(), name.data(), GIT_BRANCH_LOCAL);
        if (ir) throw RuntimeError("git_branch_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    Branch branchCreate(std::string_view name, Commit commit, bool force=false) const {
        git_reference* x = nullptr;
        int ir = git_branch_create(&x, *get(), name.data(), (commit ? *commit : nullptr), force);
        if (ir) throw RuntimeError("git_branch_create failed: %s", git_error_last()->message);
        return x;
    }
    
    Tag tagLookup(std::string_view name) const {
        return Tag::FromRef(refLookup(name));
    }
    
    Tag tagCreate(std::string_view name, Commit commit, bool force=false) const {
        git_oid id;
        int ir = git_tag_create_lightweight(&id, *get(), name.data(), *(Object)commit, force);
        if (ir) throw RuntimeError("git_tag_create failed: %s", git_error_last()->message);
        return tagLookup(name);
    }
    
    Tag tagCreateAnnotated(std::string_view name, Commit commit, Signature author, std::string_view message, bool force=false) const {
        git_oid id;
        int ir = git_tag_create(&id, *get(), name.data(), *((Object)commit), *author, message.data(), force);
        if (ir) throw RuntimeError("git_tag_create failed: %s", git_error_last()->message);
        return tagLookup(name);
    }
    
    Config config() const {
        git_config* x = nullptr;
        int ir = git_repository_config(&x, *get());
        if (ir) throw RuntimeError("git_repository_config failed: %s", git_error_last()->message);
        return x;
    }
};

inline std::string StringForId(const git_oid& oid) {
    char str[8];
    git_oid_tostr(str, sizeof(str), &oid);
    return str;
}

inline std::string ShortStringForTime(git_time_t t) {
    time_t tmp = t;
    struct tm tm = {};
    struct tm* tr = localtime_r(&tmp, &tm);
    if (!tr) throw RuntimeError("localtime_r failed: %s", strerror(errno));
    
    char buf[64];
    size_t sr = strftime(buf, sizeof(buf), "%a %b %d %R", &tm);
    if (!sr) throw RuntimeError("strftime failed");
    return buf;
}

inline std::string StringFromTime(git_time_t t) {
    time_t tmp = t;
    struct tm tm = {};
    struct tm* tr = localtime_r(&tmp, &tm);
    if (!tr) throw RuntimeError("localtime_r failed: %s", strerror(errno));
    
    char buf[64];
    size_t sr = strftime(buf, sizeof(buf), "%a %b %d %T %Y %z", &tm);
    if (!sr) throw RuntimeError("strftime failed");
    return buf;
}

inline git_time_t TimeFromString(std::string_view str, int* offset=nullptr) {
    struct tm tm = {};
    char* cr = strptime(str.data(), "%a %b %d %T %Y %z", &tm);
    if (!cr) throw RuntimeError("strptime failed");
    
    if (offset) *offset = (int)(tm.tm_gmtoff/60);
    
    time_t t = mktime(&tm);
    if (t == -1) throw RuntimeError("mktime failed");
    return t;
}

#undef _Equal
#undef _Less

} // namespace Git
