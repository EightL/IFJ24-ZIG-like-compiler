/*
* Project: Implementace překladače imperativního jazyka IFJ24
*
* @author: Jakub Lůčný <xlucnyj00>
* @author: Martin Ševčík <xsevcim00>
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ast.h"
#include "codegen.h"


// Represents number of variable declaration in current function block 
// (used for generating code to avoid variable redeclaration) 
int global_decl_cnt = 0;

// Function declarations:
void generate_initial_values();
void generate_code(AST *ast);
void generate_code_for_line(ASTNode *token_node, AST *ast);
void generate_expression(ASTNode *token_node, AST *ast);
void generate_if_statement(ASTNode *token_node, AST *ast);
void generate_while_loop(ASTNode *token_node, AST *ast);
void generate_variable_declaration(ASTNode *token_node, AST *ast);
void generate_assignment_or_expression(ASTNode *token_node, AST *ast);
void generate_expression_assignment(char *identifier, ASTNode *token_node, AST *ast);
void generate_function_call_assignment(char *identifier, ASTNode *function_call_node, AST *ast);
void generate_string_assignment(char *identifier, char *string);
void generate_function_call(char *function_name, ASTNode *token_node, AST *ast);
char *escape_string(const char *input);
void generate_function_definition(ASTNode *token_node, AST *ast);
void generate_function_return(ASTNode *token_node, AST *ast);
void generate_builtin_functions();


// Generates code to create variables in GF, frame for 'main' and 'call main'
void generate_initial_values(){
    printf(".IFJcode24\n");

    // condition result for if/while statements
    printf("DEFVAR GF@__condition_bool\n");

    // variables for type checking and conversion
    printf("DEFVAR GF@__type_conver_var1\n");
    printf("DEFVAR GF@__type_conver_var2\n");

    printf("DEFVAR GF@__type_conver_type1\n");
    printf("DEFVAR GF@__type_conver_type2\n");

    printf("DEFVAR GF@__type_conver_res\n");

    // variables for checking types of operands in division
    printf("DEFVAR GF@__typecheck_var\n");
    printf("DEFVAR GF@__typecheck_type\n");

    // variable for checking condition in if/while with |extension|
    printf("DEFVAR GF@__extcheck_var\n");
    printf("DEFVAR GF@__extcheck_type\n");

    // variables for checking if variable was already defined
    printf("DEFVAR GF@__decl_cnt\n");
    printf("MOVE GF@__decl_cnt int@0\n");
    printf("DEFVAR GF@__decl_bool\n");

    // global variable for discarding result of a function/expression
    printf("DEFVAR GF@_\n");

    // Main Frame
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    printf("CALL main\n");
    printf("EXIT int@0\n");
}

/********************** MAIN PUBLIC FUNCTION ***************************/
void generate_code(AST *ast){
    ast->active = ast->root;

    generate_initial_values();

    // Without while, the generation ends after first function definition
    while (ast->active->token->type != eof_token){
        generate_code_for_line(ast->active, ast); // starts the generation

        next_node(ast); // skip '}'
    }

    // Generate language built-in functions
    generate_builtin_functions();
}

// Based on current node in AST chooses what's gonna be generated
// Ends after whole function definition was generated
void generate_code_for_line(ASTNode *token_node, AST *ast){
    // Loop until we reach end of block or "EOF"
    while(token_node->token->type != eof_token && (strcmp(token_node->token->data, "}") != 0) && (strcmp(token_node->token->data, "return") != 0)){

        if (strcmp(token_node->token->data, "var") == 0 || strcmp(token_node->token->data, "const") == 0){
            generate_variable_declaration(token_node, ast);
        }
        else if (strcmp(token_node->token->data, "if") == 0){
            generate_if_statement(token_node, ast);
        }
        else if (strcmp(token_node->token->data, "while") == 0){
            generate_while_loop(token_node, ast);
        }
        else if (strcmp(token_node->token->data, "pub") == 0){
            generate_function_definition(token_node, ast);
        }
        else{
            generate_assignment_or_expression(token_node, ast);
        }

        token_node = ast->active; // updates token to be up to date
    }
}

// Generates code to perform expression
void generate_expression(ASTNode *token_node, AST *ast){
    char *current_token_data = token_node->token->data;
    int current_token_type = token_node->token->type;

    // Static variables to keep track of number of operations for unique labels
    static int bi_operations_counter = 0;
    static int div_counter = 0;

    // Every expression ends ';' or ')'
    while (strcmp(current_token_data, ";") != 0 && strcmp(current_token_data,")") != 0){

        // +    -   *   /   <   <=  >   >= 
        // Generates code to check if operands are same types, if not does the necessary conversions
        if (current_token_type == binary_operator_token || current_token_type == relational_operator_token){
            // Pops last 2 operands from stack and checks their types
            printf("POPS GF@__type_conver_var1\n");
            printf("POPS GF@__type_conver_var2\n");
            printf("TYPE GF@__type_conver_type1 GF@__type_conver_var1\n");
            printf("TYPE GF@__type_conver_type2 GF@__type_conver_var2\n");
    
            // If one of the operands is of type nill -> exits
            printf("JUMPIFEQ null_error_exit%d GF@__type_conver_type1 string@nil\n", bi_operations_counter);
            printf("JUMPIFEQ null_error_exit%d GF@__type_conver_type2 string@nil\n", bi_operations_counter);

            // Compares the types
            printf("EQ GF@__type_conver_res GF@__type_conver_type1 GF@__type_conver_type2\n");
            // If same types, no conversion needed
            printf("JUMPIFEQ convert_push_back%d GF@__type_conver_res bool@true\n", bi_operations_counter);
            // If this is true, 1. operand is float, 2. is int
            printf("JUMPIFEQ convert_second%d GF@__type_conver_type1 string@float\n", bi_operations_counter);
            
            // Converts 1. operand
            printf("PUSHS GF@__type_conver_var2\n");
            printf("PUSHS GF@__type_conver_var1\n");
            printf("INT2FLOATS\n");

            printf("JUMP convert_end%d\n", bi_operations_counter);

            // Converts 2. operand
            printf("LABEL convert_second%d\n", bi_operations_counter);
            printf("PUSHS GF@__type_conver_var2\n");
            printf("INT2FLOATS\n");
            printf("PUSHS GF@__type_conver_var1\n");

            printf("JUMP convert_end%d\n", bi_operations_counter);

            // If one of the operands was null -> exits with error
            printf("LABEL null_error_exit%d\n", bi_operations_counter);
            printf("EXIT int@7\n");

            // If same types, just push the operands back onto the stack
            printf("LABEL convert_push_back%d\n", bi_operations_counter);
            printf("PUSHS GF@__type_conver_var2\n");
            printf("PUSHS GF@__type_conver_var1\n");

            printf("LABEL convert_end%d\n", bi_operations_counter);

            bi_operations_counter++;
        }
        // Generates code to check if operands are same types, if not does the necessary conversions
        // Works similiar as other operators but have to check for null differently
            // because (nill == nill) == true
        else if (current_token_type == double_equal_token || current_token_type == not_equal_token){
            // Pops last 2 operands from stack and checks their types
            printf("POPS GF@__type_conver_var1\n");
            printf("POPS GF@__type_conver_var2\n");
            printf("TYPE GF@__type_conver_type1 GF@__type_conver_var1\n");
            printf("TYPE GF@__type_conver_type2 GF@__type_conver_var2\n");
    
            // If one of the operands is null, no conversion needed and we can just compare them
            printf("JUMPIFEQ convert_push_back%d GF@__type_conver_type1 string@nil\n", bi_operations_counter);
            printf("JUMPIFEQ convert_push_back%d GF@__type_conver_type2 string@nil\n", bi_operations_counter);

            // Compares the types
            printf("EQ GF@__type_conver_res GF@__type_conver_type1 GF@__type_conver_type2\n");
            // If same types, no conversion needed
            printf("JUMPIFEQ convert_push_back%d GF@__type_conver_res bool@true\n", bi_operations_counter);
            // If this is true, 1. operand is float, 2. is int
            printf("JUMPIFEQ convert_second%d GF@__type_conver_type1 string@float\n", bi_operations_counter);
            
            // Converts 1. operand
            printf("PUSHS GF@__type_conver_var2\n");
            printf("PUSHS GF@__type_conver_var1\n");
            printf("INT2FLOATS\n");

            printf("JUMP convert_end%d\n", bi_operations_counter);

            // Converts 2. operand
            printf("LABEL convert_second%d\n", bi_operations_counter);
            printf("PUSHS GF@__type_conver_var2\n");
            printf("INT2FLOATS\n");
            printf("PUSHS GF@__type_conver_var1\n");

            printf("JUMP convert_end%d\n", bi_operations_counter);

            // If same types of operands, just pushes them back onto the stack
            printf("LABEL convert_push_back%d\n", bi_operations_counter);
            printf("PUSHS GF@__type_conver_var2\n");
            printf("PUSHS GF@__type_conver_var1\n");

            printf("LABEL convert_end%d\n", bi_operations_counter);

            bi_operations_counter++;
        }

        // Generates code to perform the corresponding operation
        if(strcmp(current_token_data, "<") == 0){
            printf("LTS\n");
        }
        else if(strcmp(current_token_data, ">") == 0){
            printf("GTS\n");
        }
        else if(strcmp(current_token_data, "<=") == 0){
            printf("GTS\n");
            printf("NOTS\n");
        }
        else if(strcmp(current_token_data, ">=") == 0){
            printf("LTS\n");
            printf("NOTS\n");
        }
        else if(strcmp(current_token_data, "!=") == 0){
            printf("EQS\n");
            printf("NOTS\n");
        }
        else if(strcmp(current_token_data, "==") == 0){
            printf("EQS\n");
        }
        else if(strcmp(current_token_data, "+") == 0){
            printf("ADDS\n");
        }
        else if(strcmp(current_token_data, "-") == 0){
            printf("SUBS\n");
        }
        else if(strcmp(current_token_data, "*") == 0){
            printf("MULS\n");
        }
        else if(strcmp(current_token_data, "/") == 0){
            // Checks the type of operand on top of stack
            // We know both operands have to be already same type
            printf("POPS GF@__typecheck_var\n");
            printf("TYPE GF@__typecheck_type GF@__typecheck_var\n");

            // Checks if the last operand on the stack is int == if we can compare it to 0
            printf("JUMPIFNEQ division_continuation%d GF@__typecheck_type string@int\n", div_counter);

            // Checks for division by 0
            printf("JUMPIFNEQ division_continuation%d GF@__typecheck_var int@0\n", div_counter);
            printf("EXIT int@57\n");

            // Continues here if not dividing by 0
            printf("LABEL division_continuation%d\n", div_counter);
            printf("PUSHS GF@__typecheck_var\n");
            // Generates code to check if it's integer or float division
            printf("JUMPIFEQ __div_int%d GF@__typecheck_type string@int\n", div_counter);
            printf("DIVS\n");
            printf("JUMP __div_end%d\n", div_counter);
            printf("LABEL __div_int%d\n", div_counter);
            printf("IDIVS\n");
            printf("LABEL __div_end%d\n", div_counter);
            div_counter++;
        }
        // variables - pushes them onto the stack
        else if(current_token_type == identifier_token){
            printf("PUSHS LF@%s\n", token_node->token->data);
        }
        // literals - pushes them onto the stack
        else{
            if(current_token_type == int_token){
                printf("PUSHS int@%s\n", current_token_data);
            }
            else if(current_token_type == float_token){
                // Converts string to actual double value
                char *tmp;
                double value = strtod(current_token_data, &tmp);
                printf("PUSHS float@%a\n", value);
            }
            else if(current_token_type == null_token){
                printf("PUSHS nil@nil\n");
            }
            // Anything else shouldn't be possible if semantic analyser is working correctly
            else{
                exit(7);
            }
        }
        token_node = next_node(ast); // next token
        current_token_data = token_node->token->data;
        current_token_type = token_node->token->type;
    }
}

// Generates IF STATEMENT
void generate_if_statement(ASTNode *token_node, AST *ast){
    token_node = next_node(ast);    // Skip 'if'
    token_node = next_node(ast);    // skip '(' and go to expression "xxx..."

    static int if_label_counter = 0;    // for generating unique labels
    int current_if_label = if_label_counter++;  // Saves the current value, because there might be nested IFs

    // Checks what type of condition it is and generates conditional jumps
    // if (cond) |y| {}
    if (strcmp(token_node->next->next->token->data, "|") == 0){
        
        printf("MOVE GF@__extcheck_var LF@%s\n", token_node->token->data);
        printf("TYPE GF@__extcheck_type GF@__extcheck_var\n");

        printf("JUMPIFEQ if_else%d GF@__extcheck_type string@nil\n", current_if_label);

        token_node = next_node(ast);    // skip 'cond'
        token_node = next_node(ast);    // skip ')'
        token_node = next_node(ast);    // skip '|'

        // Generates checks for variable redeclaration
        printf("GT GF@__decl_bool GF@__decl_cnt int@%d\n", global_decl_cnt);
        printf("JUMPIFEQ ex_declskip%d GF@__decl_bool bool@true \n", current_if_label);

            printf("DEFVAR LF@%s\n", token_node->token->data);

            global_decl_cnt++;
            printf("MOVE GF@__decl_cnt int@%d\n", global_decl_cnt);
            
        printf("LABEL ex_declskip%d\n", current_if_label);
            
        printf("MOVE LF@%s GF@__extcheck_var\n", token_node->token->data);

        token_node = next_node(ast);    // skip 'y'
        token_node = next_node(ast);    // skip '|'
    }
    // if (expr) {}
    else{
        generate_expression(token_node, ast);

        token_node = ast->active;   // Have to update token_node pointer after generate_expression
        token_node = next_node(ast);    // skip ')'

        printf("POPS GF@__condition_bool\n"); // pop the condition result into global variable
    
        printf("JUMPIFEQ if_then%d GF@__condition_bool bool@true\n", current_if_label);
        printf("JUMP if_else%d\n", current_if_label);
    }

    // Generate THEN branch
    printf("LABEL if_then%d\n", current_if_label);

    token_node = next_node(ast); // Skip '{'

    generate_code_for_line(token_node, ast);
    token_node = ast->active;

    // Have to check if jumped out because of return or end of block
    if (strcmp(token_node->token->data, "return") == 0){
        generate_function_return(token_node, ast);
        token_node = ast->active;

        // If there was some dead code after the return, that we dont have to print...
        while (strcmp(token_node->token->data, "}") != 0){
            token_node = next_node(ast);
        }
    }

    printf("JUMP if_end%d\n", current_if_label);

    token_node = ast->active;
    token_node = next_node(ast);    // skip '}'
    token_node = next_node(ast);    // skip 'else'
    token_node = next_node(ast);    // '{'

    // Generate ELSE branch
    printf("LABEL if_else%d\n", current_if_label);

    generate_code_for_line(token_node, ast);
    token_node = ast->active;

    // Have to check if jumped out because of return or end of block
    if (strcmp(token_node->token->data, "return") == 0){
        generate_function_return(token_node, ast);
        token_node = ast->active;

        // If there was some dead code after the return, that we dont have to print...
        while (strcmp(token_node->token->data, "}") != 0){
            token_node = next_node(ast);
        }
    }

    token_node = next_node(ast);    // skip '}'

    // Skip here after completing then branch
    printf("LABEL if_end%d\n", current_if_label);
}

// Generates WHILE LOOP
void generate_while_loop(ASTNode *token_node, AST *ast){
    token_node = next_node(ast);    // Skip 'while'
    token_node = next_node(ast);    // Skip '(' and move to condition

    static int while_label_counter = 0; // static cnt to generate unique labels
    int current_while_label = while_label_counter++;

    // Checks what type of while loop it is and generates conditional jumps
    // while (cond) |y| {}
    if (strcmp(token_node->next->next->token->data, "|") == 0){
        // Initial check if the value in condition != null
        printf("MOVE GF@__extcheck_var LF@%s\n", token_node->token->data);
        printf("TYPE GF@__extcheck_type GF@__extcheck_var\n");
        printf("JUMPIFEQ while_end%d GF@__extcheck_type string@nil\n", current_while_label);

        // If so, define new variable and move value of the condition into it
        // Generates checks for variable redeclaration
        printf("GT GF@__decl_bool GF@__decl_cnt int@%d\n", global_decl_cnt);
        printf("JUMPIFEQ ex_declskip%d GF@__decl_bool bool@true \n", current_while_label);

            printf("DEFVAR LF@%s\n", token_node->next->next->next->token->data);
            global_decl_cnt++;
            printf("MOVE GF@__decl_cnt int@%d\n", global_decl_cnt);

        printf("LABEL ex_declskip%d\n", current_while_label);
        
        printf("MOVE LF@%s GF@__extcheck_var\n", token_node->next->next->next->token->data);

        // While always returns here when reaching end of its block 
        // to recheck the condition and update value of the special variable
        printf("LABEL while_start%d\n", current_while_label);

        printf("MOVE GF@__extcheck_var LF@%s\n", token_node->token->data);
        printf("TYPE GF@__extcheck_type GF@__extcheck_var\n");
        printf("JUMPIFEQ while_end%d GF@__extcheck_type string@nil\n", current_while_label);
        
        // update value of special var
        printf("MOVE LF@%s GF@__extcheck_var\n", token_node->next->next->next->token->data);

        token_node = next_node(ast);    // skip 'cond'
        token_node = next_node(ast);    // skip ')'
        token_node = next_node(ast);    // skip '|'
        token_node = next_node(ast);    // skip 'y'
        token_node = next_node(ast);    // skip '|'
    }
    // while (cond) {}
    else{
        printf("LABEL while_start%d\n", current_while_label);
        generate_expression(token_node, ast);

        token_node = ast->active;   // Have to update token_node pointer after generate_expression
        token_node = next_node(ast); // skip ')'
        
        // Pop the condition result to global variable
        printf("POPS GF@__condition_bool\n");

        // If condition is false jump out of loop body
        printf("JUMPIFEQ while_end%d GF@__condition_bool bool@false\n", current_while_label);
    }

    token_node = next_node(ast);    // skip '{' and start generating loop body
    generate_code_for_line(token_node, ast);

    token_node = ast->active;
    token_node = next_node(ast);    // skip '}'

    printf("JUMP while_start%d\n", current_while_label);
    printf("LABEL while_end%d\n", current_while_label);
}

// Generates code to declare new variable and assign it a value
/* Examples:
    var x = 5 + 4;
    const y = foo();
    var z = x + 5;
    var a : []u8 = "radfsda"; 
*/
void generate_variable_declaration(ASTNode *token_node, AST *ast){
    token_node = next_node(ast);    // <- 'var_name', skip 'var'/'const'

    static int decl_label_cnt = 0; // static cnt to generate unique labels

    char *var_name = token_node->token->data;

    /* If __decl_cnt > global_decl_cnt, skip declaration
        This is only true, when going back in while loop, 
        because the __decl_cnt already increased when going through the code for the first time
        but the value of global_decl_cnt is printed
    */
    printf("GT GF@__decl_bool GF@__decl_cnt int@%d\n", global_decl_cnt);

    printf("JUMPIFEQ declskip%d GF@__decl_bool bool@true \n", decl_label_cnt);

        // If we are declaring variable for the first time, update the counters so interpret skips this part of code
        //  next time when going back in while loop
        printf("DEFVAR LF@%s\n", var_name);
        global_decl_cnt++;
        printf("MOVE GF@__decl_cnt int@%d\n", global_decl_cnt);

    printf("LABEL declskip%d\n", decl_label_cnt);

    decl_label_cnt++;

    token_node = next_node(ast);    // ":" | "="

    // Skips ": type" if included in declaration
    if (strcmp(token_node->token->data, ":") == 0){
        token_node = next_node(ast);    // skip ':'
        token_node = next_node(ast);    // skip "type"
    }

    token_node = next_node(ast);    // skip '='
    
    // Variable initialization
    // var var_name = ID [variable or function call]
    if (token_node->token->type == identifier_token){   
        if (token_node->next->token->type == bracket_token){    // var var_name = <function_call>(
            generate_function_call_assignment(var_name, token_node, ast);
        }
        else{   // var var_name = <expression>;
            generate_expression_assignment(var_name, token_node, ast);
            token_node = ast->active;
            token_node = next_node(ast);    // skip ';'
        }
    }
    // var var_name = <expression>;
    else{
        if(token_node->token->type == string_token){
            char *escaped_expr_temp = escape_string(token_node->token->data);
            generate_string_assignment(var_name, escaped_expr_temp);
            free(escaped_expr_temp);
            token_node = next_node(ast);    // skip 'string'
            token_node = next_node(ast);    // skip ';'
        }
        else {
            generate_expression_assignment(var_name, token_node, ast);
            token_node = ast->active;
            token_node = next_node(ast);    // skip ';'
        }
    }
}

// Generates code to assign new value to a variable or a function call
// x = a + b * c;
// x = someFunction(a, b);
// someFunction(a, b);
void generate_assignment_or_expression(ASTNode *token_node, AST *ast){
    char *identifier = token_node->token->data; // variable or function call name

    token_node = next_node(ast); // skip 'ID'

    // Variable assignment
    if(strcmp(token_node->token->data, "=") == 0){
        token_node = next_node(ast); // skip '=' to R value

        if(token_node->token->type == identifier_token){    // function call or expression
            // check whats after identifier
            if(strcmp(token_node->next->token->data, "(") == 0){ // Its a function call
                generate_function_call_assignment(identifier, token_node, ast);
            }
            else{ // Its expression
                generate_expression_assignment(identifier, token_node, ast);
                token_node = ast->active;
                token_node = next_node(ast);    // skip ';'
            }
        }
        else if(token_node->token->type == string_token){   // R value is a string
            char *escaped_expr_temp = escape_string(token_node->token->data);
            generate_string_assignment(identifier, escaped_expr_temp);
            free(escaped_expr_temp);
            token_node = next_node(ast);    // skip 'string'
            token_node = next_node(ast);    // skip ';'
        }
        else{   // R value is an expression
            generate_expression_assignment(identifier, token_node, ast);
            token_node = ast->active;
            token_node = next_node(ast);    // skip ';'
        }
    }
    // Its a function call as a statement
    else if(strcmp(token_node->token->data, "(") == 0){
        generate_function_call(identifier, token_node, ast);
        // Revert __decl_cnt to value before function call
        printf("POPS GF@__decl_cnt\n");
    }
}

// Generates code to assign value from expression to a variable
void generate_expression_assignment(char *identifier, ASTNode *token_node, AST *ast){
    // Generate expression code
    generate_expression(token_node, ast);

    // Pop the result into variable
    if (strcmp(identifier, "_") == 0){
       printf("POPS GF@_\n"); 
    }
    else {
        printf("POPS LF@%s\n", identifier);
    }
}

// Generates code to assign value from function call to a variable
void generate_function_call_assignment(char *identifier, ASTNode *function_call_node, AST *ast){
    // Generate function call code
    char* function_name = function_call_node->token->data;
    function_call_node = next_node(ast);
    generate_function_call(function_name, function_call_node->next, ast);

    // Pop the value function returned into the variable
    if (strcmp(identifier, "_") == 0){
        printf("POPS GF@_\n"); 
    }
    else {
        printf("POPS LF@%s\n", identifier);
    }
    // Revert __decl_cnt to value before function call
    printf("POPS GF@__decl_cnt\n");
}

// Generates code to assign string to the 'identifier' variable
void generate_string_assignment(char *identifier, char *string){
    printf("PUSHS string@%s\n", string);
    printf("POPS LF@%s\n", identifier);
}

// Generates function call with the function call arguments
void generate_function_call(char *function_name, ASTNode *token_node, AST *ast){
    token_node = next_node(ast); // Skip '('

    printf("CREATEFRAME\n");    // creates new frame for the function arguments

    int arg_count = 0;
    // Generates code to save the arguments
    while (strcmp(token_node->token->data, ")") != 0){

        printf("DEFVAR TF@__arg%d\n", arg_count);

        if (token_node->token->type == identifier_token){
            // If argument is a variable
            printf("MOVE TF@__arg%d LF@%s\n", arg_count, token_node->token->data);
        }
        else if (token_node->token->type == int_token){
            // If argument is an int literal
            printf("MOVE TF@__arg%d int@%s\n", arg_count, token_node->token->data);
        }
        else if (token_node->token->type == float_token){
            // If argument is a float literal
            printf("MOVE TF@__arg%d float@%s\n", arg_count, token_node->token->data);
        }
        else if (token_node->token->type == string_token){
            // If argument is a string literal
            char *escaped_str_temp = escape_string(token_node->token->data);
            printf("MOVE TF@__arg%d string@%s\n", arg_count, escaped_str_temp);
            free(escaped_str_temp);
        }

        // Move to the next token
        token_node = next_node(ast);

        // Skip ',' if present
        if(strcmp(token_node->token->data, ",") == 0){
            token_node = next_node(ast);
        }

        arg_count++;
    }
    // token = ')'

    // Save __decl_cnt value on the stack and reset it to 0 for new function
    printf("PUSHS GF@__decl_cnt\n");
    printf("MOVE GF@__decl_cnt int@0\n");
    
    printf("PUSHFRAME\n");
    printf("CALL %s\n", function_name);

    token_node = next_node(ast); // skip ')'
    token_node = next_node(ast); // skip ';'
}

// Converts escape sequences to \xyz format
char *escape_string(const char *input) {

    size_t input_len = strlen(input);
    // Allocates enough space: worst case every character is escaped as \xyz
    size_t max_len = (input_len * 4) + 1;
    char *escaped = malloc(sizeof(char)*max_len);
    if (escaped == NULL) {
        fprintf(stderr, "Memory allocation failed in escape_string\n");
        exit(99);
    }

    size_t j = 0; // Index for escaped string

    for (size_t i = 0; i < input_len; i++) {
        unsigned char c = input[i];
        // Check if character needs to be escaped
        if (c <= 32 || c == 35 || c == 92) {
            escaped[j++] = '\\';
            escaped[j++] = '0';
            escaped[j++] = '0' + (c / 10) % 10;
            escaped[j++] = '0' + c % 10;
        } else {
            escaped[j++] = c;
        }
    }

    escaped[j] = '\0'; // Null-terminate the string
    return escaped;
}

// Generates definition of the function
// pub fn ID (parameters) <return TYPE> {
void generate_function_definition(ASTNode *token_node, AST *ast) {

    token_node = next_node(ast); // skip 'pub'
    token_node = next_node(ast); // <- function name, skip 'fn'

    // LABEL function_name
    printf("LABEL %s\n", token_node->token->data);

    // When printing new function definition, reset both counters to 0
    printf("MOVE GF@__decl_cnt int@0\n");
    global_decl_cnt = 0;

    token_node = next_node(ast); // skip 'function_name'
    token_node = next_node(ast); // <- parameter or ')', skip '('

    // Going through all the parameters and initializes them with the values from function call
    int param_idx = 0;
    while(strcmp(token_node->token->data, ")") != 0){
        // Parameter: <id> : <type>
        char *param_name = token_node->token->data; // Store identifier

        token_node = next_node(ast); // <- ':', skip "ID"
        token_node = next_node(ast); // <- type, skip ':'
        
        printf("DEFVAR LF@%s\n", param_name);

        printf("MOVE LF@%s LF@__arg%d\n", param_name, param_idx);
        param_idx++;

        // Move to ',' or ')'
        token_node = next_node(ast); // <- ',' or ')'

        // Skip ','
        if(strcmp(token_node->token->data, ",") == 0){
            token_node = next_node(ast); // go to next parameter
        }
    }

    // pub fn ID (parametry) <return TYPE> {
    token_node = next_node(ast); // skip ')'
    token_node = next_node(ast); // <- '{', skip return type
    token_node = next_node(ast); // <- skip '{'

    // generate function definition block
    generate_code_for_line(token_node, ast);
    token_node = ast->active;

    // "return" or '}'
    generate_function_return(token_node, ast);
}

// Generates return for function
void generate_function_return(ASTNode *token_node, AST *ast) {
    
    if (strcmp(token_node->token->data, "}") != 0){ // true if function has return keyword
        token_node = next_node(ast); // Skip 'return'

        if (strcmp(token_node->token->data, ";") != 0) {    // true if there is expression after "return keyword"
            // Generate code for the return expression
            generate_expression(token_node, ast);
            token_node = ast->active;
        }
        token_node = next_node(ast);    // skip ';'
    }
    // token = '}'

    // The result is on top of the stack
    // No need to do anything else, just call RETURN
    printf("POPFRAME\n");
    printf("RETURN\n");
}

// Generates function definition of all the built-in functions
void generate_builtin_functions(){
/************************  Functions for reading/writing  ************************/
    // pub fn ifj.readstr() ?[]u8
    printf("LABEL ifj$readstr\n");
    
    // Define local variables
    printf("DEFVAR LF@__retval\n");         // The read string
    printf("DEFVAR LF@__type\n");           // Type of the read value
    
    // Read input as string
    printf("READ LF@__retval string\n");

    // Check if input is of type string
    printf("TYPE LF@__type LF@__retval\n");
    printf("JUMPIFEQ ifj_readstr_end LF@__type string@string\n");
    // If not, set return value to nil
    printf("MOVE LF@__retval nil@nil\n");

    printf("LABEL ifj_readstr_end\n");
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.readi32() ?i32
    printf("LABEL ifj$readi32\n");

    printf("DEFVAR LF@__retval\n");
    printf("DEFVAR LF@__type\n");

    printf("READ LF@__retval int\n");
    printf("TYPE LF@__type LF@__retval\n");

    printf("JUMPIFEQ ifj_readi32_end LF@__type string@int\n");
    printf("MOVE LF@__retval nil@nil\n");

    printf("LABEL ifj_readi32_end\n");
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

// **********************************************************
    // pub fn ifj.readf64() ?f64
    printf("LABEL ifj$readf64\n");

    printf("DEFVAR LF@__retval\n");
    printf("DEFVAR LF@__type\n");

    printf("READ LF@__retval float\n");
    printf("TYPE LF@__type LF@__retval\n");

    printf("JUMPIFEQ ifj_readf64_end LF@__type string@float\n");
    printf("MOVE LF@__retval nil@nil\n");

    printf("LABEL ifj_readf64_end\n");
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

// **********************************************************
    // pub fn ifj.write(term) void
    printf("LABEL ifj$write\n");
    printf("DEFVAR LF@__term\n");
    printf("DEFVAR LF@__type\n");

    printf("MOVE LF@__term LF@__arg0\n");
    printf("TYPE LF@__type LF@__term\n");

    printf("JUMPIFEQ ifj_write_nil LF@__type string@nil\n"); // if the value is nill

    printf("WRITE LF@__term\n");
    printf("JUMP ifj_write_end\n");
    
    printf("LABEL ifj_write_nil\n");
    printf("WRITE string@null\n");

    printf("LABEL ifj_write_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");


/***************************  Type conversion functions  ****************************/
    /// pub fn ifj.i2f(term ∶ i32) f64
    printf("LABEL ifj$i2f\n");
    
    printf("DEFVAR LF@__retval\n");
    printf("INT2FLOAT LF@__retval LF@__arg0\n");
    
    // Push the result onto the stack
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.f2i(term ∶ f64) i32
    printf("LABEL ifj$f2i\n");
    
    printf("DEFVAR LF@__retval\n");
    printf("FLOAT2INT LF@__retval LF@__arg0\n");
    
    // Push the result onto the stack
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");


/***********************  Functions for strings  *************************/
    // pub fn ifj.string(term) []u8
    printf("LABEL ifj$string\n");

    // Push the term onto the stack
    printf("PUSHS LF@__arg0\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.length(𝑠 : []u8) i32
    printf("LABEL ifj$length\n");
    
    printf("DEFVAR LF@__s\n");
    printf("MOVE LF@__s LF@__arg0\n");
    
    printf("DEFVAR LF@__retval\n");
    printf("STRLEN LF@__retval LF@__s\n");
    
    // Push the result onto the stack
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.concat(𝑠1 : []u8, 𝑠2 : []u8) []u8
    printf("LABEL ifj$concat\n");
    
    printf("DEFVAR LF@__s1\n");
    printf("DEFVAR LF@__s2\n");
    printf("DEFVAR LF@__retval\n");

    printf("MOVE LF@__s1 LF@__arg0\n");
    printf("MOVE LF@__s2 LF@__arg1\n");    
    printf("CONCAT LF@__retval LF@__s1 LF@__s2\n");
    
    // Push the result onto the stack
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.substring(𝑠 : []u8, 𝑖 : i32, 𝑗 : i32) ?[]u8
    printf("LABEL ifj$substring\n");

    // Define local variables
    printf("DEFVAR LF@__s\n");
    printf("DEFVAR LF@__i\n");
    printf("DEFVAR LF@__j\n");
    
    printf("MOVE LF@__s LF@__arg0\n");
    printf("MOVE LF@__i LF@__arg1\n");
    printf("MOVE LF@__j LF@__arg2\n");

    printf("DEFVAR LF@__retval\n");
    printf("DEFVAR LF@__len\n");
    printf("DEFVAR LF@__cond\n");
    printf("DEFVAR LF@__substring\n");
    printf("DEFVAR LF@__tmp_char\n");

    // Check for error conditions
    // If i < 0
    printf("LT LF@__cond LF@__i int@0\n");
    printf("JUMPIFEQ ifj_substring_error LF@__cond bool@true\n");

    // If j < 0
    printf("LT LF@__cond LF@__j int@0\n");
    printf("JUMPIFEQ ifj_substring_error LF@__cond bool@true\n");

    // If i > j
    printf("GT LF@__cond LF@__i LF@__j\n");
    printf("JUMPIFEQ ifj_substring_error LF@__cond bool@true\n");

    // Get the length of the string s
    printf("STRLEN LF@__len LF@__s\n");

    // If i >= length(s)
    printf("LT LF@__cond LF@__i LF@__len\n");
    printf("JUMPIFNEQ ifj_substring_error LF@__cond bool@true\n");

    // If j > length(s)
    printf("GT LF@__cond LF@__j LF@__len\n");
    printf("JUMPIFEQ ifj_substring_error LF@__cond bool@true\n");

    // Initialize empty string
    printf("MOVE LF@__substring string@\n");

    // while loop
    printf("LABEL ifj_substring_while\n");
    printf("JUMPIFEQ ifj_substring_while_end LF@__i LF@__j\n");

    printf("GETCHAR LF@__tmp_char LF@__s LF@__i\n");
    printf("CONCAT LF@__substring LF@__substring LF@__tmp_char\n");

    printf("ADD LF@__i LF@__i int@1\n"); // i++
    printf("JUMP ifj_substring_while\n");

    // Error label: Return nil
    printf("LABEL ifj_substring_error\n");
    printf("MOVE LF@__retval nil@nil\n");
    printf("JUMP ifj_substring_end\n");

    printf("LABEL ifj_substring_while_end\n");
    printf("MOVE LF@__retval LF@__substring\n");
    printf("LABEL ifj_substring_end\n");

    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.strcmp(𝑠1 : []u8, 𝑠2 : []u8) i32
    printf("LABEL ifj$strcmp\n");

    // Define local variables
    printf("DEFVAR LF@__s1\n");
    printf("DEFVAR LF@__s2\n");
    printf("MOVE LF@__s1 LF@__arg0\n");
    printf("MOVE LF@__s2 LF@__arg1\n");

    printf("DEFVAR LF@__len1\n");
    printf("DEFVAR LF@__len2\n");
    printf("DEFVAR LF@__min_len\n");
    printf("DEFVAR LF@__i\n");
    printf("DEFVAR LF@__char1\n");
    printf("DEFVAR LF@__char2\n");
    printf("DEFVAR LF@__cmp_res\n");
    printf("DEFVAR LF@__retval\n");

    
    printf("STRLEN LF@__len1 LF@__s1\n");
    printf("STRLEN LF@__len2 LF@__s2\n");

    // Determine the minimum length
    printf("LT LF@__cmp_res LF@__len1 LF@__len2\n");
    printf("JUMPIFEQ ifj_strcmp_set_min_len1 LF@__cmp_res bool@true\n");
    printf("MOVE LF@__min_len LF@__len2\n");
    printf("JUMP ifj_strcmp_start\n");
    printf("LABEL ifj_strcmp_set_min_len1\n");
    printf("MOVE LF@__min_len LF@__len1\n");

    printf("LABEL ifj_strcmp_start\n");
    printf("MOVE LF@__i int@0\n"); // i = 0

    printf("LABEL ifj_strcmp_loop\n");
    // Loop condition: __i < __min_len
    printf("LT LF@__cmp_res LF@__i LF@__min_len\n");
    printf("JUMPIFEQ ifj_strcmp_compare_chars LF@__cmp_res bool@true\n");
    printf("JUMP ifj_strcmp_length_compare\n");

    printf("LABEL ifj_strcmp_compare_chars\n");
    // Get characters at position __i
    printf("GETCHAR LF@__char1 LF@__s1 LF@__i\n");
    printf("GETCHAR LF@__char2 LF@__s2 LF@__i\n");
    // Compare characters
    printf("GT LF@__cmp_res LF@__char1 LF@__char2\n");
    printf("JUMPIFEQ ifj_strcmp_s1_greater LF@__cmp_res bool@true\n");
    printf("LT LF@__cmp_res LF@__char1 LF@__char2\n");
    printf("JUMPIFEQ ifj_strcmp_s1_less LF@__cmp_res bool@true\n");
    // Characters are equal, continue loop
    printf("ADD LF@__i LF@__i int@1\n");
    printf("JUMP ifj_strcmp_loop\n");

    // If s1 > s2
    printf("LABEL ifj_strcmp_s1_greater\n");
    printf("MOVE LF@__retval int@1\n");
    printf("JUMP ifj_strcmp_end\n");

    // If s1 < s2
    printf("LABEL ifj_strcmp_s1_less\n");
    printf("MOVE LF@__retval int@-1\n");
    printf("JUMP ifj_strcmp_end\n");

    // After loop, compare lengths
    printf("LABEL ifj_strcmp_length_compare\n");
    printf("EQ LF@__cmp_res LF@__len1 LF@__len2\n");
    printf("JUMPIFEQ ifj_strcmp_equal LF@__cmp_res bool@true\n");
    printf("GT LF@__cmp_res LF@__len1 LF@__len2\n");
    printf("JUMPIFEQ ifj_strcmp_s1_greater LF@__cmp_res bool@true\n");
    // Else, s1 is less than s2
    printf("JUMP ifj_strcmp_s1_less\n");

    printf("LABEL ifj_strcmp_equal\n");
    printf("MOVE LF@__retval int@0\n");
    printf("JUMP ifj_strcmp_end\n");

    printf("LABEL ifj_strcmp_end\n");
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.ord(𝑠 : []u8, 𝑖 : i32) i32
    printf("LABEL ifj$ord\n");

    // Define local variables
    printf("DEFVAR LF@__s\n");
    printf("DEFVAR LF@__i\n");
    printf("MOVE LF@__s LF@__arg0\n");
    printf("MOVE LF@__i LF@__arg1\n");

    printf("DEFVAR LF@__len\n");
    printf("DEFVAR LF@__char\n");
    printf("DEFVAR LF@__retval\n");
    printf("DEFVAR LF@__cond\n");

    printf("STRLEN LF@__len LF@__s\n");

    // Default return value is 0
    printf("MOVE LF@__retval int@0\n");

    // Check if __s is empty
    printf("EQ LF@__cond LF@__len int@0\n");
    printf("JUMPIFEQ ifj_ord_end LF@__cond bool@true\n");

    // Check if i < 0 or i >= len(s)
    printf("LT LF@__cond LF@__i int@0\n");
    printf("JUMPIFEQ ifj_ord_end LF@__cond bool@true\n");
    printf("GT LF@__cond LF@__i LF@__len\n");
    printf("JUMPIFEQ ifj_ord_end LF@__cond bool@true\n");
    printf("EQ LF@__cond LF@__i LF@__len\n");
    printf("JUMPIFEQ ifj_ord_end LF@__cond bool@true\n");

    // Get character at position i and convert character to integer (ASCII value)
    printf("STRI2INT LF@__retval LF@__s LF@__i\n");

    printf("LABEL ifj_ord_end\n");
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n\n");

// **********************************************************
    // pub fn ifj.chr(𝑖 : i32) []u8
    printf("LABEL ifj$chr\n");

    // Define local variables
    printf("DEFVAR LF@__i\n");
    printf("MOVE LF@__i LF@__arg0\n");

    printf("DEFVAR LF@__retval\n");

    // Convert integer to character
    printf("INT2CHAR LF@__retval LF@__i\n");

    // Push the result onto the stack
    printf("PUSHS LF@__retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
