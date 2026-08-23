/*
 *
 * Debug-only AST pretty-printer. Included by parser.cpp after the AST
 * struct definitions (NumberLit, Ident, ..., WhileStmt) since these
 * visitors need those types in scope.
 *
*/

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void printExpr(const ExprPtr& e, int depth = 0) {
    if (!e) { cout << string(depth * 2, ' ') << "null\n"; return; }
    visit(overloaded{
        [&](const IntLit& n) {
            cout << string(depth * 2, ' ') << "IntLit(" << n.value << ")\n";
        },
        [&](const FloatLit& n) {
            cout << string(depth * 2, ' ') << "FloatLit(" << n.value << ")\n";
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
        },
        [&](const CallExpr& c) {
            cout << string(depth * 2, ' ') << "CallExpr(" << c.callee << ")\n";
            for (const ExprPtr& arg : c.args) {
                printExpr(arg, depth + 1);
            }
        }
    }, *e);
}

void printStmt(const StmtPtr& s, int depth = 0) {
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
            cout << string(depth * 2, ' ') << "AsgnStmt(" << a.name << ", op=" << static_cast<int>(a.op) << ")\n";
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
        },
        [&](const ForStmt& f) {
            cout << string(depth * 2, ' ') << "ForStmt(" << f.varName << ")\n";
            printExpr(f.iterable, depth + 1);
            for (const StmtPtr& stmt : f.body) {
                printStmt(stmt, depth + 1);
            }
        }
    }, *s);
}
