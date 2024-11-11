#include "json.h"

namespace json {

class StartArrayItemContext;
class ValueInDictItemContext;
class KeyItemContext;
class DictItemContext;


class Builder {
    private:
    Node root_;
    std::vector<Node*> nodes_stack_;

    std::string key_;
    bool key_entered_ = false;

    public:
    Builder() {
        nodes_stack_.emplace_back(&root_);
    }
    ~Builder() = default;

    KeyItemContext Key(std::string key);
    Builder& Value(Node::NodeValue v);
    StartArrayItemContext StartArray();
    Builder& EndArray();
    DictItemContext StartDict();
    Builder& EndDict();
    json::Node Build();
};

class DictItemContext {
public:
    DictItemContext(Builder& bldr) : builder(bldr) {}
    ~DictItemContext() = default;

    KeyItemContext Key(std::string key);
    Builder& EndDict() {
        return builder.EndDict();
    }
private:
    Builder& builder;
};

class StartArrayItemContext {
public:
    StartArrayItemContext(Builder& bldr) : builder(bldr) {}
    ~StartArrayItemContext() = default;

    StartArrayItemContext Value(Node::NodeValue v) {
        return builder.Value(std::move(v));
    }
    DictItemContext StartDict() {
        return builder.StartDict();
    }
    StartArrayItemContext StartArray() {
        return builder.StartArray();
    }
    Builder& EndArray() {
        return builder.EndArray();
    }
private:
    Builder& builder;
};

class KeyItemContext {
public:
    KeyItemContext(Builder& bldr) : builder(bldr) {}
    ~KeyItemContext() = default;

    ValueInDictItemContext Value(Node::NodeValue v);
    DictItemContext StartDict() {
        return builder.StartDict();
    }
    StartArrayItemContext StartArray() {
        return builder.StartArray();
    }

private:
    Builder& builder;
};

class ValueInDictItemContext {
public:
    ValueInDictItemContext(Builder& bldr) : builder(bldr) {}
    ~ValueInDictItemContext() = default;

    KeyItemContext Key(std::string key) {
        return builder.Key(std::move(key));
    }

    Builder& EndDict() {
        return builder.EndDict();
    }
private:
    Builder& builder;
};


} // json namespace end