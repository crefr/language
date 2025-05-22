# Язык программирования
Примитивный компилятор с возможностью компиляции на 2 разные платформы:

1. [Программный процессор (интерпретатор)](https://github.com/crefr/processor).
2. Intel / AMD x86-64

Компиляция проходит в 3 ключеавых этапа:
1. **Фронтенд** - трансляция из текста в абстрактное синтаксическое дерево (AST), сохранение его в промежуточный файл. Используется лексический анализ и синтаксический (основанный на алгоритме рекурсивного спуска)
2. **Миддленд** - оптимизации над промежуточным представлением (свертка констант, удаление нейтральных элементов и так далее)
3. **Бэкенд** - машинно-зависимая трансляция в итоговый исполняемый файл

Также реализован **"обратный фронтенд"** - трансляция из промежуточного представления обратно в код. Благодаря общему формату IR поддерживает кросс-трансляцию с [языком orientiered](https://github.com/orientiered/MoneyLang).

Пример кода (рекурсивное вычисление факториала):
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


## Бэкенд для X86-64

Самой масштабной частью проекта является **бинарная трансляция** в исполняемый файл. Бэкенд делится на следующие этапы:

1. Обход дерева, построение IR
2. Расчет адресов для переходов (jmp, jz, call)
3. Инъекция в выходной файл заголовка **ELF**, стандартной библиотеки
4. Обход IR, инъекция инструкций x86-84 в бинарном виде

Для последнего пункта были реализованы относительно универсальные "эмиттеры" инструкций ([x64_emitters.h](backend_x64/headers/x64_emitters.h)).

Пример использования таких эмиттеров:

```c
// sub rax, rcx
emit_sub_reg_reg(emit_ctx, R_RAX, R_RCX);

// push rax
emit_push_reg(emit_ctx, R_RAX);
```

Пример реализации такого эмиттера:
```c
// idiv reg64
size_t emit_idiv_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("idiv %s\n", reg_names[reg]);

    uint8_t rex = REX_W;
    check_dst_reg(reg, rex);

    return emit_bytes(rex, 0xF7, modRM(0b11, 7, reg));
}
```

Также для целей отладки каждый эмиттер пишет встраиваемую в данный момент инструкции в ассемблерный файл.


## X86-64 vs SPU

На первый взгляд может показаться, что бэкенд на x64 не имеет какого-либо смысла, ведь мы можем использовать универсальный платформо-независимый интерпретатор - SPU. Однако, попробуем сравнить скорости выполнения.

Для этого на обе платформы скомпилируем программу [`perf_test.txt`](code_examples/perf_test.txt) - рекурсивное вычисление факториала 15 в цикле. Будем замерять время выполнения программы с помощью утилиты `time`:

| Платформа | Время выполнения  |
| --------- | ------------------|
| X86-64    | 0.315 +- 0.005 мс |
| SPU       | 128.5 +- 0.3 мс   |

*Измерения проводились сериями по 5 на процессоре Intel Core i5-8250U (1.6 GHz)*

Получается разница во времени примерно в **400** раз! Это абсолютно оправдывает ресурсы, потраченные на реализацию бинарной трансляции.


## Установка
Чтобы установить используйте следующие команды:

``` bash
git clone --recursive https://github.com/crefr/language
cd language
make BUILD=RELEASE
```

В папке появятся исполняемые файлы `frontend.exe`, `middleend.exe` и `backend.exe`.


Если вы хотите попробовать запустить что-нибудь на SPU, понадобится следующее:
```bash
cd processor
make BUILD=RELEASE
cd ..
```

## Использование

### X86-64

Самый легкий способ:
```bash
cd backend_x64
make compile FILE=code_file_name.txt
```

В папке `compiled` появится файл `code_file_name.elf`. Это исполняемый файл, его уже можно запускать.


### SPU

Легче всего скопировать код программы в файл `program.txt` и запустить скрипт `launch.sh`. Он должен скомпилировать и запустить программу на SPU.

### Вручную
Если для каких-то целей вам понадобится скомпилировать программу в ручном режиме (до бэкенда):

1. **Фронтенд**

    Сначала нужно транслировать в промежуточное представление (IR):
    ```bash
    ./frontend.exe program.txt out_ir.ast
    ```
2. **Миддленд** (optional)

    Можно слегка оптимизировать программу:
    ```bash
    ./middleend.exe program_IR.ast
    ```
    Результат оптимизации будет перезаписан в изначальный файл

### Обратный фронтенд

Если вы желаете транслировать промежуточное представление обратно в код, используйте:
```bash
frontend.exe -1 program_IR.ast source_code.txt
```

## Грамматика

Ниже представлена грамматика языка в БНФ-подобной форме (вы также можете найти ее в файле [`grammar.txt`](grammar.txt)):

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

Также эта грамматика доступна на **BNF Playground** (the [link](https://bnfplayground.pauliankline.com/?bnf=%3CRESULT%3E%20%20%3A%3A%3D%20%3CChain%3E%0A%0A%3CChain%3E%20%20%20%3A%3A%3D%20(%3CStatement%3E%20%22%3B%22)*%0A%3CStatement%3E%20%3A%3A%3D%20%3CFuncCall%3E%20%7C%20%3CSTDfunc%3E%20%7C%20%3CReturn%3E%20%7C%20%3CIF%3E%20%7C%20%3CWhile%3E%20%7C%20%3CVarDecl%3E%20%7C%20%3CFuncDecl%3E%20%7C%20%3CAssign%3E%0A%0A%3CIF%3E%20%20%20%20%20%20%3A%3A%3D%20%22if%22%20%22(%22%20%3CExpr%3E%20%22)%22%20%3CBlock%3E%20(%3CELSE%3E)%3F%0A%3CELSE%3E%20%20%20%20%3A%3A%3D%20%22else%22%20%3CBlock%3E%0A%0A%3CWhile%3E%20%20%20%3A%3A%3D%20%22while%22%20%22(%22%20%3CExpr%3E%20%22)%22%20%3CBlock%3E%0A%0A%3CVarDecl%3E%20%3A%3A%3D%20%22var%22%20%3CID%3E%0A%0A%3CFuncDecl%3E%20%3A%3A%3D%20%22func%22%20%3CID%3E%20%22(%22%20%3CVar%3E%20(%22%2C%22%20%3CVar%3E)*%20%22)%22%20%3CBlock%3E%0A%3CBlock%3E%20%20%20%3A%3A%3D%20%22begin%22%20%3CChain%3E%20%22end%22%0A%0A%3CAssign%3E%20%20%3A%3A%3D%20%3CVar%3E%20%22%3D%22%20%3CExpr%3E%0A%3CSTDfunc%3E%20%3A%3A%3D%20%3CInput%3E%20%7C%20%3COutput%3E%0A%0A%3CReturn%3E%20%20%3A%3A%3D%20%22return%22%20%3CExpr%3E%0A%0A%3CInput%3E%20%20%20%3A%3A%3D%20%22in%22%20%20%22(%22%20%3CVar%3E%20%22)%22%0A%3COutput%3E%20%20%3A%3A%3D%20%22out%22%20%22(%22%20%3CExpr%3E%20%22)%22%0A%0A%3CExpr%3E%20%20%20%20%3A%3A%3D%20%3CAddSub%3E%20((%22%3E%22%20%7C%20%22%3C%22)%20%3CAddSub%3E)*%0A%0A%3CAddSub%3E%20%20%3A%3A%3D%20%3CMulDiv%3E%20((%22%2B%22%20%7C%20%22-%22)%20%3CMulDiv%3E)*%0A%3CMulDiv%3E%20%20%3A%3A%3D%20%3CPower%3E%20%20((%22*%22%20%7C%20%22%2F%22)%20%3CPower%3E)*%0A%0A%3CPower%3E%20%20%20%3A%3A%3D%20%3CPrimary%3E%20(%22%5E%22%20%3CPower%3E)%3F%0A%3CPrimary%3E%20%3A%3A%3D%20%22(%22%20%3CExpr%3E%20%22)%22%20%7C%20%3CMathFunc%3E%20%7C%20%3CFuncCall%3E%20%7C%20%3CVar%3E%20%7C%20%3CNumber%3E%0A%0A%3CFuncCall%3E%20%3A%3A%3D%20%3CID%3E%20%22(%22%20%3CExpr%3E%20(%22%2C%22%20%3CExpr%3E)*%20%22)%22%0A%3CVar%3E%09%20%20%3A%3A%3D%20%3CID%3E%0A%0A%3CID%3E%09%20%20%3A%3A%3D%20(%5Ba-z%5D%20%7C%20%22_%22)%20(%5Ba-z%5D%20%7C%20%5B0-9%5D%20%7C%20%22_%22)*%0A%3CNumber%3E%20%20%3A%3A%3D%20%5B0-9%5D%2B%0A%3CMathFunc%3E%20%3A%3A%3D%20%22sin%22%20%7C%20%22cos%22%20%7C%20%22tan%22%20%7C%20%22ln%22&name=))


