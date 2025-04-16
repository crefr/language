# Language
Simple compiler for my own programming language. Currently has backend only for [my processor emulator](https://github.com/crefr/processor).

The compiler has three main stages:
1. **Frontend** - translation from language to abstract syntax tree (AST) and saving intermediate representation. Code runs through lexical analysis and then through syntax analysis (implemented by recursive descent algorithm).
2. **Middleend** - optimizations of intermediate representation (folding constants, deleting neutral elements, etc.).
3. **Backend** - translation into assembler code.

Also there is **"reverse frontend"** - translation from intermediate representation back to the original code. Because of the common IR format, supports cross-translation with [orientiered's MoneyLang project](https://github.com/orientiered/MoneyLang).

Code example (recursive factorial calculating):
```
var x;
in(x);

out(factorial(x));

func factorial(number)
begin
    if (number > 1)
    begin
        var answer;
        var new_number;

        new_number = number - 1;
        answer = factorial(new_number) * number;

        return answer;
    end;

    return 1;
end;
```

## Install
To install the program please run this:

``` bash
git clone --recursive https://github.com/crefr/language
cd language
make BUILD=RELEASE
```

In the directory should appear executable files `frontend.exe`, `middleend.exe` and `backend.exe`.

Also if you want to be able to run compiled program you should compile `processor`:
```bash
cd processor
make BUILD=RELEASE
cd ..
```

## Using
### The easy way
If you are lazy, you can just run script file `launch.sh` to compile and execute program in the file `program.txt`:
``` bash
bash launch.sh
```

### Manual way
If you want to manually compile your program step by step do these steps:

1. **Frontend**

    Firstly you need to translate your program text into an IR (intermediate representation):
    ```bash
    ./frontend.exe program.txt out_ir.ast
    ```
2. **Middleend** (optional)

    If you want to slightly optimize your program you can run:
    ```bash
    ./middleend.exe program_IR.ast
    ```
    The result of optimizing will be overwritten to source file.

3. **Backend**

    Finally you need to translate the IR into assembler. So run:
    ```bash
    ./backend.exe program_IR.ast compiled.asm
    ```
Now you can run the program on the processor:
```
./processor/asm.exe compiled.asm
./processor/spu.exe compiled.bin
```

### Reverse frontend

If you want to translate the IR back to the source code you can use reverse frontend:
```bash
frontend.exe -1 program_IR.ast source_code.txt
```

## Grammar

Here is the grammar of the language syntax in the BNF-like format (you can also find it in the [`grammar.txt`](grammar.txt)):

```
<RESULT>    ::= <Chain>

<Chain>     ::= (<Statement> ";")*
<Statement> ::= <FuncCall> | <STDfunc> | <Return> | <IF> | <While> | <VarDecl> | <FuncDecl> | <Assign>

<IF>        ::= "if" "(" <Expr> ")" <Block> (<ELSE>)?
<ELSE>      ::= "else" <Block>

<While>     ::= "while" "(" <Expr> ")" <Block>

<VarDecl>   ::= "var" <ID>

<FuncDecl>  ::= "func" <ID> "(" <Var> ("," <Var>)* ")" <Block>
<Block>     ::= "begin" <Chain> "end"

<Assign>    ::= <Var> "=" <Expr>
<STDfunc>   ::= <Input> | <Output>

<Return>    ::= "return" <Expr>

<Input>     ::= "in"  "(" <Var> ")"
<Output>    ::= "out" "(" <Expr> ")"

<Expr>      ::= <AddSub> ((">" | "<") <AddSub>)*

<AddSub>    ::= <MulDiv> (("+" | "-") <MulDiv>)*
<MulDiv>    ::= <Power>  (("*" | "/") <Power>)*

<Power>     ::= <Primary> ("^" <Power>)?
<Primary>   ::= "(" <Expr> ")" | <MathFunc> | <FuncCall> | <Var> | <Number>

<FuncCall>  ::= <ID> "(" <Expr> ("," <Expr>)* ")"
<Var>	    ::= <ID>

<ID>	    ::= ([a-z] | "_") ([a-z] | [0-9] | "_")*
<Number>    ::= [0-9]+
<MathFunc>  ::= "sin" | "cos" | "tan" | "ln"
```

Also this grammar is available at the BNF Playground (the [link](https://bnfplayground.pauliankline.com/?bnf=%3CRESULT%3E%20%20%3A%3A%3D%20%3CChain%3E%0A%0A%3CChain%3E%20%20%20%3A%3A%3D%20(%3CStatement%3E%20%22%3B%22)*%0A%3CStatement%3E%20%3A%3A%3D%20%3CFuncCall%3E%20%7C%20%3CSTDfunc%3E%20%7C%20%3CReturn%3E%20%7C%20%3CIF%3E%20%7C%20%3CWhile%3E%20%7C%20%3CVarDecl%3E%20%7C%20%3CFuncDecl%3E%20%7C%20%3CAssign%3E%0A%0A%3CIF%3E%20%20%20%20%20%20%3A%3A%3D%20%22if%22%20%22(%22%20%3CExpr%3E%20%22)%22%20%3CBlock%3E%20(%3CELSE%3E)%3F%0A%3CELSE%3E%20%20%20%20%3A%3A%3D%20%22else%22%20%3CBlock%3E%0A%0A%3CWhile%3E%20%20%20%3A%3A%3D%20%22while%22%20%22(%22%20%3CExpr%3E%20%22)%22%20%3CBlock%3E%0A%0A%3CVarDecl%3E%20%3A%3A%3D%20%22var%22%20%3CID%3E%0A%0A%3CFuncDecl%3E%20%3A%3A%3D%20%22func%22%20%3CID%3E%20%22(%22%20%3CVar%3E%20(%22%2C%22%20%3CVar%3E)*%20%22)%22%20%3CBlock%3E%0A%3CBlock%3E%20%20%20%3A%3A%3D%20%22begin%22%20%3CChain%3E%20%22end%22%0A%0A%3CAssign%3E%20%20%3A%3A%3D%20%3CVar%3E%20%22%3D%22%20%3CExpr%3E%0A%3CSTDfunc%3E%20%3A%3A%3D%20%3CInput%3E%20%7C%20%3COutput%3E%0A%0A%3CReturn%3E%20%20%3A%3A%3D%20%22return%22%20%3CExpr%3E%0A%0A%3CInput%3E%20%20%20%3A%3A%3D%20%22in%22%20%20%22(%22%20%3CVar%3E%20%22)%22%0A%3COutput%3E%20%20%3A%3A%3D%20%22out%22%20%22(%22%20%3CExpr%3E%20%22)%22%0A%0A%3CExpr%3E%20%20%20%20%3A%3A%3D%20%3CAddSub%3E%20((%22%3E%22%20%7C%20%22%3C%22)%20%3CAddSub%3E)*%0A%0A%3CAddSub%3E%20%20%3A%3A%3D%20%3CMulDiv%3E%20((%22%2B%22%20%7C%20%22-%22)%20%3CMulDiv%3E)*%0A%3CMulDiv%3E%20%20%3A%3A%3D%20%3CPower%3E%20%20((%22*%22%20%7C%20%22%2F%22)%20%3CPower%3E)*%0A%0A%3CPower%3E%20%20%20%3A%3A%3D%20%3CPrimary%3E%20(%22%5E%22%20%3CPower%3E)%3F%0A%3CPrimary%3E%20%3A%3A%3D%20%22(%22%20%3CExpr%3E%20%22)%22%20%7C%20%3CMathFunc%3E%20%7C%20%3CFuncCall%3E%20%7C%20%3CVar%3E%20%7C%20%3CNumber%3E%0A%0A%3CFuncCall%3E%20%3A%3A%3D%20%3CID%3E%20%22(%22%20%3CExpr%3E%20(%22%2C%22%20%3CExpr%3E)*%20%22)%22%0A%3CVar%3E%09%20%20%3A%3A%3D%20%3CID%3E%0A%0A%3CID%3E%09%20%20%3A%3A%3D%20(%5Ba-z%5D%20%7C%20%22_%22)%20(%5Ba-z%5D%20%7C%20%5B0-9%5D%20%7C%20%22_%22)*%0A%3CNumber%3E%20%20%3A%3A%3D%20%5B0-9%5D%2B%0A%3CMathFunc%3E%20%3A%3A%3D%20%22sin%22%20%7C%20%22cos%22%20%7C%20%22tan%22%20%7C%20%22ln%22&name=))
