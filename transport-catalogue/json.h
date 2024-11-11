#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <cassert>
#include <sstream>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


class Node {
public:
    friend class Builder;
    using NodeValue = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node(NodeValue value) : node_(std::move(value)) {}

    Node() = default;
    Node(Array node_t) : node_(node_t) {}
    Node(Dict node_t) : node_(node_t) {}
    Node(int node_t) : node_(node_t) {}
    Node(double node_t) : node_(node_t) {}
    Node(std::string node_t) : node_(node_t) {}
    Node(bool node_t) : node_(node_t) {}
    Node(std::nullptr_t null) : node_(null) {}

    const Array& AsArray() const;
    int AsInt() const;
    const std::string& AsString() const;
    bool AsBool() const;
    double AsDouble() const;
    const Dict& AsMap() const;

    size_t NodeIndex() const {
        return node_.index();
    }

    const NodeValue& GetValue() const {
        return node_;
    }

    bool IsInt() const { return std::holds_alternative<int>(node_); }
    bool IsDouble() const {return (std::holds_alternative<int>(node_) || std::holds_alternative<double>(node_));}
    bool IsPureDouble() const {return std::holds_alternative<double>(node_);}
    bool IsBool() const { return std::holds_alternative<bool>(node_); }
    bool IsString() const {return std::holds_alternative<std::string>(node_);}
    bool IsNull() const { return std::holds_alternative<std::nullptr_t>(node_); }
    bool IsArray() const { return std::holds_alternative<Array>(node_); }
    bool IsMap() const { return std::holds_alternative<Dict>(node_); }

    bool operator==(const Node& rhs) const {
        if (node_.index() != rhs.node_.index()) {
            return false;
        }
        return node_ == rhs.node_;
    }

    bool operator!=(const Node& rhs) const {
        return !(*this == rhs);
    }

private:
    NodeValue& GetPureValue() {
        return node_;
    }

    NodeValue node_ = nullptr;
};



class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& rhs) const {
        return root_ == rhs.root_;
    }
    bool operator!=(const Document& rhs) const {
        return !(*this == rhs);
    }

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json