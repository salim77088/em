#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>
#include <cstdint>

namespace nexus::scripting {

enum class TokenType {
    End, Newline, Ident, Number, String,
    // Keywords
    Var, Let, Const, Func, Return, If, Else, Elif, While, For,
    True, False, Null, And, Or, Not, Print, Break, Continue, In,
    // Operators
    Assign, Plus, Minus, Star, Slash, Percent,
    PlusEq, MinusEq, StarEq, SlashEq,
    Eq, NotEq, Lt, Gt, LtEq, GtEq,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Dot, Colon, Semicolon, Arrow,
    // special
    Error
};

struct Token {
    TokenType type = TokenType::End;
    std::string text;
    double num = 0.0;
    int line = 0;
};

struct Value;
using ValueArray = std::vector<Value>;
struct ValueObject;

struct Value {
    enum class Type { Null, Bool, Number, String, Array, Object, Function } type = Type::Null;
    bool b = false;
    double num = 0.0;
    std::string str;
    std::shared_ptr<ValueArray> arr;
    std::shared_ptr<ValueObject> obj;
    int funcId = -1;  // for native/script funcs

    static Value makeNull() { return Value{}; }
    static Value makeBool(bool v) { Value x; x.type = Type::Bool; x.b = v; return x; }
    static Value makeNum(double v) { Value x; x.type = Type::Number; x.num = v; return x; }
    static Value makeStr(std::string v) { Value x; x.type = Type::String; x.str = std::move(v); return x; }
    static Value makeArray() { Value x; x.type = Type::Array; x.arr = std::make_shared<ValueArray>(); return x; }
    static Value makeObject();

    bool isTruthy() const {
        if (type == Type::Null) return false;
        if (type == Type::Bool) return b;
        if (type == Type::Number) return num != 0.0;
        if (type == Type::String) return !str.empty();
        if (type == Type::Array) return arr && !arr->empty();
        if (type == Type::Object) return (bool)obj;
        return funcId >= 0;
    }

    std::string toString() const;
};

struct ValueObject {
    std::unordered_map<std::string, Value> fields;
};

class Lexer {
public:
    Lexer(const std::string& src);
    std::vector<Token> tokenize();
private:
    char peek(int off = 0) const;
    char advance();
    bool atEnd() const;
    Token makeToken(TokenType t, const std::string& txt = "");
    Token ident();
    Token number();
    Token string();
    Token op();
    std::string m_src;
    size_t m_pos = 0;
    int m_line = 1;
};

enum class Op {
    Nop, Const, Load, Store, LoadField, StoreField, LoadIndex, StoreIndex,
    Add, Sub, Mul, Div, Mod, Neg, Not,
    Eq, NotEq, Lt, Gt, LtEq, GtEq, And, Or,
    Jmp, JmpIfFalse, Call, Return, Pop, MakeArray, MakeObject,
    Print, GetGlobal, SetGlobal
};

struct Instruction {
    Op op = Op::Nop;
    int a = 0;
    int b = 0;
    int c = 0;
    Value value;
};

struct Function {
    std::string name;
    int paramCount = 0;
    int localCount = 0;
    std::vector<Instruction> code;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    Function parseProgram();
    const std::vector<std::string>& errors() const { return m_errors; }
private:
    const Token& peek(int off = 0) const;
    const Token& advance();
    bool check(TokenType t) const;
    bool match(TokenType t);
    void error(const std::string& msg);
    void emit(Op op, int a = 0, int b = 0, int c = 0);
    int addConst(const Value& v);
    int addLocal(const std::string& name);
    int findLocal(const std::string& name);
    void parseStmt();
    void parseBlock();
    void parseVarDecl(bool isConst);
    void parseFuncDecl();
    void parseIf();
    void parseWhile();
    void parseFor();
    void parseReturn();
    void parsePrint();
    void parseExprStmt();
    void parseExpr();
    void parseAssign();
    void parseOr();
    void parseAnd();
    void parseEquality();
    void parseComparison();
    void parseTerm();
    void parseFactor();
    void parseUnary();
    void parsePostfix();
    void parsePrimary();
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
    Function m_fn;
    std::vector<std::pair<std::string, bool>> m_locals;
    std::vector<std::string> m_errors;
};

class VM {
public:
    using NativeFn = std::function<Value(const std::vector<Value>&)>;

    VM();
    bool run(const Function& fn);
    void registerNative(const std::string& name, NativeFn fn);
    void setGlobal(const std::string& name, const Value& v);
    Value getGlobal(const std::string& name);
    const std::vector<std::string>& errors() const { return m_errors; }
    const std::vector<std::string>& output() const { return m_output; }

private:
    bool exec(const Function& fn, std::vector<Value>& stack);
    void error(const std::string& msg);
    std::unordered_map<std::string, Value> m_globals;
    std::unordered_map<std::string, int> m_natives;
    std::vector<NativeFn> m_nativeFns;
    std::vector<std::string> m_errors;
    std::vector<std::string> m_output;
    size_t m_instructionCount = 0;
};

} // namespace nexus::scripting
