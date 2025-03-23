# Documentation for the IFJ and IAL Project

## Implementation of the IFJ24 Imperative Language Compiler

### Team xlucnyj00, Variant TRP-izp

- **Jakub Lůčný** (team leader) - xlucnyj00 - 25%
- **Jakub Hrdlička** - xhrdli18 - 25%
- **Rebeka Tydorová** - xtydor01 - 25%
- **Martin Ševčík** - xsevcim00 - 25%

## Table of Contents

1. Introduction
2. Team Work Distribution
3. Implementation Description
4. Lexical Analysis
5. Syntax Analysis
6. Semantic Analysis
7. Code Generation
8. Data Structures Used
9. Implementation Files Structure
10. Conclusion
11. Tables and Diagrams

## 1. Introduction

The goal of this project was to design and implement a compiler for the IFJ24 language, which translates source code written in this language into the target language IFJcode24. This compiler is built in the C programming language, with implementation covering all basic compilation phases: lexical analysis, syntax analysis, semantic analysis, and target code generation. The project was carried out as teamwork, with individual team members participating in different parts of the implementation.

## 2. Team Work Distribution

### 2.1 Jakub Lůčný (xlucnyj00)
Co-author of semantic analysis, code generation and AST, testing

### 2.2 Jakub Hrdlička (xhrdli18)
Syntax analysis, Symbol table, Token structure, etc.

### 2.3 Rebeka Tydorová (xtydor01)
Lexical analysis, expression processing

### 2.4 Martin Ševčík (xsevcim00)
Co-author of semantic analysis, code generation and AST

## 3. Implementation Description

The project solution consists of several parts. The syntax analysis implemented in the syntakticka_analyza.c file could be considered the "main" part that controls everything. Syntax analysis sequentially requests tokens from lexical analysis using the get_token function implemented in lexer.c. This part of the program gradually reads input character by character and divides individual words and characters into tokens defined in token.h. It also uses helper programs such as str_buffer for storing characters sequentially to create words and sentences, or keyword_check to sort token types according to defined keywords.

Syntax analysis then processes these tokens sequentially and checks them for potential syntax errors. It also stores the processed tokens in an abstract syntax tree (AST) implemented in the ast file. For expressions, syntax analysis calls the helper function expression implemented in the expressions file. From this moment until the end of the expression is detected, expression takes control. It requests tokens itself, checks their correctness, and generates a binary tree of the given expression. The expressions file uses supporting programs such as btree, bts_stack, and string_stack for its function. The resulting tree is then converted to postfix notation and inserted into the AST.

After reading all tokens, syntax analysis calls the semantic_analysis function implemented in the semantics file. Here the compiled code is traversed again and checked for semantic errors. The code is no longer loaded using the get_token function but is read from the AST. This second pass also stores individual functions, variables, and constants of the compiled code in a symbol table implemented in the symtable file. The symbol table consists of a hash table implemented in the hashtable file and a stack implemented in the symtable_stack file. Semantic analysis also uses this symbol table to check the semantics of individual functions, variables, and constants.

Finally, syntax analysis calls the generate_code function implemented in the codegen file. This generator gradually reads the individual tokens stored in the AST and generates the resulting translated code based on them.

## 4. Lexical Analysis

Lexical analysis is implemented using a finite automaton, which is represented in the C language using a switch control structure. We read lexical units from standard input, from which we then create tokens. Tokens carry type and value, which are determined based on the current state of the lexical analyzer. The entire lexical analysis is wrapped in the get_token() function, which returns exactly one token when requested.

## 5. Syntax Analysis

Syntax analysis could be viewed as the main controller of the entire compiler. It sequentially reads tokens from lexical analysis and checks the syntactic correctness of the input program based on a predefined grammar. The correctness of the input program is verified using recursive descent, meaning the program gradually goes deeper and deeper into its functions that mirror the grammar. When it's time for an expression, whether in variable assignment, condition, or return value, syntax analysis passes control to expressions via the expression function. Control returns to syntax analysis at the end of the expression with the first token after the expression. During syntax analysis, checked tokens are stored in an abstract syntax tree, which is later used for semantic checking and code generation. After fully verifying the syntactic correctness of the input code, syntax analysis calls a function to start semantic analysis, then a function to start code generation, and successfully terminates the program.

### 5.1 Precedence Analysis of Expressions

We process expressions using precedence syntax analysis, which is governed by a precedence table and a set of context-free rules defining permitted operations and their correct combinations. The precedence table is used to compare an operator from the input with an operator at the top of the stack. Based on their relationship in the table, shift operations (moving a token to the stack) and reduce operations (applying rules and reducing the expression) are performed. During reduction, tokens and partial results from the stack are combined into binary tree nodes, where each node represents an operation and its subtrees correspond to operands.

## 6. Semantic Analysis

Semantic analysis traverses the Abstract Syntax Tree (AST) created during syntactic analysis. For AST traversal, the next_node() function is used. It is implemented in two passes: during the first pass, only declarations of all user and built-in functions are stored in the symbol table, and during the second pass, all variables and constants are stored in the symbol table, which then serve for type checking of expressions, assignments, usage checks, modifications, etc.

During the second pass, in addition to the above-mentioned actions, references to undefined functions or variables, wrong number or type of arguments when calling functions, wrong type or impermissible discarding of function return values, missing or excessive expressions in return statements, redefinition of variables or functions, and the possibility of inferring variable types during their definition without a defined type are checked.

For clearer implementation, a global variable current_function_name is also used, which stores the name of the function whose definition is currently being semantically checked and serves to check the type of the returned expression or possibly check for unauthorized missing return keyword.

Compared to the assignment, our semantic analysis allows type conversions of both variables and expressions with values unknown at compile time, thanks to the fact that type conversion takes place dynamically during code generation.

## 7. Code Generation

The generator proceeds similarly to the semantic analyzer, traversing the Abstract Syntax Tree using the next_node() function and printing instructions of the target language IFJcode to standard output.

At the beginning of the generation itself, code is generated that creates helper variables in the global frame of the interpreter.

After generating the entire code given by the Abstract Syntax Tree, the code of built-in user functions is generated.

All type conversions occur during interpretation. This means during generation, code is generated that checks whether the operands are of the same type (int, float - type conversions between other types are not allowed and are detected during semantic checks) and if the types do not match, the integer value is converted to float.

For greater code clarity, a global variable current_function_name is used, which is used to generate code for dynamic checking that prevents the error of variable redefinition when defining a variable, for example, in the middle of a while loop that is traversed more than once.

## 8. Data Structures Used

### 8.1 Symbol Table

The symbol table consists of two basic parts, a hash table and a stack. Individual functions, variables, and constants are thus inserted directly into the hash table, each hash table itself representing one scope. When entering a new scope, the current table is stored at the top of the stack and a new table is created for the new scope. When exiting a scope, conversely, the current table is discarded and the table from the top of the stack is set as the current table. Both the hash table and the stack used in the symbol table are dynamic, so they cannot overflow.

### 8.2 Abstract Syntax Tree

The implementation of our AST changed several times during development, finally ending up as a degenerated binary tree that consists of only one branch. That is, all nodes have only a right son. So basically it's a singly linked list.

### 8.3 Dynamic Buffer

A dynamic buffer is used in lexical analysis for the ability to store gradually read characters into words or sentences with a previously unknown length.

## 9. Implementation Files Structure

- Lexical analyzer: **lexer.c**, lexer.h, token.h
- Helper structures for lexical analysis: str_buffer.c, str_buffer.h
- Keyword recognition: **keyword_check.c**, keyword_check.h
- Syntax analyzer: **syntakticka_analyza.c**
- Expression analysis: **expression.c**, expression.h
- Helper structures for expression analysis: btree.c, btree.h, bts_stack.c, bts_stack.h, string_stack.c, string_stack.h
- Semantic analysis: **semantics.c**, semantics.h
- Code generation: **codegen.c**, codegen.h
- Symbol table: **hashtable.c**, hashtable.h, symtable.c, symtable.h, symtable_stack.c, symtable_stack.h
- Abstract syntax tree: **ast.c**, ast.h

## 10. Conclusion

This project provided a comprehensive view of compiler implementation, from lexical analysis to target code generation. Thanks to a clear division of tasks within the team and well-designed data structures, we successfully created a program that can process code in the IFJ24 language and translate it into the IFJcode24 format. The result is a functional compiler that meets the requirements of the assignment to the extent we were able to achieve within the given time frame and with available resources. Although some parts of the compiler do not work 100%, the final product provides a solid foundation for further extension and improvement. This project allowed us to gain valuable experience with the design and implementation of larger software systems and deepened our understanding of the basic principles of programming languages and compilers.

## 11. Tables and Diagrams

- See in **dokumentace.pdf**
