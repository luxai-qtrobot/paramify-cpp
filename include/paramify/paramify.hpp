#pragma once

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <set>

// yaml-cpp
#include <yaml-cpp/yaml.h>

// CLI11 (header-only)
#include <CLI/CLI.hpp>

namespace paramify
{

// -----------------------------
// Errors
// -----------------------------
struct ParamError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct ParamNotFound : public ParamError
{
    using ParamError::ParamError;
};
struct ParamTypeError : public ParamError
{
    using ParamError::ParamError;
};
struct ParamParseError : public ParamError
{
    using ParamError::ParamError;
};
struct ParamSchemaError : public ParamError
{
    using ParamError::ParamError;
};

// -----------------------------
// Types
// -----------------------------
enum class ValueType
{
    Bool,
    Int,
    Double,
    String,
    ListBool,
    ListInt,
    ListDouble,
    ListString
};

inline const char *to_string(ValueType t)
{
    switch (t)
    {
    case ValueType::Bool:
        return "bool";
    case ValueType::Int:
        return "int";
    case ValueType::Double:
        return "double";
    case ValueType::String:
        return "string";
    case ValueType::ListBool:
        return "list[bool]";
    case ValueType::ListInt:
        return "list[int]";
    case ValueType::ListDouble:
        return "list[double]";
    case ValueType::ListString:
        return "list[string]";
    }
    return "unknown";
}

enum class Scope
{
    All,
    Cli,
    Runtime
};

inline const char *to_string(Scope s)
{
    switch (s)
    {
    case Scope::All:
        return "all";
    case Scope::Cli:
        return "cli";
    case Scope::Runtime:
        return "runtime";
    }
    return "unknown";
}

// -----------------------------
// Schema + Value
// -----------------------------
struct ParamDef
{
    std::string name;
    ValueType type;
    Scope scope = Scope::All;
    std::string description; // CLI help
    bool has_default = false; // (we keep name for minimal change; now means "has value/default")
};

struct Value
{
    ValueType type;

    bool b = false;
    int64_t i = 0;
    double d = 0.0;
    std::string s;

    std::vector<bool> lb;
    std::vector<int64_t> li;
    std::vector<double> ld;
    std::vector<std::string> ls;

    static Value make_bool(bool v)
    {
        Value x;
        x.type = ValueType::Bool;
        x.b = v;
        return x;
    }
    static Value make_int(int64_t v)
    {
        Value x;
        x.type = ValueType::Int;
        x.i = v;
        return x;
    }
    static Value make_double(double v)
    {
        Value x;
        x.type = ValueType::Double;
        x.d = v;
        return x;
    }
    static Value make_string(std::string v)
    {
        Value x;
        x.type = ValueType::String;
        x.s = std::move(v);
        return x;
    }

    static Value make_list_bool(std::vector<bool> v)
    {
        Value x;
        x.type = ValueType::ListBool;
        x.lb = std::move(v);
        return x;
    }
    static Value make_list_int(std::vector<int64_t> v)
    {
        Value x;
        x.type = ValueType::ListInt;
        x.li = std::move(v);
        return x;
    }
    static Value make_list_double(std::vector<double> v)
    {
        Value x;
        x.type = ValueType::ListDouble;
        x.ld = std::move(v);
        return x;
    }
    static Value make_list_string(std::vector<std::string> v)
    {
        Value x;
        x.type = ValueType::ListString;
        x.ls = std::move(v);
        return x;
    }
};

// -----------------------------
// Helpers
// -----------------------------
inline std::string kebabize(std::string x)
{
    std::replace(x.begin(), x.end(), '_', '-');
    std::replace(x.begin(), x.end(), '.', '-'); // NEW: support hierarchical keys
    return x;
}

inline Scope parse_scope(const YAML::Node &n)
{
    // Default scope if missing
    if (!n)
        return Scope::Runtime;

    auto s = n.as<std::string>();
    if (s == "all")
        return Scope::All;
    if (s == "cli")
        return Scope::Cli;
    if (s == "runtime")
        return Scope::Runtime;
    throw ParamSchemaError("Invalid scope: " + s + " (expected all|cli|runtime)");
}

inline ValueType parse_type(const std::string &t)
{
    if (t == "bool")
        return ValueType::Bool;
    if (t == "int")
        return ValueType::Int;
    if (t == "double")
        return ValueType::Double;
    if (t == "string")
        return ValueType::String;
    if (t == "list[bool]")
        return ValueType::ListBool;
    if (t == "list[int]")
        return ValueType::ListInt;
    if (t == "list[double]")
        return ValueType::ListDouble;
    if (t == "list[string]")
        return ValueType::ListString;
    throw ParamSchemaError("Invalid type: " + t);
}

inline Value yaml_to_value(const YAML::Node &n, ValueType t)
{
    try
    {
        switch (t)
        {
        case ValueType::Bool:
            return Value::make_bool(n.as<bool>());
        case ValueType::Int:
            return Value::make_int(n.as<int64_t>());
        case ValueType::Double:
            return Value::make_double(n.as<double>());
        case ValueType::String:
            return Value::make_string(n.as<std::string>());

        case ValueType::ListBool:
        {
            if (!n.IsSequence())
                throw ParamParseError("Expected sequence for list[bool]");
            std::vector<bool> v;
            for (auto it : n)
                v.push_back(it.as<bool>());
            return Value::make_list_bool(std::move(v));
        }
        case ValueType::ListInt:
        {
            if (!n.IsSequence())
                throw ParamParseError("Expected sequence for list[int]");
            std::vector<int64_t> v;
            for (auto it : n)
                v.push_back(it.as<int64_t>());
            return Value::make_list_int(std::move(v));
        }
        case ValueType::ListDouble:
        {
            if (!n.IsSequence())
                throw ParamParseError("Expected sequence for list[double]");
            std::vector<double> v;
            for (auto it : n)
                v.push_back(it.as<double>());
            return Value::make_list_double(std::move(v));
        }
        case ValueType::ListString:
        {
            if (!n.IsSequence())
                throw ParamParseError("Expected sequence for list[string]");
            std::vector<std::string> v;
            for (auto it : n)
                v.push_back(it.as<std::string>());
            return Value::make_list_string(std::move(v));
        }
        }
    }
    catch (const YAML::BadConversion &e)
    {
        throw ParamParseError(std::string("YAML conversion failed for type ") + to_string(t) + ": " + e.what());
    }
    throw ParamParseError("Unsupported type");
}

inline YAML::Node value_to_yaml(const Value &v)
{
    YAML::Node n;
    switch (v.type)
    {
    case ValueType::Bool:
        n = v.b;
        break;
    case ValueType::Int:
        n = v.i;
        break;
    case ValueType::Double:
        n = v.d;
        break;
    case ValueType::String:
        n = v.s;
        break;

    case ValueType::ListBool:
    {
        n = YAML::Node(YAML::NodeType::Sequence);
        for (bool x : v.lb)
            n.push_back(x);
        break;
    }
    case ValueType::ListInt:
    {
        n = YAML::Node(YAML::NodeType::Sequence);
        for (auto x : v.li)
            n.push_back(x);
        break;
    }
    case ValueType::ListDouble:
    {
        n = YAML::Node(YAML::NodeType::Sequence);
        for (auto x : v.ld)
            n.push_back(x);
        break;
    }
    case ValueType::ListString:
    {
        n = YAML::Node(YAML::NodeType::Sequence);
        for (auto &x : v.ls)
            n.push_back(x);
        break;
    }
    }
    return n;
}

// path helpers (relative only)
inline std::string dirname(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? std::string() : path.substr(0, pos);
}

inline std::string join_path(const std::string& base, const std::string& rel)
{
    if (rel.empty())
        return base;
    if (base.empty())
        return rel;
    return base + "/" + rel;
}


// -----------------------------
// Forward decl
// -----------------------------
class Paramify;

// -----------------------------
// ParamRef proxy (true map semantics)
// -----------------------------
class ParamRef
{
public:
    ParamRef(Paramify *owner, std::string key) : owner_(owner), key_(std::move(key)) {}

    // reads (exact type)
    operator bool() const;
    operator int64_t() const;
    operator double() const;
    operator std::string() const;

    operator std::vector<bool>() const;
    operator std::vector<int64_t>() const;
    operator std::vector<double>() const;
    operator std::vector<std::string>() const;

    // writes (allow int->double widening only)
    ParamRef &operator=(bool v);
    ParamRef &operator=(int v) { return (*this) = static_cast<int64_t>(v); }
    ParamRef &operator=(int64_t v);
    ParamRef &operator=(double v);
    ParamRef &operator=(const char *v) { return (*this) = std::string(v); }
    ParamRef &operator=(std::string v);

    ParamRef &operator=(std::vector<bool> v);
    ParamRef &operator=(std::vector<int64_t> v);
    ParamRef &operator=(std::vector<double> v);
    ParamRef &operator=(std::vector<std::string> v);

private:
    Paramify *owner_;
    std::string key_;
};

// -----------------------------
// Paramify
// -----------------------------
class Paramify
{
public:
    struct Options
    {
        bool enable_cli = true;
    };

    Paramify() = default;

    explicit Paramify(const std::string &yaml_path)
        : Paramify(yaml_path, Options{}) {}

    explicit Paramify(const std::string &yaml_path, const Options &opt)
        : file_path_(yaml_path), options_(opt)
    {
        load_from_file(yaml_path);
    }

    // Map-style
    ParamRef operator[](const std::string &key) { return ParamRef(this, key); }
    ParamRef operator[](const char *key) { return ParamRef(this, std::string(key)); }

    // Const map-style
    ParamRef operator[](const std::string &key) const { return ParamRef(const_cast<Paramify*>(this), key); }
    ParamRef operator[](const char *key) const { return ParamRef(const_cast<Paramify*>(this), std::string(key)); }

    // -----------------------------
    // Programmatic schema
    // -----------------------------
    void add_param(const std::string &name,
                   ValueType type,
                   Value default_value,
                   Scope scope = Scope::All,
                   const std::string &description = "")
    {
        ParamDef def;
        def.name = name;
        def.type = type;
        def.scope = scope;
        def.description = description;
        def.has_default = true;

        defs_[name] = def;
        values_[name] = std::move(default_value);

        yaml_loaded_ = false; // schema did not come from YAML
        file_path_.clear();   // no persistence unless user loads YAML later
    }

    // Convenient overloads
    void add_param(const std::string &name, ValueType type, bool def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_bool(def), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, int64_t def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_int(def), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, double def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_double(def), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, std::string def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_string(std::move(def)), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, std::vector<bool> def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_list_bool(std::move(def)), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, std::vector<int64_t> def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_list_int(std::move(def)), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, std::vector<double> def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_list_double(std::move(def)), scope, desc);
    }
    void add_param(const std::string &name, ValueType type, std::vector<std::string> def, Scope scope = Scope::All, const std::string &desc = "")
    {
        add_param(name, type, Value::make_list_string(std::move(def)), scope, desc);
    }

    // -----------------------------
    // YAML load
    // -----------------------------
    void load_from_file(const std::string &yaml_path)
    {
        YAML::Node root = YAML::LoadFile(yaml_path);
        file_path_ = yaml_path;
        load_from_yaml_node(root);
    }

    void load_from_yaml_string(const std::string &yaml_text)
    {
        YAML::Node root = YAML::Load(yaml_text);
        file_path_.clear();
        load_from_yaml_node(root);
    }

    void load_from_yaml_node(const YAML::Node &root)
    {
        if (!root || !root.IsMap())
            throw ParamSchemaError("Root YAML must be a map");

        if (root["description"])
            app_description_ = root["description"].as<std::string>();
        if (root["name"])
            app_name_ = root["name"].as<std::string>();

        YAML::Node params = root["parameters"];
        if (!params || !params.IsSequence())
            throw ParamSchemaError("Expected 'parameters' as a sequence");

        defs_.clear();
        values_.clear();

        // NEW: recursive loader to support nested groups + flat style
        // auto load_params = [&](auto &&self, const YAML::Node &seq, const std::string &prefix) -> void
        // {
        //     if (!seq || !seq.IsSequence())
        //         throw ParamSchemaError("Expected 'parameters' as a sequence");

        //     for (auto p : seq)
        //     {
        //         if (!p.IsMap())
        //             throw ParamSchemaError("Each item in 'parameters' must be a map");

        //         // Detect group (supports infinite nesting)
        //         const bool has_child_params = static_cast<bool>(p["parameters"]);
        //         const bool is_group_type = (p["type"] && p["type"].IsScalar() && p["type"].as<std::string>() == "group");
        //         if (has_child_params || is_group_type)
        //         {
        //             if (!p["name"])
        //                 throw ParamSchemaError("Group item is missing 'name'");
        //             const std::string gname = p["name"].as<std::string>();
        //             const std::string next_prefix = prefix.empty() ? (gname + ".") : (prefix + gname + ".");
        //             YAML::Node child = p["parameters"];
        //             if (!child || !child.IsSequence())
        //                 throw ParamSchemaError("Group '" + gname + "' must have 'parameters' as a sequence");
        //             self(self, child, next_prefix);
        //             continue;
        //         }

        //         // Leaf param (flat style or inside group)
        //         if (!p["name"])
        //             throw ParamSchemaError("Parameter item is missing 'name'");
        //         if (!p["type"])
        //             throw ParamSchemaError("Parameter '" + p["name"].as<std::string>() + "' is missing 'type'");

        //         ParamDef def;
        //         def.name = prefix + p["name"].as<std::string>();
        //         def.type = parse_type(p["type"].as<std::string>());
        //         def.scope = parse_scope(p["scope"]); // default runtime if missing
        //         if (p["description"])
        //             def.description = p["description"].as<std::string>();

        //         defs_[def.name] = def;

        //         // NEW: value preferred; fallback to default for backward compatibility
        //         YAML::Node val_node = p["value"] ? p["value"] : p["default"];
        //         if (val_node)
        //         {
        //             values_[def.name] = yaml_to_value(val_node, def.type);
        //             defs_[def.name].has_default = true;
        //         }
        //         else
        //         {
        //             defs_[def.name].has_default = false;
        //         }
        //     }
        // };

        std::set<std::string> include_stack;
        auto load_params = [&](auto &&self,
                            const YAML::Node &seq,
                            const std::string &prefix,
                            const std::string &base_dir) -> void
        {
            if (!seq || !seq.IsSequence())
                throw ParamSchemaError("Expected 'parameters' as a sequence");

            for (auto p : seq) {
                if (!p.IsMap())
                    throw ParamSchemaError("Each item in 'parameters' must be a map");

                // Detect group
                const bool is_group_type =
                    (p["type"] && p["type"].IsScalar() && p["type"].as<std::string>() == "group");

                if (is_group_type) {
                    if (!p["name"])
                        throw ParamSchemaError("Group item is missing 'name'");

                    const std::string gname = p["name"].as<std::string>();
                    const std::string next_prefix =
                        prefix.empty() ? (gname + ".") : (prefix + gname + ".");

                    // ---- NEW: handle include (string or list)
                    if (p["include"]) {
                        YAML::Node inc = p["include"];
                        std::vector<std::string> files;

                        if (inc.IsScalar())
                            files.push_back(inc.as<std::string>());
                        else if (inc.IsSequence())
                            for (auto x : inc)
                                files.push_back(x.as<std::string>());
                        else
                            throw ParamSchemaError("'include' must be string or list");

                        for (const auto &rel_path : files)
                        {
                            if (!rel_path.empty() && rel_path[0] == '/')
                                throw ParamSchemaError("Absolute paths not allowed in include: " + rel_path);

                            const std::string full =
                                join_path(base_dir, rel_path);

                            if (!include_stack.insert(full).second)
                                throw ParamSchemaError("Circular include detected: " + full);

                            YAML::Node inc_root = YAML::LoadFile(full);
                            YAML::Node inc_params = inc_root["parameters"];
                            if (!inc_params || !inc_params.IsSequence())
                                throw ParamSchemaError("Included file must contain 'parameters'");

                            self(self,
                                inc_params,
                                next_prefix,
                                dirname(full));

                            include_stack.erase(full);
                        }
                    }

                    // Inline parameters still allowed and override includes
                    if (p["parameters"]) {
                        self(self,
                            p["parameters"],
                            next_prefix,
                            base_dir);
                    }
                    continue;
                }

                // ---- Leaf parameter (unchanged)
                if (!p["name"])
                    throw ParamSchemaError("Parameter item is missing 'name'");
                if (!p["type"])
                    throw ParamSchemaError("Parameter '" + p["name"].as<std::string>() + "' is missing 'type'");

                ParamDef def;
                def.name = prefix + p["name"].as<std::string>();
                def.type = parse_type(p["type"].as<std::string>());
                def.scope = parse_scope(p["scope"]);
                if (p["description"])
                    def.description = p["description"].as<std::string>();

                defs_[def.name] = def;

                YAML::Node val_node = p["value"] ? p["value"] : p["default"];
                if (val_node) {
                    values_[def.name] = yaml_to_value(val_node, def.type);
                    defs_[def.name].has_default = true;
                }
                else { defs_[def.name].has_default = false; }
            }
        };   

        load_params(load_params, params, "", dirname(file_path_));

        yaml_loaded_ = true;
        original_root_ = root;
    }

    // -----------------------------
    // Save (optional)
    // -----------------------------
    void save_to_file(const std::string &yaml_path = "") const
    {
        std::string out = yaml_path.empty() ? file_path_ : yaml_path;
        if (out.empty())
            throw ParamError("No file path set for save");

        YAML::Node root = original_root_ ? original_root_ : YAML::Node(YAML::NodeType::Map);
        YAML::Node params = root["parameters"];

        if (!params || !params.IsSequence())
            throw ParamSchemaError("Cannot save: 'parameters' missing or not a sequence");

        // NEW: recursive saver to support nested groups + flat style
        auto save_params = [&](auto &&self, YAML::Node seq, const std::string &prefix) -> void
        {
            if (!seq || !seq.IsSequence())
                throw ParamSchemaError("Cannot save: 'parameters' missing or not a sequence");

            for (std::size_t idx = 0; idx < seq.size(); ++idx)
            {
                YAML::Node p = seq[idx];
                if (!p || !p.IsMap())
                    continue;

                const bool has_child_params = static_cast<bool>(p["parameters"]);
                const bool is_group_type = (p["type"] && p["type"].IsScalar() && p["type"].as<std::string>() == "group");

                if (has_child_params || is_group_type)
                {
                    if (!p["name"])
                        continue;
                    const std::string gname = p["name"].as<std::string>();
                    const std::string next_prefix = prefix.empty() ? (gname + ".") : (prefix + gname + ".");
                    YAML::Node child = p["parameters"];
                    if (child && child.IsSequence())
                        self(self, child, next_prefix);
                    seq[idx] = p;
                    continue;
                }

                if (!p["name"])
                    continue;

                const std::string full_name = prefix + p["name"].as<std::string>();

                auto itv = values_.find(full_name);
                if (itv == values_.end())
                    continue; // unset => do not write

                // NEW: write into "value" (preferred). Also update "default" if it existed for backward-compat.
                p["value"] = value_to_yaml(itv->second);
                if (p["default"])
                    p["default"] = p["value"];

                seq[idx] = p;
            }
        };

        save_params(save_params, params, "");

        std::ofstream f(out.c_str(), std::ios::out | std::ios::trunc);
        if (!f)
            throw ParamError("Failed to open for write: " + out);
        f << root;
    }

    // CLI integration (CLI11)
    // --------------------------------------------------
    // Returns true  -> continue running
    // Returns false -> help was shown OR parse error already printed (caller should exit)
    bool apply_cli(int argc, char **argv)
    {
        if (!options_.enable_cli)
            return true;

        bool help_requested = false;

        // -----------------------------
        // Pass 1: parse only --config
        // -----------------------------
        CLI::App app(app_description_.empty() ? "paramify app" : app_description_);
        app.allow_extras(true);

        std::string config_path;
        auto *cfg_opt = app.add_option("--config", config_path, "YAML configuration file");
        cfg_opt->expected(1);

        // --config required only if NO schema exists at all
        if (!yaml_loaded_ && file_path_.empty() && defs_.empty())
            cfg_opt->required();

        // Helper: recover --config from argv when CLI11 exits early on --help
        auto find_config_arg = [&](int argc2, char **argv2) -> std::string
        {
            for (int i = 1; i < argc2; ++i)
            {
                std::string a = argv2[i];
                if (a == "--config" && i + 1 < argc2)
                    return argv2[i + 1];
                if (a.rfind("--config=", 0) == 0)
                    return a.substr(std::string("--config=").size());
            }
            return {};
        };

        try
        {
            app.parse(argc, argv);
        }
        catch (const CLI::CallForHelp &)
        {
            // IMPORTANT: don't print app.help() here (it's incomplete)
            help_requested = true;

            // CLI11 exits early on --help, so config_path may not be populated.
            // Recover it from argv so we can load YAML and print full help in pass-2.
            config_path = find_config_arg(argc, argv);
        }
        catch (const CLI::ParseError &e)
        {
            std::cerr << e.what() << "\n";
            std::cerr << app.help();
            throw;
        }

        // If schema isn't available yet, load it (needed to build dynamic options)
        if (!config_path.empty())
        {
            load_from_file(config_path);
        }
        else if (!yaml_loaded_ && !file_path_.empty())
        {
            load_from_file(file_path_);
        }
        else if (!yaml_loaded_ && file_path_.empty() && defs_.empty())
        {
            // No schema: if user asked for help, we can only show pass-1 help
            if (help_requested)
            {
                std::cout << app.help();
                return false;
            }
            throw ParamError("No schema loaded. Provide --config <file> or add_param(...) before apply_cli().");
        }

        // -----------------------------
        // Pass 2: build dynamic options
        // -----------------------------
        CLI::App app2(app_description_.empty() ? "paramify app" : app_description_);

        std::string config_path2;
        auto *cfg2 = app2.add_option("--config", config_path2, "YAML configuration file");
        cfg2->expected(1);
        app2.allow_extras(false); // strict

        struct OptBind
        {
            ValueType type;
            Scope scope;
            std::string name;

            CLI::Option *opt = nullptr;     // yes option (bool) or the option (others)
            CLI::Option *opt_no = nullptr;  // bool only: --no-...
            bool b_no = false;              // bool only

            bool b = false;
            int64_t i = 0;
            double d = 0.0;
            std::string s;

            std::vector<bool> lb;
            std::vector<int64_t> li;
            std::vector<double> ld;
            std::vector<std::string> ls;
        };

        std::vector<OptBind> binds;
        binds.reserve(defs_.size());

        for (const auto &kv : defs_)
        {
            const auto &def = kv.second;

            if (!(def.scope == Scope::Cli || def.scope == Scope::All))
                continue;

            // IMPORTANT: store bind first so CLI writes into valid memory
            binds.emplace_back();
            auto &b = binds.back();

            b.type = def.type;
            b.scope = def.scope;
            b.name = def.name;

            const std::string flag = "--" + kebabize(def.name);
            const std::string help = def.description.empty() ? def.name : def.description;

            auto itv = values_.find(def.name);

            switch (def.type)
            {
            case ValueType::Bool:
            {
                if (itv != values_.end())
                    b.b = itv->second.b;

                // --flag sets true (presence)
                auto *opt_yes = app2.add_flag(flag, b.b, help);

                // --no-flag sets b_no=true (presence)
                auto *opt_no = app2.add_flag("--no-" + kebabize(def.name), b.b_no, "Disable " + def.name);

                opt_yes->default_str(itv != values_.end() ? (b.b ? "true" : "false") : "false");

                b.opt = opt_yes;
                b.opt_no = opt_no;
                break;
            }
            case ValueType::Int:
            {
                if (itv != values_.end())
                    b.i = itv->second.i;
                auto *opt = app2.add_option(flag, b.i, help);
                opt->default_str(itv != values_.end() ? std::to_string(b.i) : "0");
                b.opt = opt;
                break;
            }
            case ValueType::Double:
            {
                if (itv != values_.end())
                    b.d = itv->second.d;
                auto *opt = app2.add_option(flag, b.d, help);
                opt->default_str(itv != values_.end() ? std::to_string(b.d) : "0.0");
                b.opt = opt;
                break;
            }
            case ValueType::String:
            {
                if (itv != values_.end())
                    b.s = itv->second.s;
                auto *opt = app2.add_option(flag, b.s, help);
                opt->default_str(itv != values_.end() ? b.s : "");
                b.opt = opt;
                break;
            }
            case ValueType::ListBool:
            {
                if (itv != values_.end())
                    b.lb = itv->second.lb;
                auto *opt = app2.add_option(flag, b.lb, help);
                opt->expected(-1);
                b.opt = opt;
                break;
            }
            case ValueType::ListInt:
            {
                if (itv != values_.end())
                    b.li = itv->second.li;
                auto *opt = app2.add_option(flag, b.li, help);
                opt->expected(-1);
                b.opt = opt;
                break;
            }
            case ValueType::ListDouble:
            {
                if (itv != values_.end())
                    b.ld = itv->second.ld;
                auto *opt = app2.add_option(flag, b.ld, help);
                opt->expected(-1);
                b.opt = opt;
                break;
            }
            case ValueType::ListString:
            {
                if (itv != values_.end())
                    b.ls = itv->second.ls;
                auto *opt = app2.add_option(flag, b.ls, help);
                opt->expected(-1);
                b.opt = opt;
                break;
            }
            }
        }

        // If help was requested in pass-1, print FULL help now and exit
        if (help_requested)
        {
            std::cout << app2.help();
            return false;
        }

        try
        {
            app2.parse(argc, argv);
        }
        catch (const CLI::CallForHelp &)
        {
            std::cout << app2.help();
            return false;
        }
        catch (const CLI::ParseError &e)
        {
            std::cerr << e.what() << "\n";
            std::cerr << app2.help();
            throw;
        }

        // Apply overrides only if provided
        for (auto &b : binds)
        {
            const bool yes_set = (b.opt && b.opt->count() > 0);
            const bool no_set = (b.opt_no && b.opt_no->count() > 0);

            if (!yes_set && !no_set)
                continue;

            switch (b.type)
            {
            case ValueType::Bool:
                // if both provided, prefer --no-...
                set_bool(b.name, no_set ? false : b.b);
                break;
            case ValueType::Int:
                set_int(b.name, b.i);
                break;
            case ValueType::Double:
                set_double(b.name, b.d);
                break;
            case ValueType::String:
                set_string(b.name, b.s);
                break;

            case ValueType::ListBool:
                set_list_bool(b.name, std::move(b.lb));
                break;
            case ValueType::ListInt:
                set_list_int(b.name, std::move(b.li));
                break;
            case ValueType::ListDouble:
                set_list_double(b.name, std::move(b.ld));
                break;
            case ValueType::ListString:
                set_list_string(b.name, std::move(b.ls));
                break;
            }
        }

        return true;
    }


    bool apply_cli_or_exit(int argc, char **argv)
    {
        try
        {
            return apply_cli(argc, argv);
        }
        catch (const CLI::ParseError &)
        {
            return false;
        }
    }

    
    // -----------------------------
    // typed core get/set (used by ParamRef)
    // -----------------------------
    bool get_bool(const std::string &k) const { return get_exact_bool(k); }
    int64_t get_int(const std::string &k) const { return get_exact_int(k); }
    double get_double(const std::string &k) const { return get_exact_double(k); }
    std::string get_string(const std::string &k) const { return get_exact_string(k); }

    std::vector<bool> get_list_bool(const std::string &k) const { return get_exact_list_bool(k); }
    std::vector<int64_t> get_list_int(const std::string &k) const { return get_exact_list_int(k); }
    std::vector<double> get_list_double(const std::string &k) const { return get_exact_list_double(k); }
    std::vector<std::string> get_list_string(const std::string &k) const { return get_exact_list_string(k); }

    void set_bool(const std::string &k, bool v) { set_exact(k, Value::make_bool(v), false); }
    void set_int(const std::string &k, int64_t v) { set_exact(k, Value::make_int(v), false); }
    void set_double(const std::string &k, double v) { set_exact(k, Value::make_double(v), true); } // int->double widening allowed
    void set_string(const std::string &k, std::string v) { set_exact(k, Value::make_string(std::move(v)), false); }

    void set_list_bool(const std::string &k, std::vector<bool> v) { set_exact(k, Value::make_list_bool(std::move(v)), false); }
    void set_list_int(const std::string &k, std::vector<int64_t> v) { set_exact(k, Value::make_list_int(std::move(v)), false); }
    void set_list_double(const std::string &k, std::vector<double> v) { set_exact(k, Value::make_list_double(std::move(v)), false); }
    void set_list_string(const std::string &k, std::vector<std::string> v) { set_exact(k, Value::make_list_string(std::move(v)), false); }

private:
    friend class ParamRef;

    const ParamDef &def_or_throw(const std::string &key) const
    {
        auto it = defs_.find(key);
        if (it == defs_.end())
            throw ParamNotFound("Unknown parameter: " + key);
        return it->second;
    }

    const Value &value_or_throw(const std::string &key) const
    {
        def_or_throw(key);
        auto it = values_.find(key);
        if (it == values_.end())
            throw ParamNotFound("Parameter not set (no default and no override): " + key);
        return it->second;
    }

    // exact getters
    bool get_exact_bool(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::Bool)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.b;
    }
    int64_t get_exact_int(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::Int)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.i;
    }
    double get_exact_double(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::Double)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.d;
    }
    std::string get_exact_string(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::String)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.s;
    }

    std::vector<bool> get_exact_list_bool(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::ListBool)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.lb;
    }
    std::vector<int64_t> get_exact_list_int(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::ListInt)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.li;
    }
    std::vector<double> get_exact_list_double(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::ListDouble)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.ld;
    }
    std::vector<std::string> get_exact_list_string(const std::string &k) const
    {
        const auto &v = value_or_throw(k);
        if (v.type != ValueType::ListString)
            throw ParamTypeError("Type mismatch for '" + k + "'");
        return v.ls;
    }

    void set_exact(const std::string &key, const Value &incoming, bool allow_int_to_double)
    {
        const ParamDef &def = def_or_throw(key);

        if (incoming.type == def.type)
        {
            values_[key] = incoming;
            return;
        }

        if (allow_int_to_double && def.type == ValueType::Double && incoming.type == ValueType::Int)
        {
            values_[key] = Value::make_double(static_cast<double>(incoming.i));
            return;
        }

        throw ParamTypeError("Type mismatch for '" + key + "': schema is " +
                             std::string(to_string(def.type)) + " but assignment is " +
                             to_string(incoming.type));
    }

private:
    std::unordered_map<std::string, ParamDef> defs_;
    std::unordered_map<std::string, Value> values_;

    YAML::Node original_root_;

    bool yaml_loaded_ = false;
    std::string file_path_;
    Options options_;

    std::string app_name_;
    std::string app_description_;
};

// -----------------------------
// ParamRef impl
// -----------------------------
inline ParamRef::operator bool() const { return owner_->get_bool(key_); }
inline ParamRef::operator int64_t() const { return owner_->get_int(key_); }
inline ParamRef::operator double() const { return owner_->get_double(key_); }
inline ParamRef::operator std::string() const { return owner_->get_string(key_); }

inline ParamRef::operator std::vector<bool>() const { return owner_->get_list_bool(key_); }
inline ParamRef::operator std::vector<int64_t>() const { return owner_->get_list_int(key_); }
inline ParamRef::operator std::vector<double>() const { return owner_->get_list_double(key_); }
inline ParamRef::operator std::vector<std::string>() const { return owner_->get_list_string(key_); }

inline ParamRef &ParamRef::operator=(bool v)
{
    owner_->set_bool(key_, v);
    return *this;
}
inline ParamRef &ParamRef::operator=(int64_t v)
{
    owner_->set_int(key_, v);
    return *this;
}
inline ParamRef &ParamRef::operator=(double v)
{
    owner_->set_double(key_, v);
    return *this;
}
inline ParamRef &ParamRef::operator=(std::string v)
{
    owner_->set_string(key_, std::move(v));
    return *this;
}

inline ParamRef &ParamRef::operator=(std::vector<bool> v)
{
    owner_->set_list_bool(key_, std::move(v));
    return *this;
}
inline ParamRef &ParamRef::operator=(std::vector<int64_t> v)
{
    owner_->set_list_int(key_, std::move(v));
    return *this;
}
inline ParamRef &ParamRef::operator=(std::vector<double> v)
{
    owner_->set_list_double(key_, std::move(v));
    return *this;
}
inline ParamRef &ParamRef::operator=(std::vector<std::string> v)
{
    owner_->set_list_string(key_, std::move(v));
    return *this;
}

} // namespace paramify
