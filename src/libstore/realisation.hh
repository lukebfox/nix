#pragma once

#include "path.hh"
#include <nlohmann/json_fwd.hpp>


// Awfull hacky generation of the comparison operators
// Use it with
#define GENERATE_ONE_CMP(COMPARATOR, MY_TYPE, FIELDS...) \
    bool operator COMPARATOR(const MY_TYPE& other) const { \
      const MY_TYPE* me = this; \
      auto fields1 = std::make_tuple( FIELDS ); \
      me = &other; \
      auto fields2 = std::make_tuple( FIELDS ); \
      return fields1 COMPARATOR fields2; \
    }
#define GENERATE_EQUAL(args...) GENERATE_ONE_CMP(==, args)
#define GENERATE_LEQ(args...) GENERATE_ONE_CMP(<, args)
#define GENERATE_CMP(args...) \
    GENERATE_EQUAL(args) \
    GENERATE_LEQ(args)

namespace nix {

struct DrvOutput {
    StorePath drvPath;
    std::string outputName;

    std::string to_string() const;

    static DrvOutput parse(const std::string &);

    GENERATE_CMP(DrvOutput, me->drvPath, me->outputName);
};

struct Realisation {
    DrvOutput id;
    StorePath outPath;

    nlohmann::json toJSON() const;
    static Realisation fromJSON(const nlohmann::json& json, const std::string& whence);

    StorePath getPath() const { return outPath; }

    GENERATE_CMP(Realisation, me->id, me->outPath);
};

struct OpaquePath {
    StorePath path;

    StorePath getPath() const { return path; }

    GENERATE_CMP(OpaquePath, me->path);
};


/**
 * A store path with all the history of how it went into the store
 */
struct RealisedPath {
    /*
     * A path is either the result of the realisation of a derivation or
     * an opaque blob that has been directly added to the store
     */
    using Raw = std::variant<Realisation, OpaquePath>;
    Raw raw;

    using Set = std::set<RealisedPath>;

    RealisedPath(StorePath path) : raw(OpaquePath{path}) {}
    RealisedPath(Realisation r) : raw(r) {}

    /**
     * Syntactic sugar to run `std::visit` on the raw value:
     * path.visit(blah) == std::visit(blah, path.raw)
     */
    template <class Visitor>
    constexpr decltype(auto) visit(Visitor && vis) {
        return std::visit(vis, raw);
    }
    template <class Visitor>
    constexpr decltype(auto) visit(Visitor && vis) const {
        return std::visit(vis, raw);
    }

    /**
     * Get the raw store path associated to this
     */
    StorePath path() const;

    void closure(Store& store, Set& ret) const;
    static void closure(Store& store, const Set& startPaths, Set& ret);
    Set closure(Store& store) const;

    GENERATE_CMP(RealisedPath, me->raw);
};

}
