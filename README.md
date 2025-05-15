# CMM Compiler / Компилятор CMM  

- [English Version](#english-version)
- [Русская версия](#русская-версия)

---

<a name="english-version"></a>
## English Version

This is the English version of the documentation. It describes the structure and usage of the compiler in detail.

(In programm i have lib windows.h, so you can compile only on Windows and i have flag -m32 because i have 32-bit compiler g++, and if you have 64-bit
you can just delete 8 line in CMakeLists.txt)



 

## Table of Contents  
1. [Overview](#overview)  
2. [Compiler Architecture](#compiler-architecture)  
3. [Language Features](#language-features)  
4. [Standard Library](#standard-library)  
5. [Build Instructions](#build-instructions)  
6. [Usage Examples](#usage-examples)  
7. [Development Tools](#development-tools)  
8. [Implementation Details](#implementation-details)  

---

## Overview  
The CMM Compiler is a 32-bit compiler for a C-like mathematical language that compiles to x86 assembly. It features:  
- Frontend parsing  
- AST optimization  
- Backend code generation  
- Basic standard library functions  

The compiler operates exclusively on **32-bit integer values** for simplicity.  

If you want to get acquainted with the internal structure of elf files, you can use the Russian-language [article](https://habr.com/ru/articles/266831/) or the English-language [article](https://0xrick.github.io/win-internals/pe2/) divided into 6 parts.

---

## Compiler Architecture  
The compilation process follows these stages:  

1. **Frontend** (`cmm_frontend`)  
   - Performs syntax analysis 
   - Parses source code into AST (recursive descent parsing) 

2. **Optimizer** (`lang_optimizer`)  
   - Constant folding  
   - Arithmetic optimization  
   - Algebraic simplification  

3. **Backend** (`lang_compile`)  
   - Creates PE executable  

---

## Language Features  
<details>  
<summary>Basic Syntax Example</summary>  

```c  
main  
{  
    x = 5;  
    y = scan();  
    z = x + y * 2;  
    print(z);  
}  
```  
</details>  

Key features:  
- Variables and assignments  
- Arithmetic operations (`+`, `-`, `*`, `/`)  
- Control flow (`if`, `while`)  
- Functions (user-defined)  
- I/O operations (`scan`, `print`)  

---

## Standard Library  
Built-in functions:  
- `scan()` - Reads integer from input  
- `print(num)` - Outputs integer  
- Math functions: `sin`, `cos`, `pow`, etc.  

<details>  
<summary>Function Example</summary>  

```c  
square(x)  
{  
    return x * x;  
}  

main  
{  
    a = 5;  
    b = square(a);  
    print(b);  // Outputs 25  
}  
```  
</details>  

---

## Build Instructions  
### Prerequisites  
- CMake (≥ 3.8)  
- MinGW (gcc/g++)  
- NASM32 (for assembly)  

### Build Steps  
```bash  
mkdir build  
cd build  
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++  
cmake --build .  
```  

---

## Usage Examples  
### Compilation Pipeline  
```bash  
./cmm_frontend program.cmm ast.tree  
./lang_optimizer ast.tree optimized.tree  
./lang_compile optimized.tree output.exe  
```  

<details>  
<summary>Compilation Example</summary>  

**Input (program.cmm):**  
```c  
main
{
    scan(x);

    x = factorial(x);

    i = 0;
    j = 0;
    
    while(i < 10)
    {
        j = 0;

        while(j < 10)
        {
            print(10 * i + j);
            j = j + 1;
        }

        i = i + 1;    
    }

    print(x);
}

factorial(x)
{
    if (x <= 1)
    {
        return 1;
    }

    return x * factorial(x - 1);
}
```  

**Optimized AST:**  
```  
{ function-declaration { $main { nil }{ concatenation { $scan { x { nil }{ nil }}{ nil }}{ concatenation { = { x { nil }{ nil }}{ $factorial { concatenation { nil }{ x { nil }{ nil }}}{ nil }}}{ concatenation { = { i { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { = { j { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { while { < { i { nil }{ nil }}{ 10 { nil }{ nil }}}{ concatenation { = { j { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { while { < { j { nil }{ nil }}{ 10 { nil }{ nil }}}{ concatenation { $print { + { * { 10 { nil }{ nil }}{ i { nil }{ nil }}}{ j { nil }{ nil }}}{ nil }}{ concatenation { = { j { nil }{ nil }}{ + { j { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}}{ concatenation { = { i { nil }{ nil }}{ + { i { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}}}{ concatenation { $print { x { nil }{ nil }}{ nil }}{ nil }}}}}}}}{ function-declaration { $factorial { x { nil }{ nil }}{ concatenation { if { <= { x { nil }{ nil }}{ 1 { nil }{ nil }}}{ concatenation { return { 1 { nil }{ nil }}{ nil }}{ nil }}}{ concatenation { return { * { x { nil }{ nil }}{ $factorial { concatenation { nil }{ - { x { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}{ nil }}{ nil }}}}{ nil }}}  
```  
</details>  

---

## Development Tools  
Recommended tools for development:  
- **Debugging**:  
  - GDB (EDB)  

- **Binary Analysis**:  
  - `objdump -d output.exe`
  - readelf   
  - IDA Freeware  

- **Assembly**:  
  - NASM32  
  - ELF
  - CMake  

---

## Implementation Details  

### Output EXE File Structure  
The compiler generates **32-bit PE executables** with:  
1. **Text Section**:  
   - Contains x86 opcodes (e.g., `mov`, `add`, `call`)  
   - Direct stack-based variable access (`[edi+offset]`)  

2. **Data Section**:  
   - Global variables storage  
   - Hardcoded function addresses (e.g., `print_number`)  

3. **Import Table**:  
   - Links to my library ('sfasmlib.dll)  

Example disassembly snippet:  
```asm  
; Main program  
mov edi, 0x1000     ; Stack frame setup  
mov ebx, 5          ; Load constant  
mov [edi+4], ebx    ; Store variable  
call 0x401000       ; Call print function  
```  

### Key Components  
1. **Abstract Syntax Tree (AST)**  
   - Tree-based intermediate representation  
   - Optimized during compilation  

2. **Code Generation**  
   - Direct x86 opcode emission  
   - PE executable format  

3. **Memory Model**  
   - Stack-based variables  
   - Global data section  

### Limitations  
- Integer-only arithmetic  
- Basic type system  
- 32-bit target only  

--- 

## Русская версия  

# Компилятор CMM  

## Оглавление  
1. [Обзор](#обзор)  
2. [Архитектура компилятора](#архитектура-компилятора)  
3. [Возможности языка](#возможности-языка)  
4. [Стандартная библиотека](#стандартная-библиотека)  
5. [Инструкции по сборке](#инструкции-по-сборке)  
6. [Пример использования](#пример-использования)  
7. [Инструменты разработки](#инструменты-разработки)  
8. [Подробности реализации](#подробности-реализации)  

---

## Обзор  
Компилятор CMM - это 32-разрядный компилятор для математического языка, подобного C, который компилируется в ассемблере x86. Он включает в себя:  
- Синтаксический анализ интерфейса  
- Оптимизацию AST  
- Генерация внутреннего кода  
- Основные функции стандартной библиотеки  

Для простоты компилятор работает исключительно с 32-разрядными целыми значениями**.  

Если вы хотите ознакомиться с внутренней структурой файлов elf, вы можете воспользоваться русскоязычной [статьей](https://habr.com/ru/articles/266831/) или англоязычной [статьей](https://0xrick.github.io/win-internals/pe2/), разделенной на 6 частей.

---

## Архитектура компилятора  
Процесс компиляции состоит из следующих этапов:  

1. **Интерфейс** (`cmm_frontend`)  
   - Выполняет синтаксический анализ 
   - Преобразует исходный код в AST  (рекурсивный спуск) 

2. **Оптимизатор** (`lang_optimizer`)  
   - Сворачивание констант  
   - Арифметическая оптимизация  
   - Алгебраическое упрощение  

3. **Серверная часть** (`lang_compile`)  
   - Создает исполняемый файл PE  

---

## Возможности языка 
<details>  
<summary>Пример основного синтаксиса</summary>  

```
main  
{  
    x = 5;  
    y = scan();  
    z = x + y * 2;  
    печать(z);  
}  
```  
</details>  

Ключевые функции:  
- Переменные и назначения  
- Арифметические операции (`+`, `-`, `*`, `/`)  
- Управление потоком (`if`, `while`)  
- Функции (определяемые пользователем)  
- Операции ввода-вывода (`сканирование`, `печать`)  

---

## Стандартная библиотека  
Встроенные функции:  
- "scan()" - Считывает целое число из входных данных  
- `print(num)" - Выводит целое число  
- Математические функции: `sin`, `cos`, `pow` и т.д.  

<details>  
<summary> Пример функции</summary>  

```c  
square(x)  
{  
    return x * x;  
}  

main  
{  
    a = 5;  
    b = square(a);  
    print(b);  // Outputs 25  
}  
```  
</details>  

---

## Инструкции по сборке  
### Предварительные требования  
- CMake (≥ 3.8)  
- MinGW (gcc/g++)  
- NASM32 (для сборки)  

### Этапы построения  

``` 
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake -build .  
```  

---

## Пример использования  
### Конвейер компиляции  
```
./cmm_frontend program.cmm ast.tree  
./lang_optimizer ast.tree optimized.tree  
./lang_compile optimized.tree output.exe 
```  

<details>  
<summary>Базовый пример</summary>  

**Ввод (program.cmm):**  
```c  
main
{
    scan(x);

    x = factorial(x);

    i = 0;
    j = 0;
    
    while(i < 10)
    {
        j = 0;

        while(j < 10)
        {
            print(10 * i + j);
            j = j + 1;
        }

        i = i + 1;    
    }

    print(x);
}

factorial(x)
{
    if (x <= 1)
    {
        return 1;
    }

    return x * factorial(x - 1);
}
```  

**Оптимизированное AST:**  
```  
{ function-declaration { $main { nil }{ concatenation { $scan { x { nil }{ nil }}{ nil }}{ concatenation { = { x { nil }{ nil }}{ $factorial { concatenation { nil }{ x { nil }{ nil }}}{ nil }}}{ concatenation { = { i { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { = { j { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { while { < { i { nil }{ nil }}{ 10 { nil }{ nil }}}{ concatenation { = { j { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { while { < { j { nil }{ nil }}{ 10 { nil }{ nil }}}{ concatenation { $print { + { * { 10 { nil }{ nil }}{ i { nil }{ nil }}}{ j { nil }{ nil }}}{ nil }}{ concatenation { = { j { nil }{ nil }}{ + { j { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}}{ concatenation { = { i { nil }{ nil }}{ + { i { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}}}{ concatenation { $print { x { nil }{ nil }}{ nil }}{ nil }}}}}}}}{ function-declaration { $factorial { x { nil }{ nil }}{ concatenation { if { <= { x { nil }{ nil }}{ 1 { nil }{ nil }}}{ concatenation { return { 1 { nil }{ nil }}{ nil }}{ nil }}}{ concatenation { return { * { x { nil }{ nil }}{ $factorial { concatenation { nil }{ - { x { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}{ nil }}{ nil }}}}{ nil }}}  
```  
</details>  

---

## Инструменты разработки  
Рекомендуемые инструменты для разработки:  
- **Отладка**:  
  - GDB (EDB)  

- **Бинарный анализ**:  
  - `objdump -d output.exe`
  - readelf   
  - Бесплатная программа IDA  

- **Сборка**:  
  - NASM32 
  - ELF
  - CMake   

---

## Подробности реализации  

### Структура выходного EXE-файла  
Компилятор генерирует **32-разрядные PE-исполняемые файлы** с помощью:  
1. **Текстовый раздел**:  
   - Содержит коды операций x86 (например, `mov`, `add`, `call`)  
   - Прямой доступ к переменным на основе стека (`[edi+offset]`)  

2. **Раздел данных**:  
   - Хранилище глобальных переменных  
   - Жестко заданные адреса функций (например, `print_number`)  

3. **Таблица импорта**:  
   - Ссылки на мою библиотеку ('sfasmlib.dll')  

Пример фрагмента дизассемблирования:  
```asm  
; Основная программа
mov edi, 0x1000  ; Настройка стекового фрейма
mov ebx, 5       ; Загрузка константы
mov [edi+4], ebx ;
call 0x401000    ; Вызов функции печати  
```  

### Ключевые компоненты  
1. **Абстрактное синтаксическое дерево (AST)**  
   - Промежуточное представление на основе дерева  
   - Оптимизирован во время компиляции  

2. **Генерация кода**  
   - Прямое использование кода операции x86  
   - Формат исполняемого файла PE  

3. **Модель памяти**  
   - Переменные на основе стека  
   - Раздел глобальных данных  

### Ограничения  
- Арифметика только для целых чисел  
- Базовая система типов  
- только 32-разрядная целевая программа 

---