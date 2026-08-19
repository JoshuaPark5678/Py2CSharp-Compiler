/*
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

struct NumberLit;
struct Ident;
struct BinaryOp;
struct UnaryOp;

// ptr ast
using Expr = variant<NumberLit, Ident, BinaryOp, UnaryOp>;
using ExprPtr = unique_ptr<Expr>;

struct NumberLit { double value; };
struct Ident     { string name; };
struct BinaryOp  { TokenType op; ExprPtr left; ExprPtr right; };
struct UnaryOp {TokenType op; ExprPtr operand;};

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

class Parser {
public:
    Parser(vector<Token> tokens) {
        tokens_ = tokens;
    }

    ExprPtr parseExpression() { return expression(); }

    // vector<StmtPtr> parse() {
        // TODO
    // }
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
    
    // one method per grammar rule above
    // StmtPtr statement();
    // StmtPtr printStatement();
    // StmtPtr assignStatement();
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
    Lexer myLexer("(1 + 2) * 3\n");
    vector<Token> myTokens = myLexer.tokenize();
    for (const Token& t : myTokens) {
        cout << static_cast<int>(t.type) << " " << t.lexeme << "\n";
    }

    Parser myParser(myTokens);
    ExprPtr tree = myParser.parseExpression();
    cout << "\n--- AST ---\n";
    printExpr(tree);
}
