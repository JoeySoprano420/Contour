#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp> // JSON library for enhanced data parsing
#include <sstream>

using json = nlohmann::json;

// Data structures
std::unordered_map<std::string, uint8_t> opcode_lookup;
std::unordered_map<std::string, size_t> reference_table;
std::vector<int64_t> memory(256, 0);
size_t memory_index = 0;
size_t program_counter = 0;
std::vector<std::vector<int64_t>> binary_program;

// Load JSON data from file
json load_json(const std::string &file_name) {
    std::ifstream file(file_name);
    json data;
    if (!file) {
        std::cerr << "Error: Could not open " << file_name << std::endl;
        exit(1);
    }
    file >> data;
    return data;
}

// Merge JSON files into opcode lookup table
void initialize_opcode_lookup() {
    opcode_lookup = {
        {"LET", 0x10}, {"ADD", 0x20}, {"SUBTRACT", 0x21}, {"MULTIPLY", 0x22}, {"DIVIDE", 0x23},
        {"JUMP", 0x30}, {"IF", 0x31}, {"LOOP", 0x32}, {"PRINT", 0x40}
    };
}

// Load binary instructions dynamically from a file
void load_binary_program(const std::string &file_name) {
    std::ifstream file(file_name);
    if (!file) {
        std::cerr << "Error: Could not open binary program file." << std::endl;
        exit(1);
    }

    int64_t opcode;
    while (file >> opcode) {
        std::vector<int64_t> instruction;
        instruction.push_back(opcode);

        // Assume each instruction has fixed parameters for simplicity
        for (int i = 0; i < 3; ++i) {
            int64_t param;
            file >> param;
            instruction.push_back(param);
        }
        binary_program.push_back(instruction);
    }
}

// Error handling: Ensure valid opcode and memory bounds
void check_valid_opcode(uint8_t opcode) {
    if (opcode_lookup.find(std::to_string(opcode)) == opcode_lookup.end()) {
        std::cerr << "Error: Unknown opcode encountered - 0x" << std::hex << (int)opcode << std::endl;
        exit(1);
    }
}

void check_memory_bounds(size_t index) {
    if (index >= memory.size()) {
        std::cerr << "Error: Memory access out of bounds!" << std::endl;
        exit(1);
    }
}

// Allocate variables in memory
void allocate_variable(const std::string &var, int64_t value) {
    if (reference_table.find(var) == reference_table.end()) {
        reference_table[var] = memory_index++;
    }
    check_memory_bounds(reference_table[var]);
    memory[reference_table[var]] = value;
}

// Execute the binary program with control structures
void execute_binary_program() {
    while (program_counter < binary_program.size()) {
        const std::vector<int64_t> &instruction = binary_program[program_counter];
        uint8_t opcode = instruction[0];

        switch (opcode) {
            case 0x10: // let
                allocate_variable(reinterpret_cast<char *>(instruction[1]), instruction[2]);
                break;
            case 0x20: // add
                allocate_variable(reinterpret_cast<char *>(instruction[3]),
                                  memory[instruction[1]] + memory[instruction[2]]);
                break;
            case 0x21: // subtract
                allocate_variable(reinterpret_cast<char *>(instruction[3]),
                                  memory[instruction[1]] - memory[instruction[2]]);
                break;
            case 0x22: // multiply
                allocate_variable(reinterpret_cast<char *>(instruction[3]),
                                  memory[instruction[1]] * memory[instruction[2]]);
                break;
            case 0x30: // jmp
                program_counter = instruction[1] - 1; // Jump to the specified address
                break;
            case 0x31: // if
                if (memory[instruction[1]] != 0) {
                    program_counter = instruction[2] - 1; // Conditional jump
                }
                break;
            case 0x32: // loop
                for (int i = 0; i < memory[instruction[1]]; ++i) {
                    program_counter = instruction[2] - 1; // Loop to address
                }
                break;
            case 0x40: // print
                check_memory_bounds(instruction[1]);
                std::cout << "Value: " << memory[instruction[1]] << std::endl;
                break;
            default:
                std::cerr << "Unknown opcode: 0x" << std::hex << (int)opcode << std::endl;
                exit(1);
        }
        program_counter++;
    }
}

// AST Interpreter with loop and condition nodes
class AOTInterpreter {
private:
    struct Node {
        std::string command;
        Node *left;
        Node *right;
    };

    std::vector<Node *> garbageCollection;

    Node *create_node(const std::string &command) {
        Node *node = new Node();
        node->command = command;
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }

    void execute_tree(Node *root) {
        if (!root) return;

        if (root->command == "IF") {
            std::cout << "Evaluating IF condition\n";
            execute_tree(root->left);  // Evaluate left child as condition
            if (rand() % 2) execute_tree(root->right);  // Simulate condition outcome
        } else if (root->command == "LOOP") {
            std::cout << "Executing LOOP block\n";
            for (int i = 0; i < 3; ++i) execute_tree(root->left);  // Simulate a loop
        } else {
            std::cout << "Executing: " << root->command << std::endl;
        }

        garbageCollection.push_back(root);
    }

    void clean_up() {
        for (Node *node : garbageCollection) {
            delete node;
        }
        garbageCollection.clear();
    }

public:
    void run() {
        Node *root = create_node("IF");
        root->left = create_node("Condition");
        root->right = create_node("LOOP");
        root->right->left = create_node("Loop Body");

        execute_tree(root);
        clean_up();
    }
};

// REPL Implementation
void repl() {
    std::string input;
    std::cout << "REPL Mode - Type 'exit' to quit.\n";
    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, input);
        if (input == "exit") break;

        std::istringstream iss(input);
        std::string command;
        int64_t operand1, operand2;
        iss >> command >> operand1 >> operand2;

        if (opcode_lookup.find(command) != opcode_lookup.end()) {
            std::cout << "Command: " << command << " executed with operands: "
                      << operand1 << ", " << operand2 << "\n";
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
    }
}

int main() {
    initialize_opcode_lookup();
    repl();

    std::cout << "\nExecuting expanded AST interpreter:\n";
    AOTInterpreter interpreter;
    interpreter.run();

    return 0;
}
