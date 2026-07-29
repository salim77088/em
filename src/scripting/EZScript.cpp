#include "EZScript.h"
#include "../core/Logger.h"
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace nexus::scripting {

// ============ Value ============
Value Value::makeObject() {
    Value x;
    x.type = Type::Object;
    x.obj = std::make_shared<ValueObject>();
    return x;
}

std::string Value::toString() const {
    switch (type) {
        case Type::Null: return "null";
        case Type::Bool: return b ? "true" : "false";
        case Type::Number: {
            if (num == (long long)num) return std::to_string((long long)num);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", num);
            return buf;
        }
        case Type::String: return str;
        case Type::Array: {
            std::string s = "[";
            if (arr) for (size_t i = 0; i < arr->size(); ++i) {
                if (i) s += ", ";
                s += (*arr)[i].toString();
            }
            s += "]";
            return s;
        }
        case Type::Object: {
            std::string s = "{";
            if (obj) {
                bool first = true;
                for (auto& kv : obj->fields) {
                    if (!first) s += ", ";
                    first = false;
                    s += kv.first + ": " + kv.second.toString();
                }
            }
            s += "}";
            return s;
        }
        case Type::Function: return "<func>";
    }
    return "";
}

// ============ Lexer ============
Lexer::Lexer(const std::string& src) : m_src(src) {}

char Lexer::peek(int off) const {
    size_t i = m_pos + off;
    return i < m_src.size() ? m_src[i] : '\0';
}
char Lexer::advance() {
    char c = m_src[m_pos++];
    if (c == '\n') m_line++;
    return c;
}
bool Lexer::atEnd() const { return m_pos >= m_src.size(); }

Token Lexer::makeToken(TokenType t, const std::string& txt) {
    Token tok;
    tok.type = t;
    tok.text = txt;
    tok.line = m_line;
    return tok;
}

Token Lexer::ident() {
    size_t start = m_pos;
    while (!atEnd() && (isalnum(peek()) || peek() == '_')) advance();
    std::string text = m_src.substr(start, m_pos - start);
    if (text == "var") return makeToken(TokenType::Var, text);
    if (text == "let") return makeToken(TokenType::Let, text);
    if (text == "const") return makeToken(TokenType::Const, text);
    if (text == "func") return makeToken(TokenType::Func, text);
    if (text == "return") return makeToken(TokenType::Return, text);
    if (text == "if") return makeToken(TokenType::If, text);
    if (text == "else") return makeToken(TokenType::Else, text);
    if (text == "elif") return makeToken(TokenType::Elif, text);
    if (text == "while") return makeToken(TokenType::While, text);
    if (text == "for") return makeToken(TokenType::For, text);
    if (text == "true") return makeToken(TokenType::True, text);
    if (text == "false") return makeToken(TokenType::False, text);
    if (text == "null") return makeToken(TokenType::Null, text);
    if (text == "and") return makeToken(TokenType::And, text);
    if (text == "or") return makeToken(TokenType::Or, text);
    if (text == "not") return makeToken(TokenType::Not, text);
    if (text == "print") return makeToken(TokenType::Print, text);
    if (text == "break") return makeToken(TokenType::Break, text);
    if (text == "continue") return makeToken(TokenType::Continue, text);
    if (text == "in") return makeToken(TokenType::In, text);
    return makeToken(TokenType::Ident, text);
}

Token Lexer::number() {
    size_t start = m_pos;
    while (!atEnd() && isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peek(1))) { advance(); while (!atEnd() && isdigit(peek())) advance(); }
    std::string text = m_src.substr(start, m_pos - start);
    Token t = makeToken(TokenType::Number, text);
    t.num = std::stod(text);
    return t;
}

Token Lexer::string() {
    char quote = advance();
    std::string value;
    while (!atEnd() && peek() != quote) {
        char c = advance();
        if (c == '\\') {
            char e = advance();
            switch (e) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += e; break;
            }
        } else value += c;
    }
    if (!atEnd()) advance();
    return makeToken(TokenType::String, value);
}

Token Lexer::op() {
    char c = advance();
    switch (c) {
        case '+':
            if (peek() == '=') { advance(); return makeToken(TokenType::PlusEq); }
            return makeToken(TokenType::Plus);
        case '-':
            if (peek() == '=') { advance(); return makeToken(TokenType::MinusEq); }
            if (peek() == '>') { advance(); return makeToken(TokenType::Arrow); }
            return makeToken(TokenType::Minus);
        case '*':
            if (peek() == '=') { advance(); return makeToken(TokenType::StarEq); }
            return makeToken(TokenType::Star);
        case '/':
            if (peek() == '/') { while (!atEnd() && peek() != '\n') advance(); return makeToken(TokenType::Newline); }
            if (peek() == '*') { advance(); while (!atEnd() && !(peek() == '*' && peek(1) == '/')) advance(); if (!atEnd()) { advance(); advance(); } return makeToken(TokenType::Newline); }
            if (peek() == '=') { advance(); return makeToken(TokenType::SlashEq); }
            return makeToken(TokenType::Slash);
        case '%':
            return makeToken(TokenType::Percent);
        case '=':
            if (peek() == '=') { advance(); return makeToken(TokenType::Eq); }
            return makeToken(TokenType::Assign);
        case '!':
            if (peek() == '=') { advance(); return makeToken(TokenType::NotEq); }
            return makeToken(TokenType::Not);
        case '<':
            if (peek() == '=') { advance(); return makeToken(TokenType::LtEq); }
            return makeToken(TokenType::Lt);
        case '>':
            if (peek() == '=') { advance(); return makeToken(TokenType::GtEq); }
            return makeToken(TokenType::Gt);
        case '(': return makeToken(TokenType::LParen);
        case ')': return makeToken(TokenType::RParen);
        case '{': return makeToken(TokenType::LBrace);
        case '}': return makeToken(TokenType::RBrace);
        case '[': return makeToken(TokenType::LBracket);
        case ']': return makeToken(TokenType::RBracket);
        case ',': return makeToken(TokenType::Comma);
        case '.': return makeToken(TokenType::Dot);
        case ':': return makeToken(TokenType::Colon);
        case ';': return makeToken(TokenType::Semicolon);
        case '\n': return makeToken(TokenType::Newline);
        case ' ': case '\t': case '\r': return makeToken(TokenType::Newline);
        default: return makeToken(TokenType::Error, std::string(1, c));
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!atEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') { advance(); continue; }
        if (c == '\n') { tokens.push_back(makeToken(TokenType::Newline)); advance(); continue; }
        if (isalpha(c) || c == '_') { tokens.push_back(ident()); continue; }
        if (isdigit(c)) { tokens.push_back(number()); continue; }
        if (c == '"' || c == '\'') { tokens.push_back(string()); continue; }
        tokens.push_back(op());
    }
    tokens.push_back(makeToken(TokenType::End));
    return tokens;
}

// ============ Parser ============
Parser::Parser(const std::vector<Token>& tokens) : m_tokens(tokens) {}

const Token& Parser::peek(int off) const {
    size_t i = m_pos + off;
    return i < m_tokens.size() ? m_tokens[i] : m_tokens.back();
}
const Token& Parser::advance() {
    const Token& t = m_tokens[m_pos];
    if (t.type != TokenType::End) m_pos++;
    return t;
}
bool Parser::check(TokenType t) const { return peek().type == t; }
bool Parser::match(TokenType t) {
    if (peek().type == t) { advance(); return true; }
    return false;
}
void Parser::error(const std::string& msg) {
    m_errors.push_back("Line " + std::to_string(peek().line) + ": " + msg);
}
void Parser::emit(Op op, int a, int b, int c) {
    Instruction i; i.op = op; i.a = a; i.b = b; i.c = c;
    m_fn.code.push_back(i);
}
int Parser::addConst(const Value& v) {
    Instruction i; i.op = Op::Const; i.value = v;
    m_fn.code.push_back(i);
    return (int)m_fn.code.size() - 1;
}
int Parser::addLocal(const std::string& name) {
    for (size_t i = 0; i < m_locals.size(); ++i) {
        if (m_locals[i].first == name) return (int)i;
    }
    m_locals.push_back({name, false});
    m_fn.localCount++;
    return (int)m_locals.size() - 1;
}
int Parser::findLocal(const std::string& name) {
    for (size_t i = 0; i < m_locals.size(); ++i) {
        if (m_locals[i].first == name) return (int)i;
    }
    return -1;
}

Function Parser::parseProgram() {
    while (!check(TokenType::End)) {
        if (check(TokenType::Newline)) { advance(); continue; }
        parseStmt();
    }
    emit(Op::Return);
    return m_fn;
}

void Parser::parseBlock() {
    while (!check(TokenType::RBrace) && !check(TokenType::End)) {
        if (check(TokenType::Newline)) { advance(); continue; }
        parseStmt();
    }
    match(TokenType::RBrace);
}

void Parser::parseStmt() {
    switch (peek().type) {
        case TokenType::Var:
        case TokenType::Let:
        case TokenType::Const: parseVarDecl(peek().type == TokenType::Const); return;
        case TokenType::Func: parseFuncDecl(); return;
        case TokenType::If: parseIf(); return;
        case TokenType::While: parseWhile(); return;
        case TokenType::For: parseFor(); return;
        case TokenType::Return: parseReturn(); return;
        case TokenType::Print: parsePrint(); return;
        case TokenType::Break: advance(); emit(Op::Const, 0); emit(Op::Return); return;
        case TokenType::Continue: advance(); emit(Op::Const, 0); emit(Op::Return); return;
        default: parseExprStmt(); return;
    }
}

void Parser::parseVarDecl(bool isConst) {
    advance();
    if (!check(TokenType::Ident)) { error("expected name"); return; }
    std::string name = advance().text;
    if (match(TokenType::Assign)) {
        parseExpr();
    } else {
        emit(Op::Const, 0); // null
    }
    int local = addLocal(name);
    m_locals[local].second = isConst;
    emit(Op::Store, local);
    match(TokenType::Semicolon) || match(TokenType::Newline);
}

void Parser::parseFuncDecl() {
    advance();
    if (!check(TokenType::Ident)) { error("expected func name"); return; }
    std::string name = advance().text;
    match(TokenType::LParen);
    std::vector<std::string> params;
    while (!check(TokenType::RParen) && !check(TokenType::End)) {
        if (!check(TokenType::Ident)) { error("expected param"); break; }
        params.push_back(advance().text);
        if (!match(TokenType::Comma)) break;
    }
    match(TokenType::RParen);

    Function savedFn = std::move(m_fn);
    std::vector<std::pair<std::string, bool>> savedLocals = std::move(m_locals);
    m_fn = Function{};
    m_fn.name = name;
    m_fn.paramCount = (int)params.size();
    m_locals.clear();
    for (auto& p : params) addLocal(p);

    match(TokenType::LBrace);
    parseBlock();
    emit(Op::Const, 0); // default null return
    emit(Op::Return);

    Function fn = std::move(m_fn);
    m_fn = std::move(savedFn);
    m_locals = std::move(savedLocals);

    Value v; v.type = Value::Type::Function; v.funcId = (int)m_fn.code.size();
    Instruction i; i.op = Op::Const; i.value = v;
    // store as global
    emit(Op::SetGlobal, 0); // hack: index 0 for now; the value is the function
    (void)fn;
}

void Parser::parseIf() {
    advance();
    match(TokenType::LParen);
    parseExpr();
    match(TokenType::RParen);
    int jmpFalse = (int)m_fn.code.size();
    emit(Op::JmpIfFalse, 0);
    match(TokenType::LBrace);
    parseBlock();
    // elif/else
    if (check(TokenType::Elif)) { parseIf(); return; }
    if (match(TokenType::Else)) {
        int jmpEnd = (int)m_fn.code.size();
        emit(Op::Jmp, 0);
        m_fn.code[jmpFalse].a = (int)m_fn.code.size();
        if (match(TokenType::LBrace)) parseBlock();
        else parseStmt();
        m_fn.code[jmpEnd].a = (int)m_fn.code.size();
    } else {
        m_fn.code[jmpFalse].a = (int)m_fn.code.size();
    }
}

void Parser::parseWhile() {
    advance();
    match(TokenType::LParen);
    int loopStart = (int)m_fn.code.size();
    parseExpr();
    match(TokenType::RParen);
    int jmpFalse = (int)m_fn.code.size();
    emit(Op::JmpIfFalse, 0);
    match(TokenType::LBrace);
    parseBlock();
    emit(Op::Jmp, loopStart);
    m_fn.code[jmpFalse].a = (int)m_fn.code.size();
}

void Parser::parseFor() {
    // for var in iterable { ... }
    advance();
    if (!check(TokenType::Ident)) { error("expected loop var"); return; }
    std::string varName = advance().text;
    if (!match(TokenType::In)) { error("expected 'in'"); return; }
    parseExpr();
    int jmpFalse = (int)m_fn.code.size();
    emit(Op::JmpIfFalse, 0);
    int local = addLocal(varName);
    emit(Op::Store, local);
    match(TokenType::LBrace);
    parseBlock();
    m_fn.code[jmpFalse].a = (int)m_fn.code.size();
}

void Parser::parseReturn() {
    advance();
    if (check(TokenType::Newline) || check(TokenType::Semicolon) || check(TokenType::RBrace)) {
        emit(Op::Const, 0);
    } else {
        parseExpr();
    }
    emit(Op::Return);
    match(TokenType::Semicolon) || match(TokenType::Newline);
}

void Parser::parsePrint() {
    advance();
    if (match(TokenType::LParen)) {
        parseExpr();
        while (match(TokenType::Comma)) { parseExpr(); emit(Op::Print); }
        match(TokenType::RParen);
    } else {
        parseExpr();
    }
    emit(Op::Print);
    match(TokenType::Semicolon) || match(TokenType::Newline);
}

void Parser::parseExprStmt() {
    parseExpr();
    match(TokenType::Semicolon) || match(TokenType::Newline);
}

void Parser::parseExpr() {
    parseAssign();
}

void Parser::parseAssign() {
    parseOr();
    if (check(TokenType::Assign)) {
        advance();
        parseExpr();
        emit(Op::Store, 0); // top of stack
    }
}

void Parser::parseOr() {
    parseAnd();
    while (match(TokenType::Or)) { parseAnd(); emit(Op::Or); }
}

void Parser::parseAnd() {
    parseEquality();
    while (match(TokenType::And)) { parseEquality(); emit(Op::And); }
}

void Parser::parseEquality() {
    parseComparison();
    while (check(TokenType::Eq) || check(TokenType::NotEq)) {
        TokenType t = advance().type;
        parseComparison();
        emit(t == TokenType::Eq ? Op::Eq : Op::NotEq);
    }
}

void Parser::parseComparison() {
    parseTerm();
    while (check(TokenType::Lt) || check(TokenType::Gt) || check(TokenType::LtEq) || check(TokenType::GtEq)) {
        TokenType t = advance().type;
        parseTerm();
        Op op = Op::Lt;
        if (t == TokenType::Lt) op = Op::Lt;
        else if (t == TokenType::Gt) op = Op::Gt;
        else if (t == TokenType::LtEq) op = Op::LtEq;
        else op = Op::GtEq;
        emit(op);
    }
}

void Parser::parseTerm() {
    parseFactor();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        TokenType t = advance().type;
        parseFactor();
        emit(t == TokenType::Plus ? Op::Add : Op::Sub);
    }
}

void Parser::parseFactor() {
    parseUnary();
    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        TokenType t = advance().type;
        parseUnary();
        if (t == TokenType::Star) emit(Op::Mul);
        else if (t == TokenType::Slash) emit(Op::Div);
        else emit(Op::Mod);
    }
}

void Parser::parseUnary() {
    if (check(TokenType::Minus)) { advance(); parseUnary(); emit(Op::Neg); return; }
    if (check(TokenType::Not)) { advance(); parseUnary(); emit(Op::Not); return; }
    parsePostfix();
}

void Parser::parsePostfix() {
    parsePrimary();
    while (true) {
        if (check(TokenType::LParen)) {
            advance();
            int argc = 0;
            if (!check(TokenType::RParen)) {
                parseExpr(); argc++;
                while (match(TokenType::Comma)) { parseExpr(); argc++; }
            }
            match(TokenType::RParen);
            emit(Op::Call, argc);
        } else if (match(TokenType::Dot)) {
            if (!check(TokenType::Ident)) { error("expected field"); break; }
            std::string name = advance().text;
            Value v = Value::makeStr(name);
            Instruction i; i.op = Op::Const; i.value = v;
            m_fn.code.push_back(i);
            emit(Op::LoadField);
        } else if (match(TokenType::LBracket)) {
            parseExpr();
            match(TokenType::RBracket);
            emit(Op::LoadIndex);
        } else break;
    }
}

void Parser::parsePrimary() {
    const Token& t = peek();
    switch (t.type) {
        case TokenType::Number: advance(); { Value v = Value::makeNum(t.num); Instruction i; i.op = Op::Const; i.value = v; m_fn.code.push_back(i); } return;
        case TokenType::String: advance(); { Value v = Value::makeStr(t.text); Instruction i; i.op = Op::Const; i.value = v; m_fn.code.push_back(i); } return;
        case TokenType::True: advance(); { Value v = Value::makeBool(true); Instruction i; i.op = Op::Const; i.value = v; m_fn.code.push_back(i); } return;
        case TokenType::False: advance(); { Value v = Value::makeBool(false); Instruction i; i.op = Op::Const; i.value = v; m_fn.code.push_back(i); } return;
        case TokenType::Null: advance(); { Instruction i; i.op = Op::Const; i.value = Value::makeNull(); m_fn.code.push_back(i); } return;
        case TokenType::Ident: {
            advance();
            int local = findLocal(t.text);
            if (local >= 0) emit(Op::Load, local);
            else { emit(Op::GetGlobal, 0); }
            return;
        }
        case TokenType::LParen: advance(); parseExpr(); match(TokenType::RParen); return;
        case TokenType::LBracket: {
            advance();
            emit(Op::MakeArray);
            int count = 0;
            while (!check(TokenType::RBracket) && !check(TokenType::End)) {
                parseExpr();
                emit(Op::StoreIndex, count, 1);
                count++;
                if (!match(TokenType::Comma)) break;
            }
            match(TokenType::RBracket);
            return;
        }
        case TokenType::LBrace: {
            advance();
            emit(Op::MakeObject);
            while (!check(TokenType::RBrace) && !check(TokenType::End)) {
                if (!check(TokenType::Ident) && !check(TokenType::String)) { error("expected key"); break; }
                std::string key = advance().text;
                match(TokenType::Colon);
                parseExpr();
                Instruction i; i.op = Op::Const; i.value = Value::makeStr(key);
                m_fn.code.push_back(i);
                emit(Op::StoreField);
                if (!match(TokenType::Comma)) break;
            }
            match(TokenType::RBrace);
            return;
        }
        default:
            error("unexpected token: " + t.text);
            advance();
            return;
    }
}

// ============ VM ============
VM::VM() {
    registerNative("len", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::makeNum(0);
        auto& a = args[0];
        if (a.type == Value::Type::String) return Value::makeNum((double)a.str.size());
        if (a.type == Value::Type::Array && a.arr) return Value::makeNum((double)a.arr->size());
        return Value::makeNum(0);
    });
    registerNative("sqrt", [](const std::vector<Value>& args) -> Value {
        if (args.empty() || args[0].type != Value::Type::Number) return Value::makeNum(0);
        return Value::makeNum(std::sqrt(args[0].num));
    });
    registerNative("sin", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::makeNum(0);
        return Value::makeNum(std::sin(args[0].num));
    });
    registerNative("cos", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::makeNum(0);
        return Value::makeNum(std::cos(args[0].num));
    });
    registerNative("abs", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::makeNum(0);
        return Value::makeNum(std::fabs(args[0].num));
    });
    registerNative("toStr", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::makeStr("null");
        return Value::makeStr(args[0].toString());
    });
}

void VM::registerNative(const std::string& name, NativeFn fn) {
    if (m_natives.find(name) == m_natives.end()) {
        m_natives[name] = (int)m_nativeFns.size();
        m_nativeFns.push_back(fn);
    } else {
        m_nativeFns[m_natives[name]] = fn;
    }
}

void VM::setGlobal(const std::string& name, const Value& v) { m_globals[name] = v; }
Value VM::getGlobal(const std::string& name) {
    auto it = m_globals.find(name);
    return it != m_globals.end() ? it->second : Value::makeNull();
}

void VM::error(const std::string& msg) { m_errors.push_back(msg); }

bool VM::run(const Function& fn) {
    m_errors.clear();
    std::vector<Value> stack;
    bool ok = exec(fn, stack);
    return ok;
}

bool VM::exec(const Function& fn, std::vector<Value>& stack) {
    std::vector<Value> locals(fn.localCount + 16);
    size_t ip = 0;
    m_instructionCount = 0;
    while (ip < fn.code.size()) {
        if (++m_instructionCount > 100000000) {
            error("Instruction limit exceeded");
            return false;
        }
        const Instruction& ins = fn.code[ip++];
        switch (ins.op) {
            case Op::Nop: break;
            case Op::Const:
                stack.push_back(ins.value); break;
            case Op::Load:
                if (ins.a < (int)locals.size()) stack.push_back(locals[ins.a]);
                else stack.push_back(Value::makeNull());
                break;
            case Op::Store:
                if (ins.a < (int)locals.size()) locals[ins.a] = stack.back();
                stack.pop_back();
                break;
            case Op::GetGlobal:
                stack.push_back(Value::makeNull());
                break;
            case Op::SetGlobal:
                if (!stack.empty()) stack.pop_back();
                break;
            case Op::Add: {
                if (stack.size() < 2) break;
                Value b = stack.back(); stack.pop_back();
                Value a = stack.back(); stack.pop_back();
                if (a.type == Value::Type::Number && b.type == Value::Type::Number)
                    stack.push_back(Value::makeNum(a.num + b.num));
                else
                    stack.push_back(Value::makeStr(a.toString() + b.toString()));
                break;
            }
            case Op::Sub: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeNum(a.num - b.num)); break; }
            case Op::Mul: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeNum(a.num * b.num)); break; }
            case Op::Div: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); if (b.num == 0) stack.push_back(Value::makeNull()); else stack.push_back(Value::makeNum(a.num / b.num)); break; }
            case Op::Mod: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeNum(std::fmod(a.num, b.num))); break; }
            case Op::Neg: { Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeNum(-a.num)); break; }
            case Op::Not: { Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(!a.isTruthy())); break; }
            case Op::Eq: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); bool eq = (a.toString() == b.toString()) && (a.type == b.type); stack.push_back(Value::makeBool(eq)); break; }
            case Op::NotEq: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); bool eq = (a.toString() == b.toString()) && (a.type == b.type); stack.push_back(Value::makeBool(!eq)); break; }
            case Op::Lt: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(a.num < b.num)); break; }
            case Op::Gt: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(a.num > b.num)); break; }
            case Op::LtEq: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(a.num <= b.num)); break; }
            case Op::GtEq: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(a.num >= b.num)); break; }
            case Op::And: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(a.isTruthy() && b.isTruthy())); break; }
            case Op::Or: { Value b = stack.back(); stack.pop_back(); Value a = stack.back(); stack.pop_back(); stack.push_back(Value::makeBool(a.isTruthy() || b.isTruthy())); break; }
            case Op::Jmp: ip = (size_t)ins.a; break;
            case Op::JmpIfFalse: {
                if (!stack.empty()) {
                    Value v = stack.back(); stack.pop_back();
                    if (!v.isTruthy()) ip = (size_t)ins.a;
                }
                break;
            }
            case Op::Call: {
                int argc = ins.a;
                if ((int)stack.size() < argc + 1) break;
                std::vector<Value> args(argc);
                for (int i = argc - 1; i >= 0; --i) { args[i] = stack.back(); stack.pop_back(); }
                Value fnVal = stack.back(); stack.pop_back();
                if (fnVal.type == Value::Type::Function && fnVal.funcId >= 0 && fnVal.funcId < (int)m_nativeFns.size()) {
                    stack.push_back(m_nativeFns[fnVal.funcId](args));
                } else {
                    stack.push_back(Value::makeNull());
                }
                break;
            }
            case Op::Return: {
                if (!stack.empty()) {
                    Value v = stack.back();
                    (void)v;
                }
                return true;
            }
            case Op::Pop: if (!stack.empty()) stack.pop_back(); break;
            case Op::MakeArray: stack.push_back(Value::makeArray()); break;
            case Op::MakeObject: stack.push_back(Value::makeObject()); break;
            case Op::LoadField: {
                Value name = stack.back(); stack.pop_back();
                Value obj = stack.back(); stack.pop_back();
                if (obj.type == Value::Type::Object && obj.obj) {
                    auto it = obj.obj->fields.find(name.str);
                    stack.push_back(it != obj.obj->fields.end() ? it->second : Value::makeNull());
                } else stack.push_back(Value::makeNull());
                break;
            }
            case Op::StoreField: {
                Value name = stack.back(); stack.pop_back();
                Value v = stack.back(); stack.pop_back();
                Value obj = stack.back(); stack.pop_back();
                if (obj.type != Value::Type::Object) {
                    obj = Value::makeObject();
                }
                obj.obj->fields[name.str] = v;
                stack.push_back(obj);
                break;
            }
            case Op::LoadIndex: {
                Value idx = stack.back(); stack.pop_back();
                Value arr = stack.back(); stack.pop_back();
                if (arr.type == Value::Type::Array && arr.arr && idx.type == Value::Type::Number) {
                    size_t i = (size_t)idx.num;
                    if (i < arr.arr->size()) stack.push_back((*arr.arr)[i]);
                    else stack.push_back(Value::makeNull());
                } else if (arr.type == Value::Type::String && idx.type == Value::Type::Number) {
                    size_t i = (size_t)idx.num;
                    if (i < arr.str.size()) stack.push_back(Value::makeStr(std::string(1, arr.str[i])));
                    else stack.push_back(Value::makeNull());
                } else stack.push_back(Value::makeNull());
                break;
            }
            case Op::StoreIndex: {
                (void)ins;
                if (stack.size() < 2) break;
                Value v = stack.back(); stack.pop_back();
                Value arr = stack.back(); stack.pop_back();
                if (arr.type == Value::Type::Array && arr.arr) arr.arr->push_back(v);
                stack.push_back(arr);
                break;
            }
            case Op::Print: {
                if (!stack.empty()) {
                    std::string s = stack.back().toString();
                    stack.pop_back();
                    m_output.push_back(s);
                    NX_INFO("EZScript", "%s", s.c_str());
                }
                break;
            }
        }
    }
    return true;
}

} // namespace nexus::scripting
