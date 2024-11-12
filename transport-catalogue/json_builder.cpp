#include "json_builder.h"

namespace json {

    Builder::ValueInDictItemContext Builder::KeyItemContext::Value(Node::NodeValue v) {
        return ValueInDictItemContext(builder.Value(std::move(v)));
    }

    Builder::KeyItemContext Builder::DictItemContext::Key(std::string key) {
        return builder.Key(std::move(key));
    }

    Builder::KeyItemContext json::Builder::Key(std::string key) {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Function call in ended builder");
        }
        if (nodes_stack_.back()->IsMap() && !key_entered_) {
            key_ = key;
            std::get<Dict>(nodes_stack_.back()->GetPureValue())[std::move(key)];
            key_entered_ = true;
        } else {
            throw std::logic_error("Call key only in dict");
        }
        return KeyItemContext(*this);
    }

    Builder& json::Builder::Value(Node::NodeValue v) {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Function call in ended builder");
        }
        if (nodes_stack_.size() == 1 && nodes_stack_.back()->IsNull()) {
            nodes_stack_.back()->GetPureValue() = v;
            nodes_stack_.pop_back();
        } else if (nodes_stack_.back()->IsArray()) {
            std::get<Array>(nodes_stack_.back()->GetPureValue()).emplace_back(v);
        } else if (nodes_stack_.back()->IsMap()) {
            if (!key_entered_) {
                throw std::logic_error("Value in Dict need Key");
            }
            std::get<Dict>(nodes_stack_.back()->GetPureValue())[key_] = v;
            key_.clear();
            key_entered_ = false;
        } else {
            throw std::logic_error("Incorrect value call error");
        }
        return *this;
    }

    Builder::StartArrayItemContext json::Builder::StartArray() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Function call in ended builder");
        }
        if (nodes_stack_.back()->IsMap() && !key_entered_) {
            throw std::logic_error("Expected key");
        }
        if (nodes_stack_.back()->IsNull()) {
            nodes_stack_.back()->GetPureValue() = Array();
        } else if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.emplace_back(&std::get<Array>(nodes_stack_.back()->GetPureValue()).emplace_back(Array()));
        } else if (nodes_stack_.back()->IsMap() && key_entered_) {
            std::get<Dict>(nodes_stack_.back()->GetPureValue())[key_] = Array();
            nodes_stack_.emplace_back(&std::get<Dict>(nodes_stack_.back()->GetPureValue()).at(key_));
            key_.clear();
            key_entered_ = false;
        } else {
            throw std::logic_error("StartArray Failed");
        }
        return StartArrayItemContext(*this);
    }

    Builder& json::Builder::EndArray() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Function call in ended builder");
        }
        if (!nodes_stack_.back()->IsArray()) {
            throw std::logic_error("EndArray called not for Array");
        }
        nodes_stack_.pop_back();
        return *this;
    }

    Builder::DictItemContext json::Builder::StartDict() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Function call in ended builder");
        }
        if (nodes_stack_.back()->IsMap() && !key_entered_) {
            throw std::logic_error("Expected key");
        }
        if (nodes_stack_.back()->IsNull()) {
            Dict dict;
            nodes_stack_.back()->GetPureValue() = Dict();
        } else if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.emplace_back(&std::get<Array>(nodes_stack_.back()->GetPureValue()).emplace_back(Dict()));
        } else if (nodes_stack_.back()->IsMap() && key_entered_) {
            std::get<Dict>(nodes_stack_.back()->GetPureValue())[key_] = Dict();
            nodes_stack_.emplace_back(&std::get<Dict>(nodes_stack_.back()->GetPureValue()).at(key_));
            key_.clear();
            key_entered_ = false;
        } else {
            throw std::logic_error("StartDict Failed");
        }
        return DictItemContext(*this);
    }

    Builder& json::Builder::EndDict() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Function call in ended builder");
        }
        if (!nodes_stack_.back()->IsMap()) {
            throw std::logic_error("EndDict called not for dict");
        }
        if (key_entered_) {
            throw std::logic_error("Key don't have value");
        }
        nodes_stack_.pop_back();
        return *this;
    }

    json::Node json::Builder::Build() {
        if (!root_.IsNull() && nodes_stack_.empty()) {
            return root_;
        }
        throw std::logic_error("Building not ended");
    }

} // namespace json end