*
 *
 * The Lexer. Given an arithmatic python line, return a tokenized version. 
 *
*/

#include <cctype>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
using namespace std;

enum class TokenType {
    NUMBER, IDENT, PLUS, MINUS, STAR, SLASH,
    EQUALS, LPAREN, RPAREN, PRINT,
    NEWLINE, INDENT, DEDENT, END_OF_FILE, UNKNOWN
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
            {'=', TokenType::EQUALS}
        };

        unordered_map<string, TokenType> keywords = {
            {"print", TokenType::PRINT}
        };

        vector<Token> tokenize() {
            vector<Token> tokens;
            skipSpaces();
            while (position_ < source_.size()) {
                tokens.push_back(scanToken());
                skipSpaces();
            }
            tokens.push_back(Token{TokenType::END_OF_FILE, "", line_});
            return tokens;
        }// runs the whole scan, returns all tokens

    private:
        char peek(int pos) {
            return source_[pos];
        }

        void advance(int n = 1){
            position_ += n;
        }

        bool validPos(int pos) {
            return pos < source_.size();
        }

        void skipSpaces() {
            while ((validPos(position_) && isspace(peek(position_))) && peek(position_) != '\n') {
                advance();
            }
        }

        // produces one token, advances position_
        Token scanToken() {
            int step = 1;
            string text = "";
            TokenType type;
            char snippet = peek(position_);

            if (snippet == '\n'){
                type = TokenType::NEWLINE;
                line_++;
                text.push_back(snippet);
            }
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
            else if (operation.find(snippet) != operation.end()) {
                 type = operation.at(snippet);
                 text.push_back(snippet);
            }
            else if (isdigit(snippet)) {
                type = TokenType::NUMBER; 
                text.push_back(snippet);

                while (validPos(position_ + step) && isdigit(peek(position_ + step))) {
                    // Add to end
                    text.push_back((peek(position_ + step)));      
                    step++;
                }
            }
            else {
                type = TokenType::UNKNOWN;
                text = text + ("UNKNOWN");
            }
            
            advance(step);
            return Token{type, text, line_};
            
        }

        string source_;
        size_t position_ = 0;
        int line_ = 1;
        vector<int> indentStack_ = {0};
};


int main() {
     Lexer myLexer("print(x + 1) + 1    ");
    
    for (const Token& t : myLexer.tokenize()) {
        cout << static_cast<int>(t.type) << " " << t.lexeme << "\n";
    }
}
