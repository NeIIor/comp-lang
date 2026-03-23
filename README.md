# CMM Compiler / Компилятор CMM  

- [English Version](#english-version)
- [Русская версия](#русская-версия)

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
8. [Документация кода (Doxygen)](#документация-кода-doxygen)  
9. [Подробности реализации](#подробности-реализации)  

---

## Обзор  
Компилятор CMM — это компилятор для языка, подобного C, который генерирует **32-битный PE** для Windows (x86). Сборка самого компилятора и бэкенд ориентированы на **Windows** и MinGW. Он включает в себя:  
- Синтаксический анализ интерфейса  
- Оптимизацию AST  
- Генерация внутреннего кода  
- Основные функции стандартной библиотеки  

Для простоты компилятор работает исключительно с 32-разрядными целыми значениями**.  

Если вы хотите ознакомиться с внутренней структурой исполняемых файлов PE Windows, вы можете воспользоваться русскоязычной [статьей](https://habr.com/ru/articles/266831/) или англоязычной [статьей](https://0xrick.github.io/win-internals/pe2/), разделенной на 6 частей.

---

## Архитектура компилятора  
Компилятор состоит из следующих частей:  

1. **Фронт-энд** (`cmm_frontend`)  
   - Выполняет синтаксический анализ 
   - Преобразует исходный код в AST  (с помощью рекурсивного спуска) 

2. **Миддл-энд** (`lang_optimizer`)  
   - Свертка констант   
   - Удаление операций с нейтральными элементами  

3. **Бэк-энд** (`lang_compile`)  
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
    print(z);  
}  
```  
</details>  

Ключевые функции:  
- Переменные и назначения  
- Арифметические операции (`+`, `-`, `*`, `/`)  
- Управление потоком (`if`, `while`)  
- Функции (определяемые пользователем)  
- Операции ввода-вывода (`scan`, `print`)  

---

## Стандартная библиотека  
Встроенные функции:  
- `scan()` / `scan(x)` — считывает целое число (в рантайме Windows — через консольный ввод, см. раздел «Запуск output.exe» ниже)  
- `print(expr)` — выводит целое число  
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
- **ОС**: сборка и запуск инструментов — **Windows** (бэкенд использует `windows.h`).  
- **CMake** (≥ 3.8)  
- **MinGW-w64** с `gcc` / `g++` в `PATH` (удобно: [MSYS2](https://www.msys2.org/) UCRT64 или аналог)  

Пересборка `sfasmlib.dll` из исходников в репозитории не входит в CMake; для работы достаточно уже лежащей в `sfasmlib/sfasmlib.dll`, которую CMake копирует в каталог `build`.

### Этапы построения  

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build .
```

На 64-битном MinGW **без** multilib сборка **самих** инструментов компилятора с `-m32` не найдёт 32-битные системные библиотеки. По умолчанию хост-сборка **без** `-m32`; сгенерированный **`output.exe`** всё равно остаётся **32-битным PE**. Нужен именно 32-битный хост-компилятор инструментов: `cmake .. -DCMM_32BIT_HOST=ON` (требуются библиотеки i686/multilib).

---

## Пример использования  
### Конвейер компиляции  
После сборки **перейдите в каталог `build`** (там же должны лежать `sfasmlib.dll` и скомпилированные утилиты).

Ниже — **не одна строка**, а **три последовательных команды**: каждую вводите в терминале **отдельно** и нажимайте **Enter**; дождитесь строки `OK. ...` перед следующей. Так вы по очереди: (1) из `.cmm` получаете `ast.tree`, (2) из него — `optimized.tree`, (3) собираете `output.exe`.

**cmd** (текущая папка — `build`):  
```bat
cmm_frontend.exe ..\example\program.cmm ast.tree
lang_optimizer.exe ast.tree optimized.tree
lang_compile.exe optimized.tree output.exe
```

**PowerShell** (часто нужно явно указать программу в текущей папке — префикс `.\`):  
```powershell
.\cmm_frontend.exe ..\example\program.cmm ast.tree
.\lang_optimizer.exe ast.tree optimized.tree
.\lang_compile.exe optimized.tree output.exe
```

В **cmd** то же можно одной строкой через `&&`:  
```bat
cmm_frontend.exe ..\example\program.cmm ast.tree && lang_optimizer.exe ast.tree optimized.tree && lang_compile.exe optimized.tree output.exe
```

**Linux / Git Bash** (те же стадии, без `.exe`):  
```bash
./cmm_frontend ../example/program.cmm ast.tree
./lang_optimizer ast.tree optimized.tree
./lang_compile optimized.tree output.exe
```

*Примечание:* бэкенд `lang_compile` зависит от `windows.h`; на Linux без MinGW-кросса под Windows эту стадию обычно не собирают — ориентир в этом README — **Windows**.

### Что за `.exe` в `build`  
| Файл | Назначение |
|------|------------|
| `cmm_frontend.exe` | Исходник `.cmm` → текстовый файл AST |
| `lang_optimizer.exe` | AST → оптимизированное AST |
| `lang_compile.exe` | AST → исполняемый **`output.exe`** |
| `cmm_frontend_rev.exe` | AST → псевдокод (вспомогательно, для отладки) |

Имеет смысл **запускать** как программу только **`output.exe`**; остальные — этапы конвейера.

### Запуск `output.exe`  
- Положите **`sfasmlib.dll`** в **ту же папку**, что и `output.exe` (в `build` это уже так после CMake).  
- Запускайте из **окна консоли** (`cmd` или PowerShell): `output.exe`.  
- Ввод для `scan` / `scan(x)` и вывод для `print` реализованы в **`sfasmlib`** через **`ReadConsole`** / **`WriteConsole`**, а не через классический перенаправленный stdin/stdout. Надёжный способ проверки — **ввести число с клавиатуры** в открытой консоли. Перенаправление вроде `echo 5 \| output.exe` или вывод в файл может вести себя не так, как ожидается.  

Результат компиляции — файл **`output.exe`** в текущем каталоге.  

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
{ function-declaration { $main { nil }{ concatenation { $scan { x { nil }{ nil }}{ nil }}{ concatenation { = { x { nil }{ nil }}{ $factorial { concatenation { nil }{ x { nil }{ nil }}}{ nil }}}{ concatenation { = { i { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { = { j { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { while { < { i { nil }{ nil }}{ 10 { nil }{ nil }}}{ concatenation { = { j { nil }{ nil }}{ 0 { nil }{ nil }}}{ concatenation { while { < { j { nil }{ nil }}{ 10 { nil }{ nil }}}{ concatenation { $print { + { * { 10 { nil }{ nil }}{ i { nil }{ nil }}}{ j { nil }{ nil }}}{ nil }}{ concatenation { = { j { nil }{ nil }}{ + { j { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}}{ concatenation { = { i { nil }{ nil }}{ + { i { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}}}{ concatenation { $print { x { nil }{ nil }}{ nil }}{ nil }}}}}}}}{ function-declaration { $factorial { x { nil }{ nil }}{ concatenation { if { <= { x { nil }{ nil }}{ 1 { nil }{ nil }}}{ concatenation { return { 1 { nil }{ nil }}{ nil }}{ nil }}}{ concatenation { return { * { x { nil }{ nil }}{ $factorial { concatenation { nil }{ - { x { nil }{ nil }}{ 1 { nil }{ nil }}}}{ nil }}}{ nil }}{ nil }}}}{ nil }}}  (добавить картинку)
```  
</details>  

---

## Инструменты разработки  
Рекомендуемые инструменты для разработки:  
- **Отладка**: GDB, Visual Studio (для `output.exe` и утилит)  
- **Бинарный анализ**: `objdump -d output.exe` (MinGW), IDA Free  
- **Сборка**: CMake, MinGW  

### Документация кода (Doxygen)  
При желании можно собрать справку по коду с [Doxygen](https://www.doxygen.nl/): создайте локальный **`Doxyfile`** (например `doxygen -g Doxyfile`) и настройте под проект; конфиг в репозиторий **не включается**.  

Команда из корня проекта (нужен установленный `doxygen` и файл `Doxyfile`):  
```bash
doxygen Doxyfile
```  

Результат: каталог **`html/`** (откройте **`html/index.html`** в браузере) и **`latex/`** (исходники LaTeX для сборки PDF при необходимости). Это **сгенерированные** файлы; они перечислены в **`.gitignore`**.

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
   - Импорт из `sfasmlib.dll`  

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
- Только 32-разрядная целевая программа 

---


<a name="english-version"></a>
## English Version

The CMM toolchain targets **32-bit Windows PE** executables. Build the compiler on **Windows** with MinGW; the backend relies on `windows.h`. By default the **compiler tools** are 64-bit; the generated **`output.exe`** is still a 32-bit PE. Use `cmake .. -DCMM_32BIT_HOST=ON` only if you need the tools themselves built as 32-bit (`-m32`) and your GCC has i686/multilib.

## Table of Contents  
1. [Overview](#overview)  
2. [Compiler Architecture](#compiler-architecture)  
3. [Language Features](#language-features)  
4. [Standard Library](#standard-library)  
5. [Build Instructions](#build-instructions)  
6. [Usage Examples](#usage-examples)  
7. [Development Tools](#development-tools)  
8. [API Documentation (Doxygen)](#api-documentation-doxygen)  
9. [Implementation Details](#implementation-details)  

---

## Overview  
The CMM compiler is a **C-like** language implementation that emits **32-bit Windows PE** (x86) executables. The toolchain is built and intended to run on **Windows** with MinGW. It provides:  
- Frontend parsing  
- AST optimization  
- Backend code generation  
- Basic standard library functions  

The compiler operates exclusively on **32-bit integer values** for simplicity.  

For PE internals, see the Russian [article](https://habr.com/ru/articles/266831/) or the English [series](https://0xrick.github.io/win-internals/pe2/) (6 parts).

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
- `scan()` / `scan(x)` — reads an integer (Windows runtime uses the console; see [Running output.exe](#usage-examples))  
- `print(expr)` — prints an integer  
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
- **Host OS**: **Windows** for building and running the toolchain (backend uses `windows.h`).  
- **CMake** (≥ 3.8)  
- **MinGW-w64** with `gcc`/`g++` on `PATH` (e.g. [MSYS2](https://www.msys2.org/) UCRT64)  

Rebuilding `sfasmlib.dll` from the bundled assembly is **not** part of CMake; the prebuilt DLL in `sfasmlib/` is copied into `build` automatically.

### Build Steps  
```bash  
mkdir build  
cd build  
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++  
cmake --build .  
```  

On 64-bit MinGW **without** multilib, a host build with `-m32` will fail to link system libraries. The default configuration builds **64-bit tools**; the emitted **`output.exe`** is still **32-bit PE**. To build the **tools** as 32-bit: `cmake .. -DCMM_32BIT_HOST=ON` (requires i686/multilib).

---

## Usage Examples  
### Compilation Pipeline  
After building, **change to the `build` directory** (where `sfasmlib.dll` and the tools live).

The following is **three separate commands**, not one line of text. Run **each line** and press **Enter**; wait for an `OK. ...` message before the next. In order: (1) `.cmm` → `ast.tree`, (2) → `optimized.tree`, (3) → `output.exe`.

**cmd** (current directory is `build`):  
```bat
cmm_frontend.exe ..\example\program.cmm ast.tree
lang_optimizer.exe ast.tree optimized.tree
lang_compile.exe optimized.tree output.exe
```

**PowerShell** (programs in the current folder usually need the `.\` prefix):  
```powershell
.\cmm_frontend.exe ..\example\program.cmm ast.tree
.\lang_optimizer.exe ast.tree optimized.tree
.\lang_compile.exe optimized.tree output.exe
```

**cmd** one-liner with `&&`:  
```bat
cmm_frontend.exe ..\example\program.cmm ast.tree && lang_optimizer.exe ast.tree optimized.tree && lang_compile.exe optimized.tree output.exe
```

**Linux / Git Bash** (same stages, no `.exe`):  
```bash
./cmm_frontend ../example/program.cmm ast.tree
./lang_optimizer ast.tree optimized.tree
./lang_compile optimized.tree output.exe
```

*Note:* the `lang_compile` backend includes `windows.h`. Without a Windows-targeting MinGW cross toolchain, you typically do not build that stage on Linux — this README assumes **Windows** for the full pipeline.

### Executables in `build`  
| File | Role |
|------|------|
| `cmm_frontend.exe` | `.cmm` source → AST text file |
| `lang_optimizer.exe` | AST → optimized AST |
| `lang_compile.exe` | AST → **`output.exe`** |
| `cmm_frontend_rev.exe` | AST → pseudocode (debugging / inspection) |

Only **`output.exe`** is meant to be run as an application; the others are pipeline stages.

### Running `output.exe`  
- Keep **`sfasmlib.dll`** in the **same folder** as `output.exe` (true by default in `build` after CMake).  
- Run from a **console window** (`cmd` or PowerShell): `output.exe`.  
- I/O for `scan` / `print` is implemented in **`sfasmlib`** with **`ReadConsole`** / **`WriteConsole`**, not classic redirected stdin/stdout. Prefer **typing input in an interactive console**. Pipes such as `echo 5 | output.exe` or file redirection may not behave like a normal C program.  

The compiler output is **`output.exe`** in the current directory.


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
- **Debugging**: GDB, Visual Studio (for `output.exe` and the tools)  
- **Binary analysis**: `objdump -d output.exe` (MinGW), IDA Free  
- **Build**: CMake, MinGW  

### API Documentation (Doxygen)  
Optional [Doxygen](https://www.doxygen.nl/) API docs: create a local **`Doxyfile`** (e.g. `doxygen -g Doxyfile`) and tune it; the config is **not** tracked in this repository.  

From the project root (with `doxygen` installed and a `Doxyfile` present):  
```bash
doxygen Doxyfile
```  

Output: **`html/`** (open **`html/index.html`** in a browser) and **`latex/`** (LaTeX sources for an optional PDF build). These directories are **generated** and are normally **not** committed — they are listed in **`.gitignore`**.

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
   - Imports from `sfasmlib.dll`  

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
