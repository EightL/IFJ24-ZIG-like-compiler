/*
* Project: Implementacia prekladaca imperativneho jazyka IFJ2024
*
* @author: Rebeka Tydorova <xtydor01>
*
*/
#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "lexer.h"
#include "token.h"
#include "string_stack.h"
#include "bts_stack.h"
#include "btree.h"
#include "expression.h"
#include "ast.h"

void create_postfix(bst_node_t *node, AST *ast);


//append to binary tree and push to stack
void bts_append(bts_Stack *stack, char* operation) {
    //get nodes from stack
    bst_node_t *right_child = bts_Stack_Top(stack); 
    bts_Stack_Pop(stack); 
    bst_node_t *left_child = bts_Stack_Top(stack);
    bts_Stack_Pop(stack); 

    //token representing operation
    token_t *token = (token_t *)malloc(sizeof(token_t));
    if (token == NULL) {
        fprintf(stderr, "Error: malloc failed for token\n");
        exit(99);
    }
    token->data = strdup(operation);
    if (token->data == NULL) {
        fprintf(stderr, "Error: strdup failed\n");
        free(token);
        exit(1);
    }
    if(strcmp(token->data, "==") == 0){
        token->type = double_equal_token;
    } else if(strcmp(token->data, "!=") == 0){
        token->type = not_equal_token;
    } else {
    token->type = binary_operator_token;
    }

    //creating new node
    bst_node_t *node = bts_create_node(token);
    node->left = left_child;
    node->right = right_child;

    //push new node to stack
    bts_Stack_Push(stack, node); 
}

//rules recognition
void process_rule(const char *rule, bts_Stack *stack) {
    if (strcmp(rule, "E+E") == 0) {
        bts_append(stack,  "+");
    } 
    else if (strcmp(rule, "E-E") == 0) {
        bts_append(stack,  "-");
    } 
    else if (strcmp(rule, "E*E") == 0) {
        bts_append(stack,  "*");
    } 
    else if (strcmp(rule, "E/E") == 0) {
        bts_append(stack,  "/");
    } 
    else if (strcmp(rule, "(E)") == 0) {
    } 
    else if (strcmp(rule, "i") == 0) {
    } 
    else if (strcmp(rule, "E==E") == 0) {
        bts_append(stack,  "==");
    } 
    else if (strcmp(rule, "E!=E") == 0) {
        bts_append(stack,  "!=");
    } 
    else if (strcmp(rule, "E<E") == 0) {
        bts_append(stack,  "<");
    } 
    else if (strcmp(rule, "E>E") == 0) {
        bts_append(stack,  ">");
    } 
    else if (strcmp(rule, "E<=E") == 0) {
        bts_append(stack,  "<=");
    } 
    else if (strcmp(rule, "E>=E") == 0) {
        bts_append(stack,  ">=");
    } else {
        fprintf(stderr, "Syntax error \n");
        exit(2);
    }
}

//check input token
token_t* check_token(token_t* token, int* brackets, token_t* output_token){
    if(*brackets != -1){
        token = get_token();
        if (strcmp(token->data,"(") == 0){
            (*brackets)++;
        }
        else if (strcmp(token->data,")") == 0){
            (*brackets)--;
        }
    }
            
    if(strcmp(token->data, ";") == 0 || *brackets == -1){
        output_token->data = token->data;
        output_token->type = token->type;
        token->data = "$";
    }

    return token;
}

//precedence table
const char *precedence_table[15][15] = {
    {"",     "*",   "/",   "+",   "-",  "==",  "!=",   "<",   ">",   "<=",  ">=",  "(",   ")",   "i", "$"},
    { "*",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { "/",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { "+",   "S",   "S",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { "-",   "S",   "S",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { "==",  "S",   "S",   "S",   "S",   "R",   "R",   "S",   "S",   "S",   "S",   "S",   "R",   "S", "R"},
    { "!=",  "S",   "S",   "S",   "S",   "R",   "R",   "S",   "S",   "S",   "S",   "S",   "R",   "S", "R"},
    { "<",   "S",   "S",   "S",   "S",   "S",   "S",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { ">",   "S",   "S",   "S",   "S",   "S",   "S",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { "<=",  "S",   "S",   "S",   "S",   "S",   "S",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { ">=",  "S",   "S",   "S",   "S",   "S",   "S",   "R",   "R",   "R",   "R",   "S",   "R",   "S", "R"},
    { "(",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "=",   "S", " "},
    { ")",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "E",   "R",   "E", "R"},
    { "i",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "R",   "O",   "R",   "E", "R"},
    { "$",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "S",   "E",   "S", " "}
        
};

//process expression
token_t* expression(token_t *token, AST *ast){

    //initializing 
    int brackets = 0;

    if (strcmp(token->data,"(") == 0){
        brackets++;
    }
    else if (strcmp(token->data,")") == 0){
        brackets--;
    }

    Stack stack;
    Stack_Init(&stack);
    Stack_Push(&stack, "$");

    bts_Stack bts_stack;
    bts_Stack_Init(&bts_stack);

    token_t *output_token;
    output_token = malloc(sizeof(token_t));


    while (true) {
        //getting column number in precedence table
        int row;
        int column;
        bool correct_input = false;
        if(token->type == identifier_token || token->type == int_token || token->type == float_token || token->type == string_token || token->type == null_token) {
            column = 13;
            correct_input = true;
        }
        else{
            for (int i = 0; i < 15; i++) {
                if (strcmp(token->data, precedence_table[0][i]) == 0) {
                    column = i;
                    correct_input = true;
                }
            }
        }

        //input check
        if(correct_input == false && strcmp(token->data, ";") != 0){
            fprintf(stderr, "Syntax error \n");
            exit(2);
        }
        
        //getting row number in precedence table
        char *topToken = stack.string[Stack_find(&stack)];
            for (int i = 0; i < 15; i++) {
                if (strcmp(topToken, precedence_table[0][i]) == 0) {
                row = i;
                }
            }

        //expression processed 
        if (strcmp(topToken, "$") == 0 && strcmp(token->data, "$") == 0){
            break;
        }

        //shift
        if(strcmp(precedence_table[row][column], "S") == 0){
            if(token->type == identifier_token || token->type == int_token || token->type == float_token || token->type == string_token || token->type == null_token) {
            Stack_insert_str(&stack);
                Stack_Push(&stack, "i");
                bst_node_t *node = bts_create_node(token);
                bts_Stack_Push(&bts_stack, node);
            }
            else {
                Stack_insert_str(&stack);
                Stack_Push(&stack, token->data);
            }

            token = check_token(token, &brackets, output_token);
        }

        //reduce
        else if (strcmp(precedence_table[row][column], "R") == 0) {
            char *rule = Stack_rule_str(&stack);
            process_rule(rule, &bts_stack);
            Stack_extract_str(&stack);
            Stack_Push(&stack, "E");
        }

        //equal operation
        else if (strcmp(precedence_table[row][column], "=") == 0) {
            Stack_Push(&stack, token->data);
            char *rule = Stack_rule_str(&stack);
            process_rule(rule, &bts_stack);
            Stack_extract_str(&stack);
            Stack_Push(&stack, "E");

            token = check_token(token, &brackets, output_token);
        }

        else if (strcmp(precedence_table[row][column], "O") == 0) {
            output_token->data = token->data;
            output_token->type = token->type;
            break;
        }

        else if (strcmp(precedence_table[row][column], "E") == 0) {
            fprintf(stderr, "Syntax error \n");
            exit(2);
        }

    }
    
    //add to AST
    bst_node_t *parent = bts_Stack_Top(&bts_stack); 
    bts_Stack_Pop(&bts_stack);
    create_postfix(parent, ast);
    
    //dispose all
    Stack_Dispose(&stack);
    bts_Stack_Dispose(&bts_stack);
    return output_token;
}

//create postfix from btree
void create_postfix(bst_node_t *node, AST *ast){
    if (node != NULL){
        create_postfix(node->left, ast);
        create_postfix(node->right, ast);
        create_node(node->value, ast);
    }
}
