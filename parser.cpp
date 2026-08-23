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
struct IntLit;
struct FloatLit;
struct Ident;
struct BinaryOp;
struct UnaryOp;
struct StringLit;
struct BoolLit;
struct CallExpr;

using Expr = variant<IntLit, FloatLit, Ident, BinaryOp, UnaryOp, StringLit, BoolLit, CallExpr>;
using ExprPtr = unique_ptr<Expr>;

struct IntLit {int value;};
struct FloatLit { double value; };
struct Ident     { string name; };
struct BinaryOp  { TokenType op; ExprPtr left; ExprPtr right; };
struct UnaryOp {TokenType op; ExprPtr operand;};
struct StringLit {string value; };
struct BoolLit {bool value;};
struct CallExpr {string callee; vector<ExprPtr> args; };

// === STATEMENT === //
struct PrintStmt;
struct ExprStmt;
struct AsgnStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
// add more later 

using Stmt = variant<PrintStmt, ExprStmt, AsgnStmt, IfStmt, WhileStmt, ForStmt>;
using StmtPtr = unique_ptr<Stmt>;

struct PrintStmt {ExprPtr value; };
struct ExprStmt {ExprPtr value; };
struct AsgnStmt {string name; ExprPtr value; TokenType op;};
struct IfStmt {ExprPtr condition; vector<StmtPtr> thenBranches; vector<StmtPtr> elseBranches;};
struct WhileStmt {ExprPtr condition; vector<StmtPtr> body;};
struct ForStmt {string varName; ExprPtr iterable; vector<StmtPtr> body; };
// For stmt TBD

// DEBUG
#include "debug_print.cpp"

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
        } else if (currType == TokenType::WHILE) {
            return whileStatement();
        } else if (currType == TokenType::FOR) {
            return forStatement();
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
    StmtPtr identStatement() {
        TokenType nextType = peek(position_ + 1).type;
        if (nextType == TokenType::EQUALS || nextType == TokenType::MOD_EQUAL
            || nextType == TokenType::PLUS_EQUAL || nextType == TokenType::MINUS_EQUAL 
            || nextType == TokenType::STAR_EQUAL || nextType == TokenType::SLASH_EQUAL) {

            return assignStatement();
        } else {
            auto value = expression();
            expect(TokenType::NEWLINE, "Expected Newline");
            return make_unique<Stmt>(ExprStmt{std::move(value)});
        }
    }
    StmtPtr assignStatement() {
        string name = peek(position_).lexeme;
        advance();
        TokenType op = peek(position_).type;
        advance();
        auto value = expression();
        expect(TokenType::NEWLINE, "Expected Newline");
        return make_unique<Stmt>(AsgnStmt{name, std::move(value), op}); 
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
    StmtPtr whileStatement() {
        expect(TokenType::WHILE, "Expected 'while'");
        auto condition = expression();
        expect(TokenType::COLON, "Expected Colonk");
        expect(TokenType::NEWLINE, "Expected Newline");
        vector<StmtPtr> whileBlock = block();
        return make_unique<Stmt>(WhileStmt{std::move(condition), std::move(whileBlock)});
    }
    StmtPtr forStatement() {
        expect(TokenType::FOR, "Expected 'for'");
        string varName = peek(position_).lexeme;
        advance();
        expect(TokenType::IN, "Expected 'in'");
        ExprPtr iterable = expression();
        expect(TokenType::COLON, "Expected Colon");
        expect(TokenType::NEWLINE, "Expected Newline");
        vector<StmtPtr> forBlock = block();
        return make_unique<Stmt>(ForStmt{varName, std::move(iterable), std::move(forBlock)});
    }

    ExprPtr expression() {
        return logicalOperators();
    }
    ExprPtr logicalOperators() {
        // for continous multiplication and division
        auto result = comparativeOperators();
        while (check(TokenType::AND) || check(TokenType::OR)) {
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
        if (currType == TokenType::PLUS || currType == TokenType::MINUS || currType == TokenType::NOT){
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
        } else if (currType == TokenType::INT){
            advance();
            return make_unique<Expr>(IntLit{stoi(currToken.lexeme)});
        }else if (currType == TokenType::FLOAT){
            advance();
            return make_unique<Expr>(FloatLit{stod(currToken.lexeme)});
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
        } else if (currType == TokenType::RANGE) {
            vector<ExprPtr> args;
            advance();
            expect(TokenType::LPAREN, "Expected LPAREN");
            while (peek(position_ + 1).type == TokenType::COMMA) {
                args.push_back(expression());
                advance();
            }
            args.push_back(expression());
            expect(TokenType::RPAREN, "Expected RPAREN");
            return make_unique<Expr>(CallExpr{"range", std::move(args)});
        }
        advance();
        return nullptr;
    }

    vector<Token> tokens_;
    size_t position_ = 0;
    vector<parseError> errorList = {};  
};

int main() {
    Lexer myLexer("x = 1\nfor i in range(2, 6, 2):\n    x += 1\n");
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
