*
* The Parser. Given a list of tokens, convert it into an AST.
*/
#include "lexer.cpp"
#include <cstddef>
#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <optional>

using namespace std;

struct parseError {
    string type;
    string message;
    int line;
};

// === EXPRESSIONS === //
struct NumberLit;
struct Ident;
struct BinaryOp;
struct UnaryOp;

using Expr = variant<NumberLit, Ident, BinaryOp, UnaryOp>;
using ExprPtr = unique_ptr<Expr>;

struct NumberLit { double value; };
struct Ident     { string name; };
struct BinaryOp  { TokenType op; ExprPtr left; ExprPtr right; };
struct UnaryOp {TokenType op; ExprPtr operand;};

// === STATEMENT === //
struct PrintStmt;
struct ExprStmt;
struct AsgnStmt;
// add more later 

using Stmt = variant<PrintStmt, ExprStmt, AsgnStmt>;
using StmtPtr = unique_ptr<Stmt>;

struct PrintStmt {ExprPtr value; };
struct ExprStmt {ExprPtr value; };
struct AsgnStmt {string name; ExprPtr value; };

// ====== Debug Start ====== //
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void printExpr(const ExprPtr& e, int depth = 0) {
    // for debug
    if (!e) { cout << string(depth * 2, ' ') << "null\n"; return; }
    visit(overloaded{
        [&](const NumberLit& n) {
            cout << string(depth * 2, ' ') << "NumberLit(" << n.value << ")\n";
        },
        [&](const Ident& i) {
            cout << string(depth * 2, ' ') << "Ident(" << i.name << ")\n";
        },
        [&](const BinaryOp& b) {
            cout << string(depth * 2, ' ') << "BinaryOp(" << static_cast<int>(b.op) << ")\n";
            printExpr(b.left, depth + 1);
            printExpr(b.right, depth + 1);
        },
        [&](const UnaryOp& u) {
            cout << string(depth * 2, ' ') << "UnaryOp(" << static_cast<int>(u.op) << ")\n";
            printExpr(u.operand, depth + 1);
        }
    }, *e);
}

void printStmt(const StmtPtr& s, int depth = 0) {
    // for debug
    if (!s) { cout << string(depth * 2, ' ') << "null\n"; return; }
    visit(overloaded{
        [&](const PrintStmt& p) {
            cout << string(depth * 2, ' ') << "PrintStmt\n";
            printExpr(p.value, depth + 1);
        },
        [&](const ExprStmt& e) {
            cout << string(depth * 2, ' ') << "ExprStmt\n";
            printExpr(e.value, depth + 1);
        },
        [&](const AsgnStmt& a) {
            cout << string(depth * 2, ' ') << "AsgnStmt(" << a.name << ")\n";
            printExpr(a.value, depth + 1);
        }
    }, *s);
}
// ====== Debug End ====== //


class Parser {
public:
    Parser(vector<Token> tokens) {
        tokens_ = tokens;
    }

    ExprPtr parseExpression() { return expression(); }

    vector<StmtPtr> parse() {
        vector<StmtPtr> result;

        while (peek(position_).type != TokenType::END_OF_FILE) {
            result.push_back(statement());
        }
        return result;
    }

    const vector<parseError>& getErrors() { return errorList; }
private:
    // primitives (mirror the lexer's peek/advance/validPos)
    Token peek(size_t pos){
        return tokens_[pos];
    }
    void advance() {
        position_ += 1;
    }
    bool check(TokenType type) {
         // is the current token this type?
        return peek(position_).type == type;
    }
    bool match(TokenType type){
        if (check(type)) {
            advance();
            return true;
        } 
        return false;
    }
    optional<Token> expect(TokenType type, string errMsg) {
        Token currToken = peek(position_);
        if (match(type)) {
            return currToken;
        } else {
            errorList.push_back(parseError{"MISSING EXPECTED", errMsg, currToken.line});
            return nullopt;
        }
    } 
    StmtPtr identStatement() {
        if (peek(position_ + 1).type == TokenType::EQUALS) {
            return assignStatement();
        } else {
            auto value = expression();
            expect(TokenType::NEWLINE, "Expected Newline");
            return make_unique<Stmt>(ExprStmt{std::move(value)});
        }
    }
    // one method per grammar rule above
    StmtPtr statement(){
        Token currToken = peek(position_);
        TokenType currType = currToken.type;

        if (currType == TokenType::PRINT) {
            return printStatement();
        } else if (currType == TokenType::IDENT) {
            return identStatement();
        } else {
            auto value = expression();
            expect(TokenType::NEWLINE, "Expected Newline");
            return make_unique<Stmt>(ExprStmt{std::move(value)});
        }
    }
    StmtPtr printStatement() {
        expect(TokenType::PRINT, "Expected 'print'");
        expect(TokenType::LPAREN, "Expected '(' after print");
        auto value = expression();
        expect(TokenType::RPAREN, "Expected ')'");
        expect(TokenType::NEWLINE, "Expected Newline");
        return make_unique<Stmt>(PrintStmt{std::move(value)});
    }
    StmtPtr assignStatement() {
        string name = peek(position_).lexeme;
        advance();
        expect(TokenType::EQUALS, "Expected equals");
        auto value = expression();
        expect(TokenType::NEWLINE, "Expected Newline");
        return make_unique<Stmt>(AsgnStmt{name, std::move(value)}); 
    }
    ExprPtr expression(){
        // for continous addition and subtraction
        auto result = term();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            TokenType op = peek(position_).type;
            advance();
            auto right = term();
            result = make_unique<Expr>(BinaryOp{op, std::move(result), std::move(right)});
        }        
        return result;


    }
    ExprPtr term() {
        // for continous multiplication and division
        auto result = factor();
        while (check(TokenType::STAR) || check(TokenType::SLASH)) {
            TokenType op = peek(position_).type;
            advance();
            auto right = factor();
            result = make_unique<Expr>(BinaryOp{op, std::move(result), std::move(right)});
        }        
        return result;
    }
    ExprPtr factor() {
        // for the levels of parenthesis'

        Token currToken = peek(position_);
        TokenType currType = currToken.type;
        if (currType == TokenType::NUMBER){
            advance();
            return make_unique<Expr>(NumberLit{stod(currToken.lexeme)});
        } else if (currType == TokenType::IDENT) {
            advance();
            return make_unique<Expr>(Ident{currToken.lexeme});
        } else if (currType == TokenType::LPAREN) {
            // Decent
            advance();
            auto inner = expression();
            auto result = expect(TokenType::RPAREN, "Missing Right Paren");
            if (result.has_value()) {
                return inner;
            } else {
                // Missing RPAREN
                return nullptr;
            }
        } else if (currType == TokenType::PLUS || currType == TokenType::MINUS){
            advance();
            return make_unique<Expr>(UnaryOp{currType, factor()});
        } else {
            return nullptr;
        }
    }

    vector<Token> tokens_;
    size_t position_ = 0;
    vector<parseError> errorList = {};  
};

int main() {
    Lexer myLexer("x = 5\nprint(x)\nprint(x + 1)\n");
    vector<Token> myTokens = myLexer.tokenize();
    for (const Token& t : myTokens) {
        cout << static_cast<int>(t.type) << " " << t.lexeme << "\n";
    }

    Parser myParser(myTokens);
    vector<StmtPtr> tree = myParser.parse();

    cout << "\n--- AST ---\n";
    for (const StmtPtr& s : tree) {
        printStmt(s);
    }

    // ERROR HANDLE
    for (const lexError& err : myLexer.getErrors()){
        cout << "[" << err.type << "] line " << err.line << ": " << err.message << "\n";
    }
    for (const parseError& err : myParser.getErrors()) {
        cout << "[" << err.type << "] line " << err.line << ": " << err.message << "\n";
    }
}
