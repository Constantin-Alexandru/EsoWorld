#include "../ANSI-Library/ansi_lib.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <stdlib.h>

bool debug_flag = false;

#define PROGRAM_MEM_SIZE 30000

enum Token
{
    PTR_RIGHT = 0,
    PTR_LEFT = 1,
    INCR = 2,
    DECR = 3,
    OUTP = 4,
    INP = 5,
    LOOP_BEG = 6,
    LOOP_END = 7,
};

void error(std::string message);
void progress(int percentage, std::string progress_msg, Colours colour);
void help();
int wrap_val(int val, int lwr_bound, int upp_bound);
bool read_file(const char *filename, std::vector<char> &content);
bool convert_to_tokens(std::vector<char> content, std::vector<Token> &tokens);
bool validate_program(std::vector<Token> tokens);
void interpret_program(std::vector<Token> tokens);

int main(int argc, char const *argv[])
{
#ifdef _WIN32
    setupConsole();
#endif

    if (argc < 2 || argc > 3)
    {
        error("Invalid number of arguments: expected 1 or 2, but received " + argc);
        return -1;
    }

    int file_index = 1;

    if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--h") == 0)
        {
            help();
            return 0;
        }
    }
    else if (argc == 3)
    {
        if (strcmp(argv[1], "--debug") == 0 || strcmp(argv[1], "--d") == 0)
        {
            file_index = 2;
            debug_flag = true;
        }
        else if (strcmp(argv[2], "--debug") == 0 || strcmp(argv[2], "--d") == 0)
        {
            debug_flag = true;
        }
    }

    std::string file_name(argv[file_index]);

    if (file_name.substr(file_name.size() - 3, 3) != ".bf")
    {
        error("File " + file_name + " is not a brainfuck file: expected file ending in .bf");
        return -1;
    }

    std::vector<char> content;

    progress(0, std::string("Reading contents from file").append(argv[file_index]), TXT_YELLOW);

    if (!read_file(argv[file_index], content))
    {
        return -1;
    }

    progress(25, "Converting input to tokens", TXT_YELLOW);

    std::vector<Token> tokens;

    if (!convert_to_tokens(content, tokens))
    {
        return -1;
    }

    progress(50, "Validating code", TXT_YELLOW);

    if (!validate_program(tokens))
    {
        return -1;
    }

    progress(75, "Running interpreter on tokens", TXT_YELLOW);

    interpret_program(tokens);

    progress(100, "Program interpreted, exiting...", TXT_GREEN);
    resetConsole();

    return 0;
}

void error(std::string message)
{
    SET_8_VALUE_COLOUR(TXT_RED);
    SET_GRAPHIC_MODE(BOLD_MODE);
    printf("ERROR: ");
    SET_GRAPHIC_MODE(BOLD_MODE_RESET);
    printf("%s\n", message.c_str());

    SET_8_VALUE_COLOUR(TXT_WHITE);
}

void progress(int percentage, std::string progress_msg, Colours colour)
{
    if (!debug_flag)
        return;

    SET_8_VALUE_COLOUR(TXT_WHITE);
    printf(percentage == 0 ? "[  %d%] " : percentage < 100 ? "[ %d%] "
                                                           : "[%d%] ",
           percentage);

    SET_8_VALUE_COLOUR(colour);
    printf("%s\n", progress_msg.c_str());

    SET_8_VALUE_COLOUR(TXT_WHITE);
}

void help()
{
    SET_8_VALUE_COLOUR(TXT_RED);
    printf("========= BRAINFUCK COMPILER =========\n");
    
    MOVE_CURSOR_BEGINNING_LINE_DOWN_BY_LINES(1);

    SET_GRAPHIC_MODE(BOLD_MODE);
    SET_8_VALUE_COLOUR(TXT_WHITE);

    printf("COMMANDS:\n");

    SET_GRAPHIC_MODE(BOLD_MODE_RESET);

    printf("   - brainfuck ");

    SET_GRAPHIC_MODE(DIM_MODE);
    printf("--help ");
    SET_GRAPHIC_MODE(DIM_MODE_RESET);
    printf(": Shows this menu\n");

    MOVE_CURSOR_BEGINNING_LINE_DOWN_BY_LINES(1);

    printf("   - brainfuck ");
    
    SET_GRAPHIC_MODE(DIM_MODE);
    printf("<file-path> --debug");
    SET_GRAPHIC_MODE(DIM_MODE_RESET);

    printf(": Interprets a brainfuck file\n");

    SET_GRAPHIC_MODE(DIM_MODE);
    printf("      - <file-path> : the brainfuck file\n");
    printf("      - --debug : shows debug info\n");

    SET_8_VALUE_COLOUR(TXT_WHITE);
    SET_GRAPHIC_MODE(DIM_MODE_RESET);

    SET_GRAPHIC_MODE(BOLD_MODE);

    printf("Note: ");

    SET_GRAPHIC_MODE(BOLD_MODE_RESET);

    printf("The flag and <file-path> are interchangable");

}

int wrap_val(int val, int lwr_bound, int upp_bound)
{
    int range = upp_bound - lwr_bound;
    val = ((val - lwr_bound) % range);

    return val < 0 ? upp_bound + val : lwr_bound + val;
}

bool read_file(const char *filename, std::vector<char> &content)
{
    std::ifstream in(filename);

    if (!in.is_open())
    {
        error(std::string("File ").append(filename).append(" could not be loaded."));

        return false;
    }

    std::string valid_chars = "><+-.,[]";

    char ch;

    while (in.get(ch))
    {
        if (valid_chars.find(ch) != std::string::npos)
        {
            content.push_back(ch);
        }
    }

    in.close();

    return true;
}

bool convert_to_tokens(std::vector<char> content, std::vector<Token> &tokens)
{
    for (char c : content)
    {
        if (c == '>')
            tokens.push_back(Token::PTR_RIGHT);
        else if (c == '<')
            tokens.push_back(Token::PTR_LEFT);
        else if (c == '+')
            tokens.push_back(Token::INCR);
        else if (c == '-')
            tokens.push_back(Token::DECR);
        else if (c == '.')
            tokens.push_back(Token::OUTP);
        else if (c == ',')
            tokens.push_back(Token::INP);
        else if (c == '[')
            tokens.push_back(Token::LOOP_BEG);
        else if (c == ']')
            tokens.push_back(Token::LOOP_END);
        else
        {
            error(std::string("Character ").append(&c).append(" is not inside the list of characters"));
            return false;
        }
    }

    return true;
}

bool validate_program(std::vector<Token> tokens)
{
    int loops = 0;

    for (int i = 0; i < tokens.size(); i++)
    {
        switch (tokens[i])
        {
        case LOOP_BEG:
            loops++;
            break;
        case LOOP_END:
            loops--;
            break;
        }

        if (loops < 0)
        {
            char buffer[30];

            itoa(i, buffer, 10);

            error(std::string("The loop ended at position ").append(std::string(buffer)).append(" does not have a beginning"));
            return false;
        }
    }

    if (loops != 0)
    {
        char buffer[33];
        itoa(loops, buffer, 10);
        error(std::string("Expected all loops to be closed, but ").append(buffer).append(" loops are not closed "));

        return false;
    }

    return true;
}

void interpret_program(std::vector<Token> tokens)
{
    std::stack<int> loop_beginnings;

    std::vector<char> memory(PROGRAM_MEM_SIZE, 0);

    int memory_index = 0;

    std::vector<std::string> token_string = { "PTR_RIGHT", "PTR_LEFT", "INCR", "DECR", "OUTP", "INP", "LOOP_BEG", "LOOP_END" };

    for (int i = 0; i < tokens.size(); i++)
    {

        switch (tokens[i])
        {
        case PTR_RIGHT:
            memory_index = wrap_val(memory_index + 1, 0, PROGRAM_MEM_SIZE);
            break;
        case PTR_LEFT:
            memory_index = wrap_val(memory_index - 1, 0, PROGRAM_MEM_SIZE);
            break;
        case INCR:
            memory[memory_index]++;
            break;
        case DECR:
            memory[memory_index]--;
            break;
        case OUTP:
            printf("%c", memory[memory_index]);
            break;
        case INP:
            std::cin.get(memory[memory_index]);
            break;
        case LOOP_BEG:
            if (memory[memory_index] == 0)
            {
                int loops = 1;

                while (loops != 0  && i < tokens.size())
                {
                    i++;
                    
                    if(tokens[i] == Token::LOOP_BEG)
                        loops++;
                    if(tokens[i] == Token::LOOP_END)
                        loops--;
                }
            }
            else
            {
                loop_beginnings.push(i);
            }
            break;
        case LOOP_END:

            int index = loop_beginnings.top() - 1;
            loop_beginnings.pop();


            if (memory[memory_index] != 0)
            {
                i = index;
            }
            break;
        }

    }
}
