#include "json.h"

using namespace std;

namespace json {

namespace {

using Number = std::variant<int, double>;

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

std::nullptr_t LoadNull(istream& input) {
    string str;
    for (char c; input >> c && c != ',' && c != ']' && c != '}';) {
        str.push_back(c);
    }
    if (str == "ull") {
        return nullptr;
    } else {
        throw ParsingError("incorrect null call " + str);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;
    bool closed = false;
    for (char c; input >> c;) {
        if (c == ']') {
            closed = true;
            break;
        }
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (!closed) {
        throw ParsingError("Incorrect array input");
    }
    return Node(move(result));
}

Node LoadDict(istream& input) {
    Dict result;
    bool closed = false;
    for (char c; input >> c;) {
        if (c == '}') {
            closed = true;
            break;
        }
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input);
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }
    if (!closed) {
        throw ParsingError("Dict not closed");
    }
    return Node(move(result));
}

Node LoadBool(istream& input) {
    string str;
    for (char c; input >> c && c != ',';) {
        if (c == ']' || c == '}') {
            input.putback(c);
            break;
        }
        str.push_back(c);
    }
    if (str == "rue"sv) {
        return true;
    } else if (str == "alse"sv) {
        return false;
    }
    throw ParsingError("incorrect bool input " + str);
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        return LoadBool(input);
    } else {
        input.putback(c);
        auto num = LoadNumber(input);
        if (num.index() == 0) {
            return Node(get<int>(num));
        } else {
            return Node(get<double>(num));
        }
    }
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

// Экранирование
// R"("\r\n\t\"\\")"s - to - "\r\n\t\"\\"
std::string StringTab(const string& str) {
    using namespace std::literals;

    auto it = str.begin();

    std::string s;
    while (it != str.end()) {
        const char ch = *it;
        if (ch == '"') {
            s.push_back('\\');
            s.push_back(ch);
        } else if (ch == '\r') {
            s.push_back('\\');
            s.push_back('r');
        } else if (ch == '\n') {
            s.push_back('\\');
            s.push_back('n');
        } else if (ch == '\t') {
            s.push_back('\\');
            s.push_back('t');
        } else if (ch == '\\') {
            s.push_back('\\');
            s.push_back('\\');
        } else {
            s.push_back(ch);
        }
        ++it;
    }
    return s;
}

void PrintNode(const Node& node, PrintContext& out);

void PrintValue(const int& value, PrintContext& out) {
    out.out << value;
}

void PrintValue(const double& value, PrintContext& out) {
    out.out << value;
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, PrintContext& out) {
    out.out << "null"sv;
}

void PrintValue(const bool b, PrintContext& out) {
    out.out << boolalpha << b;
}

void PrintValue(const std::string& str, PrintContext& out) {
    out.out << '"' << StringTab(str) << '"';
}

void PrintValue(const Array& arr, PrintContext& out) {
    out.out << '[';
    for (auto it = arr.begin() ; it != arr.end() ; ++it) {
        PrintNode(*it, out);
        if (it != (--arr.end())) {
            out.out << ", "s;
        }
    }
    out.out << ']';
}

void PrintValue(const Dict& dict, PrintContext& out) {
    out.out << '{';
    for (auto it = dict.begin() ; it != dict.end() ; ++it) {
        PrintNode(it->first, out);
        out.out << ": ";
        PrintNode(it->second, out);
        if (it != (--dict.end())) {
            out.out << ", "s;
        }
    }
    out.out << '}';
}

void PrintNode(const Node& node, PrintContext& out) {
    std::visit(
        [&out](const auto& value){ PrintValue(value, out); },
        node.GetValue());
}

}  // namespace


const Array& Node::AsArray() const {
    if (!std::holds_alternative<Array>(node_)) { throw std::logic_error{"wrong node type"}; }
    return std::get<Array>(node_);
}

int Node::AsInt() const {
    if (!std::holds_alternative<int>(node_)) { throw std::logic_error{"wrong node type"}; }
    return std::get<int>(node_);
}

const string& Node::AsString() const {
    if (!std::holds_alternative<std::string>(node_)) { throw std::logic_error{"wrong node type"}; }
    return std::get<std::string>(node_);
}

bool Node::AsBool() const {
    if (!std::holds_alternative<bool>(node_)) { throw std::logic_error{"wrong node type"}; }
    return std::get<bool>(node_);
}

double Node::AsDouble() const {
    if (std::holds_alternative<double>(node_)) {
        return std::get<double>(node_);
    } else if (std::holds_alternative<int>(node_)) {
        int i = std::get<int>(node_);
        return i + 0.0;
    } else {
        throw std::logic_error{"wrong node type"};
        return 0.0;
    }
}

const Dict& Node::AsMap() const {
    if (!std::holds_alternative<Dict>(node_)) { throw std::logic_error{"wrong node type"}; }
    return std::get<Dict>(node_);
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintContext pc{output};
    PrintNode(doc.GetRoot(), pc);
}

}  // namespace json