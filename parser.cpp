*
* The Parser. Given a list of tokens, convert it into an AST.
*/
#include "lexer.cpp"
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
struct StringLit;
struct BoolLit;

using Expr = variant<NumberLit, Ident, BinaryOp, UnaryOp, StringLit, BoolLit>;
using ExprPtr = unique_ptr<Expr>;

struct NumberLit { double value; };
struct Ident     { string name; };
struct BinaryOp  { TokenType op; ExprPtr left; ExprPtr right; };
struct UnaryOp {TokenType op; ExprPtr operand;};
struct StringLit {string value; };
struct BoolLit {bool value;};

// === STATEMENT === //
struct PrintStmt;
struct ExprStmt;
struct AsgnStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
// add more later 

using Stmt = variant<PrintStmt, ExprStmt, AsgnStmt, IfStmt, WhileStmt>;
using StmtPtr = unique_ptr<Stmt>;

struct PrintStmt {ExprPtr value; };
struct ExprStmt {ExprPtr value; };
struct AsgnStmt {string name; ExprPtr value; };
struct IfStmt {ExprPtr condition; vector<StmtPtr> thenBranches; vector<StmtPtr> elseBranches;};
struct WhileStmt {ExprPtr condition; vector<StmtPtr> body;};
// For stmt TBD

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
        },
        [&](const StringLit& s) {
            cout << string(depth * 2, ' ') << "StringLit(" << s.value << ")\n";
        },
        [&](const BoolLit& b) {
            cout << string(depth * 2, ' ') << "BoolLit(" << (b.value ? "true" : "false") << ")\n";
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
        },
        [&](const IfStmt& i) {
            cout << string(depth * 2, ' ') << "IfStmt\n";
            printExpr(i.condition, depth + 1);
            cout << string((depth + 1) * 2, ' ') << "Then:\n";
            for (const StmtPtr& stmt : i.thenBranches) {
                printStmt(stmt, depth + 2);
            }
            if (!i.elseBranches.empty()) {
                cout << string((depth + 1) * 2, ' ') << "Else:\n";
                for (const StmtPtr& stmt : i.elseBranches) {
                    printStmt(stmt, depth + 2);
                }
            }
        },
        [&](const WhileStmt& w) {
            cout << string(depth * 2, ' ') << "WhileStmt\n";
            printExpr(w.condition, depth + 1);
            for (const StmtPtr& stmt : w.body) {
                printStmt(stmt, depth + 1);
            }
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

    vector<StmtPtr> block() {
        expect(TokenType::INDENT, "Expected Indent");
        vector<StmtPtr> result;

        while (peek(position_).type != TokenType::DEDENT && peek(position_).type != TokenType::END_OF_FILE) { 
            result.push_back(statement());
        }
        expect(TokenType::DEDENT, "Expected Dedent");
        return result;
    }

    StmtPtr statement(){
        Token currToken = peek(position_);
        TokenType currType = currToken.type;

        if (currType == TokenType::PRINT) {
            return printStatement();
        } else if (currType == TokenType::IDENT) {
            return identStatement();
        } else if (currType == TokenType::IF) {
            return ifStatement();
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
    StmtPtr ifStatement() {
        vector<StmtPtr> emptyVector;
        advance(); // Could be IF or ELIF
        auto condition = expression();
        expect(TokenType::COLON, "Expected Colon");
        if (peek(position_).type == TokenType::NEWLINE) {
            expect(TokenType::NEWLINE, "Expected Newline");
            vector<StmtPtr> thenBlock = block();
            if (peek(position_).type == TokenType::ELIF) {
                vector<StmtPtr> elifBlock;
                elifBlock.push_back(ifStatement());
                return make_unique<Stmt>(IfStmt{std::move(condition), std::move(thenBlock), std::move(elifBlock)});
            } else if (peek(position_).type == TokenType::ELSE) {
                expect(TokenType::ELSE, "Expected 'else'");
                expect(TokenType::COLON, "Expected Colon");
                expect(TokenType::NEWLINE, "Expected Newline"); 
                return make_unique<Stmt>(IfStmt{std::move(condition), std::move(thenBlock), block()});
            } else {
                return make_unique<Stmt>(IfStmt{std::move(condition), std::move(thenBlock), std::move(emptyVector)});
            }
        }
        vector<StmtPtr> inlineBlock;
        inlineBlock.push_back(statement()); 
        return make_unique<Stmt>(IfStmt{std::move(condition), std::move(inlineBlock), std::move(emptyVector)});
    }

    ExprPtr expression() {
        return logicalOperators();
    }
    ExprPtr logicalOperators() {
        // for continous multiplication and division
        auto result = comparativeOperators();
        while (check(TokenType::AND) || check(TokenType::OR) || check(TokenType::NOT)) {
            TokenType op = peek(position_).type;
            advance();
            auto right = comparativeOperators();
            result = make_unique<Expr>(BinaryOp{op, std::move(result), std::move(right)});
        }        
        return result;
    }
    ExprPtr comparativeOperators() {
        // for equalities and comparison
        auto result = additiveOperators();
        while (check(TokenType::GREATER) || check(TokenType::LESS)
        || check(TokenType::GREATER_EQUAL) || check(TokenType::LESS_EQUAL)
        || check(TokenType::EQUAL_EQUAL) || check(TokenType::NOT_EQUAL)) {
            TokenType op = peek(position_).type;
            advance();
            auto right = additiveOperators();
            result = make_unique<Expr>(BinaryOp{op, std::move(result), std::move(right)});
        }        
        return result;
    }
    ExprPtr additiveOperators(){
        // for continous addition and subtraction
        auto result = multiplicativeOperators();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            TokenType op = peek(position_).type;
            advance();
            auto right = multiplicativeOperators();
            result = make_unique<Expr>(BinaryOp{op, std::move(result), std::move(right)});
        }        
        return result;
    }
    ExprPtr multiplicativeOperators() {
        // for continous multiplication and division
        auto result = unary();
        while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::MOD)) {
            TokenType op = peek(position_).type;
            advance();
            auto right = unary();
            result = make_unique<Expr>(BinaryOp{op, std::move(result), std::move(right)});
        }        
        return result;
    }
    ExprPtr unary() {
        Token currToken = peek(position_);
        TokenType currType = currToken.type;
        if (currType == TokenType::PLUS || currType == TokenType::MINUS){
            advance();
            return make_unique<Expr>(UnaryOp{currType, unary()});
        }
        return primary();
    }
    ExprPtr primary() {
        Token currToken = peek(position_);
        TokenType currType = currToken.type;
        if (currType == TokenType::LPAREN) {
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
        } else if (currType == TokenType::NUMBER){
            advance();
            return make_unique<Expr>(NumberLit{stod(currToken.lexeme)});
        } else if (currType == TokenType::IDENT) {
            advance();
            return make_unique<Expr>(Ident{currToken.lexeme});
        } else if (currType == TokenType::STRING) {
            advance();
            return make_unique<Expr>(StringLit{currToken.lexeme});
        } else if (currType == TokenType::BOOL) {
            advance();
            if (currToken.lexeme == "True"){
                return make_unique<Expr>(BoolLit{true});
            }
            return make_unique<Expr>(BoolLit{false});
        }
        return nullptr;
    }

    vector<Token> tokens_;
    size_t position_ = 0;
    vector<parseError> errorList = {};  
};

int main() {
    Lexer myLexer("if x:\n    print(1)\nelif y:\n    print(2)\nelse:\n    print(3)\nif z: print(9)\n");
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
