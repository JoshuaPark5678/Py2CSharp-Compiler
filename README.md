# Py2C# Transpiler

A from-scratch source-to-source transpiler that takes a restricted subset
of Python and emits C#. Built in C++ with a hand-rolled lexer, a
recursive-descent parser (no `ast` module), and an OOP-visitor code
generator. C# is executed with .NET.

The input language is intentionally small: dynamically-typed variables
(emitted as C# `dynamic`), arithmetic and boolean expressions, `print`,
`if`/`elif`/`else`, `while`, `for` over `range()`, and compound
assignment. Full Python is explicitly out of scope.

## Status

Working MVP: sample programs transpile and run with correct output.
Functions, lists, dicts, and static-type inference are not started.

## Build

One translation unit, no dependencies beyond a C++ compiler:

```
g++ codegen.cpp -o main
```

`parser.cpp` includes `lexer.cpp` and `debug_print.cpp`, so `codegen.cpp`
is the only file you compile.

## Run

```
./main prog.py            # writes Program.cs next to the binary
./main prog.py debug      # also prints source, tokens, AST, generated C#
./main prog.py --run      # writes to a scratch project and runs it
./main prog.py debug --run
```

`--run` needs a scratch console project (one-time setup):

```
dotnet new console -o /tmp/py2cs-run
```

## Example

Input (`test.py`):

```python
x = 1
y = 2
print(x + y)
for i in range(5):
    print(i)
```

Generated C# (excerpt):

```csharp
dynamic x = 1;
dynamic y = 2;
Console.WriteLine((x + y));
for (int i = 0; i < 5; i++) {
    Console.WriteLine(i);
}
```

## Roadmap

1. Lexer (done: indent/dedent tracking, error collection)
2. Parser (done: precedence, if/elif/else, while, for-range, compound assign)
3. Variables via C# `dynamic` (done; static types dropped as unnecessary)
4. Control flow (done)
5. Code generation to C# (done, MVP verified end-to-end)
6. Run C# via .NET (done: `--run` flag)
7. Functions (next)
8. More data structures: lists, dicts, sets (next)

## Known gaps

- AST debug printer and error reporting are debug-only; silent bad runs
  give no diagnostics without the `debug` flag.
- General function calls are a stub (`CallExpr` outside `range()`).
- Compound assignment parses but emits plain `=` in some paths.
- Accepts `"str" + int`, which real Python rejects (C# `dynamic`
  binds it at runtime).
      
      
