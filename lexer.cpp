/*
 *
 * The Lexer. Given an arithmatic python line, return a tokenized version. 
 *
*/

#include <cctype>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>
using namespace std;

enum class TokenType {
    NUMBER, IDENT, PLUS, MINUS, STAR, SLASH,
    EQUALS, COLON, LPAREN, RPAREN, PRINT, IF, WHILE, FOR, STRING,
    NEWLINE, INDENT, DEDENT, END_OF_FILE, UNKNOWN
};

struct lexError {
    string type;
    string message;
    int line;
};

struct Token {
    TokenType type;
    string lexeme;   // the raw text matched, e.g. "42" or "x"
    int line;              // for error messages later
};

class Lexer {
    public:
        Lexer(string source) {
          source_ = source;
        }

        unordered_map<char, TokenType> operation = {
            {'+', TokenType::PLUS},
            {'-', TokenType::MINUS},
            {'*', TokenType::STAR},
            {'/', TokenType::SLASH},
            {'(', TokenType::LPAREN},
            {')', TokenType::RPAREN},
            {'=', TokenType::EQUALS},
            {':', TokenType::COLON}
        };

        unordered_map<string, TokenType> keywords = {
        {"print", TokenType::PRINT},
        {"if", TokenType::IF},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR}
        };

        vector<Token> tokenize() {
            vector<Token> tokens;
            while (position_ < source_.size()) {
                Token t = scanToken();
                if (t.type == TokenType::END_OF_FILE) {
                    // Sentinel from scanToken(): a trailing comment ran to
                    // true EOF. Nothing real to keep; fall to the epilogue.
                    break;
                }
                tokens.push_back(t);
                if (!isStartLine_) {
                    skipSpaces();
                    skipComment();
                }
            }

            // Drain all indents at the end of file.
            while (indentStack_.size() > 1) {
                indentStack_.pop_back();
                tokens.push_back(Token{TokenType::DEDENT, "dedent", line_});
            }

            tokens.push_back(Token{TokenType::END_OF_FILE, "", line_});
            return tokens;
        }// runs the whole scan, returns all tokens

    private:
        char peek(size_t pos) {
            return source_[pos];
        }

        void advance(int n = 1){
            position_ += n;
        }

        bool validPos(size_t pos) {
            return pos < source_.size();
        }

        void skipSpaces(int n = -1) { // optional num of Spaces to skip
            while ((validPos(position_) && isspace(peek(position_))) && peek(position_) != '\n' && n != 0) {
                n--;
                advance();
            }
        }

        int countSpaces() {
            int n = 0;
            while ((validPos(position_ + n) && isspace(peek(position_ + n))) && peek(position_ + n) != '\n') {
                n++;
            }
            return n;
        }

        // '#' to end of line, discarded.        
        void skipComment() {
            if (validPos(position_) && peek(position_) == '#') {
                while (validPos(position_) && peek(position_) != '\n') {
                    advance();
                }
            }
        }
        
        optional<Token> handleLineStart() {
            // Compare Indent with the indentStack_
            int numOfSpaces = countSpaces(); 

            if (numOfSpaces > indentStack_.back()) {
                // New indents

                indentStack_.push_back(numOfSpaces);
                skipSpaces();
                skipComment();
                isStartLine_ = false;
                return Token{TokenType::INDENT, "indent", line_};
            } 
            else if (numOfSpaces < indentStack_.back()) {
                // Dedent

                indentStack_.pop_back();
                // Inconsistent Indents?
                if (indentStack_.back() < numOfSpaces) {
                    errorList.push_back(lexError{ "Indentation", "Indent Level Inconsistent", line_});
                }      
                return Token{TokenType::DEDENT, "dedent", line_};
            }
            else {
                // No New indents

                skipSpaces();
                skipComment();
                if (!validPos(position_)) {
                    // A trailing comment ran to true EOF with no
                    // newline left to stop at; nothing real left to scan.
                    return Token{TokenType::END_OF_FILE, "", line_};
                }
            }
            return nullopt;
        }
        // produces one token, advances position_
        Token scanToken() {
            int tokenLine = line_;

            // Is it start of a line?
            if (isStartLine_) {
                auto result =  handleLineStart();
                if (result.has_value()) {
                    return result.value();
                }
            }
            isStartLine_ = false;

            int step = 1;
            string text = "";
            TokenType type;
            char snippet = peek(position_);
            
            // NEWLINE
            if (snippet == '\n'){ 
                type = TokenType::NEWLINE;
                line_++;
                isStartLine_ = true;
                text = "Newline";
            }
            // VARIABLES
            else if (isalpha(snippet) || snippet == '_') {
                type = TokenType::IDENT; 
                text.push_back(snippet);

                while (validPos(position_ + step) && (isalpha(peek(position_ + step)) || isdigit(peek(position_ + step))|| peek(position_ + step) == '_')) {
                    // Add to end
                    text.push_back((peek(position_ + step)));      
                    step++;
                }

                if (keywords.find(text) != keywords.end()){
                    type = keywords.at(text);
                }
            }
            // OPERATIONS
            else if (operation.find(snippet) != operation.end()) {
                 type = operation.at(snippet);
                 text.push_back(snippet);
            }
            // NUMBERS
            else if (isdigit(snippet) || snippet == '.') {
                type = TokenType::NUMBER; 
                text.push_back(snippet);
                bool isFloat = snippet == '.'; 

                while (validPos(position_ + step) && (isdigit(peek(position_ + step)) || (peek(position_ + step) == '.' && !isFloat))) {
                    if (!isFloat){
                        isFloat = peek(position_ + step) == '.'; 
                    } 
                    text.push_back((peek(position_ + step)));
                    step++;
                }
            }
            // STRING LITERAL
            else if (snippet == '"') {
                type = TokenType::STRING; 

                while (validPos(position_ + step) && peek(position_ + step) != '"'){
                    text.push_back(peek(position_ + step));
                    step++;
                }
                if (validPos(position_ + step)) {
                    // End Quote Skip
                    step++;
                } else {
                    // No end Quote
                    errorList.push_back(lexError{ "MISSING QUOTES", "No Matching quotes", tokenLine});
                }
            }
            // UNKNOWN CATCH
            else {
                type = TokenType::UNKNOWN;
                text.push_back(snippet);
                errorList.push_back(lexError{"UNKNOWN", "UNKNOWN lexical element", tokenLine}); 
            }
            
            advance(step);
            return Token{type, text, tokenLine}; 
        }

        string source_;
        size_t position_ = 0;
        int line_ = 1;
        bool isStartLine_ = true;
        vector<int> indentStack_ = {0};
        vector<lexError> errorList = {};
};

int main() {
     Lexer myLexer("if a:\n    if b:\n        print(Hello)\n    print(2)\n");
    
    for (const Token& t : myLexer.tokenize()) {
        cout << static_cast<int>(t.type) << " " << t.lexeme << "\n";
    }
}
