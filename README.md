---

# CMM Compiler  

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

---

## Compiler Architecture  
The compilation process follows these stages:  

1. **Frontend** (`cmm_frontend`)  
   - Parses source code into AST  
   - Performs syntax analysis  

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
