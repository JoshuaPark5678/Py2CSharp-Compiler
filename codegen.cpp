/*
* The Code generator, produces c# code from the parsed AST
*/ 

#include <iostream>
#include "parser.cpp"
#include <set>
#include <string>
# include <variant>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;

class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual string visitIntLit(const IntLit& n) = 0; 
    virtual string visitFloatLit(const FloatLit& n) = 0;
    virtual string visitIdent(const Ident& i) = 0;
    virtual string visitBinaryOp(const BinaryOp& b) = 0; 
    virtual string visitUnaryOp(const UnaryOp& u) = 0;
    virtual string visitStringLit(const StringLit& s) = 0;
    virtual string visitBoolLit(const BoolLit& b) = 0;
    virtual string visitCallExpr(const CallExpr& c) = 0;
};

class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual string visitPrintStmt(const PrintStmt& p) = 0;
    virtual string visitExprStmt(const ExprStmt& e) = 0;
    virtual string visitAsgnStmt(const AsgnStmt& a) = 0;
    virtual string visitIfStmt(const IfStmt& i) = 0;
    virtual string visitWhileStmt(const WhileStmt& w) = 0;
    virtual string visitForStmt(const ForStmt& f) = 0;
};

string IntLit::accept(ExprVisitor& v) const { return v.visitIntLit(*this); }
string FloatLit::accept(ExprVisitor& v) const { return v.visitFloatLit(*this); }
string Ident::accept(ExprVisitor& v) const { return v.visitIdent(*this); }
string BinaryOp::accept(ExprVisitor& v) const { return v.visitBinaryOp(*this); }
string UnaryOp::accept(ExprVisitor& v) const { return v.visitUnaryOp(*this); }
string StringLit::accept(ExprVisitor& v) const { return v.visitStringLit(*this); }
string BoolLit::accept(ExprVisitor& v) const { return v.visitBoolLit(*this); }
string CallExpr::accept(ExprVisitor& v) const { return v.visitCallExpr(*this); }

string PrintStmt::accept(StmtVisitor& v) const { return v.visitPrintStmt(*this); }
string ExprStmt::accept(StmtVisitor& v) const { return v.visitExprStmt(*this); }
string AsgnStmt::accept(StmtVisitor& v) const { return v.visitAsgnStmt(*this); }
string IfStmt::accept(StmtVisitor& v) const { return v.visitIfStmt(*this); }
string WhileStmt::accept(StmtVisitor& v) const { return v.visitWhileStmt(*this); }
string ForStmt::accept(StmtVisitor& v) const { return v.visitForStmt(*this); }

class CodeGenVisitor : public ExprVisitor, public StmtVisitor {
public:
    CodeGenVisitor(vector<StmtPtr>& stmts) {
        program_ = std::move(stmts);
    }

    string generate() {
        string output = "";
        for (const StmtPtr& stmt : program_) {
            output = output + genStmt(stmt);
        }
        return output;
    }

    // ExprVisitor
    string visitIntLit(const IntLit& n) override { return std::to_string(n.value); }
    string visitFloatLit(const FloatLit& n) override { return std::to_string(n.value); }
    string visitIdent(const Ident& i) override { return i.name; }
    string visitBinaryOp(const BinaryOp& b) override {  return "(" + genExpr(b.left) + genOp(b.op) + genExpr(b.right) + ")"; }
    string visitUnaryOp(const UnaryOp& u) override { return "(" + genOp(u.op) + genExpr(u.operand) + ")"; }
    string visitStringLit(const StringLit& s) override {return "\"" + s.value + "\"" ; }
    string visitBoolLit (const BoolLit& b) override {return b.value ? "true" : "false"; }
    string visitCallExpr(const CallExpr& c) override { return " Unused visit CallExpr " ;   }



    // StmtVisitor
    string visitPrintStmt(const PrintStmt& p) override { return "Console.WriteLine(" + genExpr(p.value) + ");";}
    string visitExprStmt(const ExprStmt& e) override { return genExpr(e.value) + ";";}
    string visitAsgnStmt(const AsgnStmt& a) override { 
        if (declaredVars_.count(a.name)) {
            return a.name + " = " + genExpr(a.value);
        }
        return "dynamic " + a.name + " = " + genExpr(a.value) + ";";
    }
    string visitIfStmt(const IfStmt& i) override {
        string thenBranches;
        for (const auto& stmt : i.thenBranches){
            thenBranches.append("   " + genStmt(stmt));
        }
        if (i.elseBranches.empty()) {
            return "if (" + genExpr(i.condition) + ") {\n" + thenBranches + "}\n";
        }
        string elseBranches;
        for (const auto& stmt : i.elseBranches){
            elseBranches.append("   " + genStmt(stmt));
        }
        return "if (" + genExpr(i.condition) + ") {\n" + thenBranches + "} else {\n" + elseBranches + "}\n";
    }

    string visitWhileStmt(const WhileStmt& w) override {
        string body;
        for (const auto& stmt : w.body){
            body.append("    " + genStmt(stmt));
        }

        return "while (" + genExpr(w.condition) + ") {\n" + body + "}\n";
    }

    string visitForStmt(const ForStmt& f) override {
        string body;
        for (const auto& stmt : f.body) {
            body.append("   " + genStmt(stmt));
        }
        if (std::holds_alternative<CallExpr>(*f.iterable)) {
            const auto& iterable = std::get<CallExpr>(*f.iterable);
            if (iterable.callee == "range") {
                const auto& args = iterable.args;
                int argsSize = args.size();
                string varName = f.varName;
                string start = "0";
                string cond;
                string inc = "++";
                if (argsSize == 1) {
                    cond = " < " + genExpr(args[0]);
                } else if (argsSize == 2) {
                    start = genExpr(args[0]);
                    cond = " < " + genExpr(args[1]);
                } else if (argsSize == 3) {
                    start = genExpr(args[0]);
                    cond = " < " + genExpr(args[1]);
                    inc = " += " + genExpr(args[2]);
                } else {
                    return " ERROR ";
                }
                return "for (int " + varName + " = " + start + "; " + varName + cond + "; " + varName + inc + ") {\n" + body + "}\n";
            }
        }
        return " ERROR "; 

    }
private:
    struct Dispatcher {
        CodeGenVisitor& v;
        // works for any node type that has accept
        template<typename T>
        string operator()(const T& node) { return node.accept(v); }
    };

    string genExpr(const ExprPtr& e) {
        if (e) {
            return std::visit(Dispatcher{*this}, *e);

        }
        return "";
    }

    string genStmt(const StmtPtr& s){
        if (s) {
            return std::visit(Dispatcher{*this}, *s) + "\n";
        }
        return "";
    }
 
    string genOp(TokenType op) {
        switch (op) {
            case TokenType::PLUS:
                return " + ";
            case TokenType::MINUS:
                return " - ";
            case TokenType::STAR:
                return " * ";
            case TokenType::SLASH:
                return " / ";
            case TokenType::MOD:
                return " % ";
            case TokenType::GREATER:
                return " > ";
            case TokenType::LESS:
                return " < ";
            case TokenType::GREATER_EQUAL:
                return " >= ";
            case TokenType::LESS_EQUAL:
                return " <= ";
            case TokenType::EQUAL_EQUAL:
                return " == ";
            case TokenType::NOT_EQUAL:
                return " != ";
            default:
                // Would make error in future
                return " error ";
        }
    }
    
    vector<StmtPtr> program_;
    set<string> declaredVars_;
};

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    std::ifstream file(argv[1]);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();

    cout << "\n--- SOURCE ---\n";
    std::cout << fileContent; 

    Lexer myLexer(fileContent);
    vector<Token> myTokens = myLexer.tokenize();

    for (const Token& t : myTokens) {
        cout << static_cast<int>(t.type) << " " << t.lexeme << "\n";
    }

    Parser myParser(myTokens);
    auto MyTree = myParser.parse();

    cout << "\n--- AST ---\n";
    for (const StmtPtr& s : MyTree) {
        printStmt(s);
    }

    CodeGenVisitor myCodeGenVisitor(MyTree);
    string output = myCodeGenVisitor.generate();

    cout << "\n--- OUTPUT ---\n";
    cout << output;
    
    // ERROR HANDLE
    for (const lexError& err : myLexer.getErrors()){
        cout << "[" << err.type << "] line " << err.line << ": " << err.message << "\n";
    }
    for (const parseError& err : myParser.getErrors()) {
        cout << "[" << err.type << "] line " << err.line << ": " << err.message << "\n";
    }
    return 0;
}
