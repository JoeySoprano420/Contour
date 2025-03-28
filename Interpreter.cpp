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

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <map>

using namespace std;

// Opcode Lookup Table (updated with new opcodes for loop structures)
unordered_map<string, uint8_t> opcode_lookup = {
    {"LET", 0x10}, {"ADD", 0x20}, {"SUBTRACT", 0x21}, {"MULTIPLY", 0x22}, {"DIVIDE", 0x23},
    {"MODULO", 0x24}, {"AND", 0x25}, {"OR", 0x26}, {"NOT", 0x27},
    {"IF", 0x30}, {"ELSE", 0x31}, {"JUMP", 0x32}, {"LOOP", 0x33},
    {"FUNC", 0x50}, {"CALL", 0x51}, {"RETURN", 0x52},
    {"BREAK", 0x34}, {"CONTINUE", 0x35}, {"PRINT", 0x40},
    {"WHILE", 0x60}, {"FOR", 0x61}
};

// Memory and Stack
vector<int64_t> global_memory(256, 0);  // Global memory
stack<unordered_map<string, int64_t>> local_stack;  // Stack for function calls and local variables

int max_recursion_depth = 50;  // Set max recursion depth
int current_recursion_depth = 0;  // Track the recursion depth

// Function Table: Maps function names to their AST representations
unordered_map<string, struct ASTNode*> function_table;

// Error handling utility
void throw_error(const string& msg) {
    throw runtime_error("Error: " + msg);
}

// Check memory bounds
void check_memory_bounds(size_t index) {
    if (index >= global_memory.size()) throw_error("Memory access out of bounds.");
}

// AST Node class with enhanced function-related fields
class ASTNode {
public:
    string command;
    int64_t value;               // For constants and loop counters
    vector<ASTNode*> children;   // Nested commands
    string function_name;        // For function nodes
    vector<int64_t> function_args;  // Function arguments
    ASTNode* condition;          // For conditional expressions in loops and if-else statements

    ASTNode(const string& cmd, int64_t val = 0) : command(cmd), value(val), condition(nullptr) {}
    ~ASTNode() {
        for (ASTNode* child : children) delete child;
        if (condition) delete condition;
    }
};

// Parse expressions into AST, including functions and control structures
ASTNode* parse_expression(stringstream& ss) {
    string cmd;
    ss >> cmd;

    ASTNode* root = new ASTNode(cmd);

    if (cmd == "FUNC") {
        ss >> root->function_name;  // Function name
        string param;
        while (ss >> param && param != "{") {
            root->function_args.push_back(stoi(param));  // Collect arguments
        }
        string block;
        while (getline(ss, block, ';')) {
            if (block == "END") break;  // End function definition
            stringstream block_stream(block);
            root->children.push_back(parse_expression(block_stream));
        }
        function_table[root->function_name] = root;  // Store function in table
    } else if (cmd == "CALL") {
        ss >> root->function_name;  // Function name to call
        int64_t arg;
        while (ss >> arg) root->function_args.push_back(arg);  // Collect arguments
    } else if (cmd == "RETURN") {
        ss >> root->value;  // Return value
    } else if (cmd == "WHILE") {
        // Parse the condition and body for the while loop
        root->condition = parse_expression(ss);
        string body;
        while (getline(ss, body, ';')) {
            if (body == "END") break;  // End of loop
            stringstream body_stream(body);
            root->children.push_back(parse_expression(body_stream));
        }
    } else if (cmd == "FOR") {
        // Parse the loop parameters (e.g., start, end, increment) and body
        int64_t start, end, increment;
        ss >> start >> end >> increment;
        root->value = start;
        root->children.push_back(parse_expression(ss));  // Parse loop body
        root->condition = new ASTNode("CONDITION", end);  // Store the loop end condition
    } else {
        int64_t param;
        while (ss >> param) root->children.push_back(new ASTNode("", param));
    }
    return root;
}

// Execute AST recursively with function handling, recursion detection, and stack-based memory
void execute_ast(ASTNode* root) {
    if (!root) return;

    try {
        if (root->command == "LET") {
            local_stack.top()[to_string(root->children[0]->value)] = root->children[1]->value;
        } else if (root->command == "ADD") {
            int64_t result = local_stack.top()[to_string(root->children[0]->value)] +
                             local_stack.top()[to_string(root->children[1]->value)];
            local_stack.top()[to_string(root->children[2]->value)] = result;
        } else if (root->command == "CALL") {
            if (function_table.find(root->function_name) == function_table.end())
                throw_error("Undefined function: " + root->function_name);

            // Recursion depth check
            if (current_recursion_depth >= max_recursion_depth)
                throw_error("Stack overflow due to too many recursive calls.");

            // Increase recursion depth
            current_recursion_depth++;

            // Create a new stack frame for the function call
            unordered_map<string, int64_t> new_frame;
            ASTNode* func = function_table[root->function_name];

            // Bind arguments to function parameters
            for (size_t i = 0; i < func->function_args.size(); ++i) {
                new_frame[to_string(func->function_args[i])] = root->function_args[i];
            }
            local_stack.push(new_frame);

            // Execute function body
            for (ASTNode* child : func->children) execute_ast(child);
            local_stack.pop();  // Remove stack frame after function execution

            // Decrease recursion depth
            current_recursion_depth--;
        } else if (root->command == "RETURN") {
            // Handle return value
            local_stack.top()[to_string(root->value)] = root->value;
        } else if (root->command == "WHILE") {
            // Execute while loop until condition is false
            while (local_stack.top()[to_string(root->condition->value)] != 0) {
                for (ASTNode* child : root->children) {
                    execute_ast(child);
                }
            }
        } else if (root->command == "FOR") {
            // Execute for loop (start -> end -> increment)
            for (int64_t i = root->value; i <= root->condition->value; i++) {
                local_stack.top()[to_string(i)] = i;  // Set loop counter
                for (ASTNode* child : root->children) {
                    execute_ast(child);
                }
            }
        } else if (root->command == "PRINT") {
            cout << "Output: " << local_stack.top()[to_string(root->children[0]->value)] << endl;
        } else {
            cerr << "Unknown command: " << root->command << endl;
        }
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
    }
}

// REPL for user interaction
void repl() {
    cout << "Extended REPL with Function Calls, Loops, and Error Handling (Type 'exit' to quit)\n";

    string input;
    while (true) {
        cout << ">> ";
        getline(cin, input);

        if (input == "exit") break;

        stringstream ss(input);
        ASTNode* ast = parse_expression(ss);
        execute_ast(ast);
        delete ast;
    }
}

int main() {
    cout << "Extended Virtual Machine with Functions, Loops, and Stack Overflow Prevention...\n";

    // Example hard-coded program with function definitions and loops
    cout << "Executing hard-coded function call test:\n";
    stringstream sample_code;
    sample_code << "FUNC factorial 0 { IF 0; RETURN 1; END; CALL factorial 0; MULTIPLY 0 0 0; } ";
    sample_code << "CALL factorial 5;";  // Call function 'factorial'

    ASTNode* root = parse_expression(sample_code);
    execute_ast(root);
    delete root;

    // Start REPL
    repl();

    return 0;
}

// New opcode entries
opcode_lookup["MALLOC"] = 0x70;
opcode_lookup["FREE"] = 0x71;

// Example function for dynamic memory allocation and use
void malloc_example() {
    int* array = malloc(sizeof(int) * 100);  // Allocate space for an array of 100 integers
    for (int i = 0; i < 100; ++i) {
        array[i] = i * 10;  // Assign values
    }
    free(array);  // Deallocate memory when done
}

void repl() {
    cout << "Extended REPL with Function Calls, Loops, and Error Handling (Type 'exit' to quit)\n";
    string input;

    while (true) {
        cout << ">> ";
        getline(cin, input);

        if (input == "exit") break;

        try {
            stringstream ss(input);
            ASTNode* ast = parse_expression(ss);
            execute_ast(ast);
            delete ast;
        } catch (const runtime_error& e) {
            cout << "Error: " << e.what() << endl;
        }
    }
}

// Example function listing command
void list_functions() {
    cout << "Defined functions: \n";
    for (const auto& entry : function_table) {
        cout << "Function: " << entry.first << "\n";
    }
}

class GCObject {
public:
    int ref_count = 0;  // Reference count

    void add_ref() { ref_count++; }
    void release() {
        ref_count--;
        if (ref_count == 0) {
            delete this;
        }
    }
};

// Implementing automatic garbage collection for allocated objects
void gc_collect() {
    // This is a simplified version. The actual GC process can be more complex.
    for (auto& obj : allocated_objects) {
        if (obj->ref_count == 0) {
            delete obj;
            allocated_objects.remove(obj);
        }
    }
}

// SWITCH/CASE opcode implementation
unordered_map<string, uint8_t> opcode_lookup = {
    // other opcodes...
    {"SWITCH", 0x80},
    {"CASE", 0x81},
    {"DEFAULT", 0x82},
};

void execute_switch_case(ASTNode* root) {
    // Switch statement evaluation
    int64_t switch_value = local_stack.top()[to_string(root->value)];
    for (ASTNode* child : root->children) {
        if (child->command == "CASE" && child->value == switch_value) {
            execute_ast(child);  // Execute corresponding case block
            break;
        }
    }
}

// Serialize program state to a file
void save_program_state(const string& filename) {
    ofstream out(filename, ios::binary);
    // Serialize the function table and memory state
    out.write(reinterpret_cast<char*>(&function_table), sizeof(function_table));
    out.close();
}

// Deserialize program state from a file
void load_program_state(const string& filename) {
    ifstream in(filename, ios::binary);
    // Deserialize the function table and memory state
    in.read(reinterpret_cast<char*>(&function_table), sizeof(function_table));
    in.close();
}

// Example: Adding Switch and Case operations in opcode table
opcode_lookup["SWITCH"] = 0x80;
opcode_lookup["CASE"] = 0x81;
opcode_lookup["DEFAULT"] = 0x82;

// Add Switch/Case handling in execution
void execute_switch_case(ASTNode* root) {
    if (!root || root->command != "SWITCH") return;
    
    int64_t switch_value = local_stack.top()[to_string(root->children[0]->value)];
    bool matched = false;

    // Traverse through case nodes
    for (ASTNode* child : root->children) {
        if (child->command == "CASE") {
            int64_t case_value = child->value;
            if (switch_value == case_value) {
                matched = true;
                for (ASTNode* case_child : child->children) {
                    execute_ast(case_child);  // Execute the matched case body
                }
            }
        } else if (child->command == "DEFAULT" && !matched) {
            for (ASTNode* default_child : child->children) {
                execute_ast(default_child);  // Execute the default body if no case matched
            }
        }
    }
}

#include <list>

// List of allocated objects
list<GCObject*> allocated_objects;

// Implementing automatic garbage collection for allocated objects
void gc_collect() {
    // Check reference count of all allocated objects and delete if ref_count == 0
    for (auto it = allocated_objects.begin(); it != allocated_objects.end(); ) {
        if ((*it)->ref_count == 0) {
            delete *it;
            it = allocated_objects.erase(it);  // Remove from list and continue
        } else {
            ++it;
        }
    }
}

class GCObject {
public:
    int ref_count = 0;  // Reference count

    // Increase reference count
    void add_ref() { 
        ref_count++; 
    }

    // Decrease reference count and delete if zero
    void release() { 
        ref_count--; 
        if (ref_count == 0) {
            delete this;
        }
    }

    // Static function to track allocated objects
    static void track(GCObject* obj) {
        allocated_objects.push_back(obj);
        obj->add_ref();  // Start tracking the object
    }
};

// Updated opcode for SWITCH/CASE handling
unordered_map<string, uint8_t> opcode_lookup = {
    // other opcodes...
    {"SWITCH", 0x80},
    {"CASE", 0x81},
    {"DEFAULT", 0x82}
};

// Parse the SWITCH/CASE block
ASTNode* parse_switch_case_block(stringstream& ss) {
    string cmd;
    ss >> cmd;

    if (cmd == "SWITCH") {
        ASTNode* switch_node = new ASTNode("SWITCH");
        int64_t value;
        ss >> value;  // The value to compare against

        while (true) {
            ss >> cmd;
            if (cmd == "CASE") {
                int64_t case_value;
                ss >> case_value;
                ASTNode* case_node = new ASTNode("CASE", case_value);
                string body;
                while (getline(ss, body, ';')) {
                    if (body == "END") break;  // End of the case block
                    stringstream body_stream(body);
                    case_node->children.push_back(parse_expression(body_stream));
                }
                switch_node->children.push_back(case_node);
            } else if (cmd == "DEFAULT") {
                ASTNode* default_node = new ASTNode("DEFAULT");
                string body;
                while (getline(ss, body, ';')) {
                    if (body == "END") break;  // End of the default block
                    stringstream body_stream(body);
                    default_node->children.push_back(parse_expression(body_stream));
                }
                switch_node->children.push_back(default_node);
            } else if (cmd == "END") {
                break;
            }
        }
        return switch_node;
    }

    return nullptr;
}

// Execute SWITCH/CASE in AST
void execute_switch_case(ASTNode* switch_node) {
    if (switch_node->command == "SWITCH") {
        int64_t value_to_check = switch_node->value;

        // Iterate over children (CASE or DEFAULT)
        for (ASTNode* child : switch_node->children) {
            if (child->command == "CASE") {
                if (child->value == value_to_check) {
                    // Execute the case block
                    for (ASTNode* case_child : child->children) {
                        execute_ast(case_child);
                    }
                    break;  // Exit after the first matching case
                }
            } else if (child->command == "DEFAULT") {
                // Execute default block if no case matched
                for (ASTNode* default_child : child->children) {
                    execute_ast(default_child);
                }
                break;  // Default only executes if no case matches
            }
        }
    }
}

// Execute AST (Updated to handle SWITCH/CASE)
void execute_ast(ASTNode* root) {
    if (!root) return;

    try {
        if (root->command == "SWITCH") {
            execute_switch_case(root);
        } else if (root->command == "LET") {
            local_stack.top()[to_string(root->children[0]->value)] = root->children[1]->value;
        } else if (root->command == "ADD") {
            int64_t result = local_stack.top()[to_string(root->children[0]->value)] +
                             local_stack.top()[to_string(root->children[1]->value)];
            local_stack.top()[to_string(root->children[2]->value)] = result;
        } else if (root->command == "CALL") {
            if (function_table.find(root->function_name) == function_table.end())
                throw_error("Undefined function: " + root->function_name);

            if (current_recursion_depth >= max_recursion_depth)
                throw_error("Stack overflow due to too many recursive calls.");

            current_recursion_depth++;

            unordered_map<string, int64_t> new_frame;
            ASTNode* func = function_table[root->function_name];

            for (size_t i = 0; i < func->function_args.size(); ++i) {
                new_frame[to_string(func->function_args[i])] = root->function_args[i];
            }
            local_stack.push(new_frame);

            for (ASTNode* child : func->children) execute_ast(child);
            local_stack.pop();

            current_recursion_depth--;
        } else if (root->command == "PRINT") {
            cout << "Output: " << local_stack.top()[to_string(root->children[0]->value)] << endl;
        } else {
            cerr << "Unknown command: " << root->command << endl;
        }
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
    }
}

class GCObject {
public:
    int ref_count = 0;

    void add_ref() { ref_count++; }
    void release() {
        ref_count--;
        if (ref_count == 0) {
            delete this;
        }
    }
};

// Sample dynamic memory allocation and deallocation
void malloc_example() {
    int* array = (int*)malloc(sizeof(int) * 100);  // Allocate space for an array of 100 integers
    for (int i = 0; i < 100; ++i) {
        array[i] = i * 10;  // Assign values
    }
    free(array);  // Deallocate memory when done
}

// Add GC collection (simplified)
void gc_collect() {
    for (auto& obj : allocated_objects) {
        if (obj->ref_count == 0) {
            delete obj;
            allocated_objects.remove(obj);
        }
    }
}

•	Implement more advanced memory management techniques such as generational garbage collection.
	•	Enhance loop and conditional handling to support a wider range of control structures.

void execute_switch_case(ASTNode* root) {
    if (root->command != "SWITCH") return;

    // Assuming the first child is the expression being evaluated (condition for SWITCH)
    int64_t condition_value = local_stack.top()[to_string(root->children[0]->value)];
    
    // Iterate over the CASE nodes
    for (ASTNode* case_node : root->children) {
        if (case_node->command == "CASE") {
            int64_t case_value = case_node->value;
            if (condition_value == case_value) {
                // Execute the case block if the value matches
                for (ASTNode* case_child : case_node->children) {
                    execute_ast(case_child);
                }
                break;
            }
        }
    }
}

void execute_ast(ASTNode* root) {
    if (!root) return;

    try {
        if (root->command == "LET") {
            // Handle LET command
        } else if (root->command == "ADD") {
            // Handle ADD command
        } else if (root->command == "SWITCH") {
            execute_switch_case(root);  // Handle SWITCH/CASE command
        } else {
            // Other commands
        }
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
    }
}

class GCObject {
public:
    int ref_count;

    GCObject() : ref_count(0) {}

    void retain() {
        ref_count++;
    }

    void release() {
        ref_count--;
        if (ref_count == 0) {
            // Perform cleanup or delete object
            delete this;
        }
    }
};

void gc_collect(std::vector<GCObject*>& allocated_objects) {
    for (auto it = allocated_objects.begin(); it != allocated_objects.end();) {
        if ((*it)->ref_count == 0) {
            delete *it;
            it = allocated_objects.erase(it);
        } else {
            ++it;
        }
    }
}

void* vm_malloc(size_t size) {
    void* ptr = malloc(size);
    GCObject* obj = new GCObject();
    memory_map[static_cast<int*>(ptr)] = obj;
    return ptr;
}

void vm_free(void* ptr) {
    auto it = memory_map.find(static_cast<int*>(ptr));
    if (it != memory_map.end()) {
        it->second->release();
        memory_map.erase(it);
        free(ptr);
    }
}

void* vm_malloc(size_t size) {
    void* ptr = malloc(size);
    GCObject* obj = new GCObject();
    memory_map[static_cast<int*>(ptr)] = obj;
    return ptr;
}

void vm_free(void* ptr) {
    auto it = memory_map.find(static_cast<int*>(ptr));
    if (it != memory_map.end()) {
        it->second->release();
        memory_map.erase(it);
        free(ptr);
    }
}

void call_function(const std::string& function_name, int arg) {
    Function* func = function_table[function_name];
    // Set up the arguments, execute the function, and handle recursion or depth checks.
}

bool is_valid_command(const std::string& command) {
    // Validate the command structure and arguments
    if (command.empty() || command[0] != 'C') {
        return false;  // Example error check
    }
    return true;
}

void parse_command(const std::string& input) {
    if (!is_valid_command(input)) {
        std::cerr << "Error: Invalid command syntax!" << std::endl;
        return;
    }
    // Proceed with normal parsing
}

// Handle SWITCH/CASE logic
else if (root->command == "SWITCH") {
    int64_t switch_value = local_stack.top()[to_string(root->children[0]->value)];
    bool case_matched = false;
    for (size_t i = 1; i < root->children.size(); i++) {
        if (root->children[i]->command == "CASE") {
            int64_t case_value = root->children[i]->value;
            if (switch_value == case_value) {
                case_matched = true;
            }
        } else if (root->children[i]->command == "DEFAULT" && !case_matched) {
            case_matched = true; // Execute default if no case matched
        }
        if (case_matched) {
            for (ASTNode* child : root->children[i]->children) {
                execute_ast(child);
            }
        }
    }
}

else if (root->command == "MALLOC") {
    int64_t size = root->children[0]->value;
    void* ptr = malloc(size);
    local_stack.top()[to_string(root->children[1]->value)] = (int64_t)ptr;
} else if (root->command == "FREE") {
    void* ptr = (void*)local_stack.top()[to_string(root->children[0]->value)];
    free(ptr);
}

unordered_set<GCObject*> allocated_objects;

// When allocating a new object:
GCObject* new_obj = new GCObject();
allocated_objects.insert(new_obj);

// During garbage collection:
void gc_collect() {
    for (auto it = allocated_objects.begin(); it != allocated_objects.end();) {
        if ((*it)->ref_count == 0) {
            delete *it;
            it = allocated_objects.erase(it);
        } else {
            ++it;
        }
    }
}

cout << "Current recursion depth: " << current_recursion_depth << endl;

void throw_error(const string& msg, const string& location = "Unknown") {
    throw runtime_error("Error at " + location + ": " + msg);
}

void memory_status() {
    cout << "Memory usage:\n";
    for (size_t i = 0; i < global_memory.size(); ++i) {
        if (global_memory[i] != 0) {
            cout << "Memory[" << i << "]: " << global_memory[i] << endl;
        }
    }
}

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <map>
#include <memory>  // For smart pointers

using namespace std;

// Opcode Lookup Table (updated with new opcodes for loop structures)
unordered_map<string, uint8_t> opcode_lookup = {
    {"LET", 0x10}, {"ADD", 0x20}, {"SUBTRACT", 0x21}, {"MULTIPLY", 0x22}, {"DIVIDE", 0x23},
    {"MODULO", 0x24}, {"AND", 0x25}, {"OR", 0x26}, {"NOT", 0x27},
    {"IF", 0x30}, {"ELSE", 0x31}, {"JUMP", 0x32}, {"LOOP", 0x33},
    {"FUNC", 0x50}, {"CALL", 0x51}, {"RETURN", 0x52},
    {"BREAK", 0x34}, {"CONTINUE", 0x35}, {"PRINT", 0x40},
    {"WHILE", 0x60}, {"FOR", 0x61}
};

// Memory and Stack
vector<int64_t> global_memory(256, 0);  // Global memory
stack<unordered_map<string, int64_t>> local_stack;  // Stack for function calls and local variables

int max_recursion_depth = 50;  // Set max recursion depth
int current_recursion_depth = 0;  // Track the recursion depth

// Function Table: Maps function names to their AST representations
unordered_map<string, shared_ptr<struct ASTNode>> function_table;

// Error handling utility
void throw_error(const string& msg, const string& context = "") {
    if (!context.empty()) {
        throw runtime_error("Error: " + msg + " Context: " + context);
    }
    else {
        throw runtime_error("Error: " + msg);
    }
}

// Check memory bounds
void check_memory_bounds(size_t index, const string& context) {
    if (index >= global_memory.size()) throw_error("Memory access out of bounds.", context);
}

// AST Node class with enhanced function-related fields
class ASTNode {
public:
    string command;
    int64_t value;               // For constants and loop counters
    vector<shared_ptr<ASTNode>> children;   // Nested commands
    string function_name;        // For function nodes
    vector<int64_t> function_args;  // Function arguments
    shared_ptr<ASTNode> condition;          // For conditional expressions in loops and if-else statements

    ASTNode(const string& cmd, int64_t val = 0) : command(cmd), value(val), condition(nullptr) {}
};

// Parse expressions into AST, including functions and control structures
shared_ptr<ASTNode> parse_expression(stringstream& ss) {
    string cmd;
    ss >> cmd;

    auto root = make_shared<ASTNode>(cmd);

    if (cmd == "FUNC") {
        ss >> root->function_name;  // Function name
        string param;
        while (ss >> param && param != "{") {
            root->function_args.push_back(stoi(param));  // Collect arguments
        }
        string block;
        while (getline(ss, block, ';')) {
            if (block == "END") break;  // End function definition
            stringstream block_stream(block);
            root->children.push_back(parse_expression(block_stream));
        }
        function_table[root->function_name] = root;  // Store function in table
    } else if (cmd == "CALL") {
        ss >> root->function_name;  // Function name to call
        int64_t arg;
        while (ss >> arg) root->function_args.push_back(arg);  // Collect arguments
    } else if (cmd == "RETURN") {
        ss >> root->value;  // Return value
    } else if (cmd == "WHILE") {
        // Parse the condition and body for the while loop
        root->condition = parse_expression(ss);
        string body;
        while (getline(ss, body, ';')) {
            if (body == "END") break;  // End of loop
            stringstream body_stream(body);
            root->children.push_back(parse_expression(body_stream));
        }
    } else if (cmd == "FOR") {
        // Parse the loop parameters (e.g., start, end, increment) and body
        int64_t start, end, increment;
        ss >> start >> end >> increment;
        root->value = start;
        root->children.push_back(parse_expression(ss));  // Parse loop body
        root->condition = make_shared<ASTNode>("CONDITION", end);  // Store the loop end condition
    } else {
        int64_t param;
        while (ss >> param) root->children.push_back(make_shared<ASTNode>("", param));
    }
    return root;
}

// Execute AST recursively with function handling, recursion detection, and stack-based memory
void execute_ast(shared_ptr<ASTNode> root, const string& context = "") {
    if (!root) return;

    try {
        if (root->command == "LET") {
            local_stack.top()[to_string(root->children[0]->value)] = root->children[1]->value;
        } else if (root->command == "ADD") {
            int64_t result = local_stack.top()[to_string(root->children[0]->value)] +
                             local_stack.top()[to_string(root->children[1]->value)];
            local_stack.top()[to_string(root->children[2]->value)] = result;
        } else if (root->command == "CALL") {
            if (function_table.find(root->function_name) == function_table.end())
                throw_error("Undefined function: " + root->function_name, context);

            // Recursion depth check
            if (current_recursion_depth >= max_recursion_depth)
                throw_error("Stack overflow due to too many recursive calls.", context);

            // Increase recursion depth
            current_recursion_depth++;

            // Create a new stack frame for the function call
            unordered_map<string, int64_t> new_frame;
            auto func = function_table[root->function_name];

            // Bind arguments to function parameters
            for (size_t i = 0; i < func->function_args.size(); ++i) {
                new_frame[to_string(func->function_args[i])] = root->function_args[i];
            }
            local_stack.push(new_frame);

            // Execute function body
            for (auto& child : func->children) execute_ast(child, "Inside function " + root->function_name);
            local_stack.pop();  // Remove stack frame after function execution

            // Decrease recursion depth
            current_recursion_depth--;
        } else if (root->command == "RETURN") {
            // Handle return value
            local_stack.top()[to_string(root->value)] = root->value;
        } else if (root->command == "WHILE") {
            // Execute while loop until condition is false
            while (local_stack.top()[to_string(root->condition->value)] != 0) {
                for (auto& child : root->children) {
                    execute_ast(child, "Inside while loop");
                }
            }
        } else if (root->command == "FOR") {
            // Execute for loop (start -> end -> increment)
            for (int64_t i = root->value; i <= root->condition->value; i++) {
                local_stack.top()[to_string(i)] = i;  // Set loop counter
                for (auto& child : root->children) {
                    execute_ast(child, "Inside for loop");
                }
            }
        } else if (root->command == "PRINT") {
            cout << "Output: " << local_stack.top()[to_string(root->children[0]->value)] << endl;
        } else {
            cerr << "Unknown command: " << root->command << endl;
        }
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
    }
}

// REPL for user interaction
void repl() {
    cout << "Extended REPL with Function Calls, Loops, and Error Handling (Type 'exit' to quit)\n";

    string input;
    while (true) {
        cout << ">> ";
        getline(cin, input);

        if (input == "exit") break;

        try {
            stringstream ss(input);
            auto ast = parse_expression(ss);
            execute_ast(ast, "Main REPL execution");
        } catch (const runtime_error& e) {
            cout << "Error: " << e.what() << endl;
        }
    }
}

// Garbage collection example
class GCObject {
public:
    int ref_count = 0;  // Reference count

    void add_ref() { ref_count++; }
    void release() {
        ref_count--;
        if (ref_count == 0) {
            cout << "Garbage collecting object\n";
            delete this;
        }
    }
};

// Main function
int main() {
    // Start REPL for testing
    repl();
    return 0;
}

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <map>
#include <fstream> // For serialization and deserialization
#include <set>     // For garbage collection

using namespace std;

// Opcode Lookup Table (Enhanced for SWITCH and dynamic memory)
unordered_map<string, uint8_t> opcode_lookup = {
    {"LET", 0x10}, {"ADD", 0x20}, {"PRINT", 0x40}, {"CALL", 0x51}, {"RETURN", 0x52},
    {"WHILE", 0x60}, {"FOR", 0x61}, {"SWITCH", 0x70}, {"CASE", 0x71}, {"DEFAULT", 0x72},
    {"MALLOC", 0x80}, {"FREE", 0x81}, {"SAVE", 0x90}, {"LOAD", 0x91}
};

// Memory Structures
vector<int64_t> global_memory(256, 0); // Global memory
unordered_map<size_t, int64_t*> dynamic_memory; // Dynamic memory allocation

stack<unordered_map<string, int64_t>> local_stack; // Local variables
int max_recursion_depth = 50, current_recursion_depth = 0; // Recursion depth

// Garbage Collection Tracking
set<int64_t*> memory_references; // Track allocated dynamic memory

// Function Table
unordered_map<string, struct ASTNode*> function_table;

void throw_error(const string& msg) {
    throw runtime_error("Error: " + msg);
}

// AST Node Class
class ASTNode {
public:
    string command;
    int64_t value;
    vector<ASTNode*> children;
    ASTNode* condition;

    ASTNode(const string& cmd, int64_t val = 0) : command(cmd), value(val), condition(nullptr) {}
    ~ASTNode() {
        for (ASTNode* child : children) delete child;
        if (condition) delete condition;
    }
};

// Serialize AST to file
void serialize_ast(ASTNode* root, ostream& out) {
    if (!root) return;
    out << root->command << " " << root->value << " ";
    out << root->children.size() << " ";
    for (ASTNode* child : root->children) {
        serialize_ast(child, out);
    }
}

// Deserialize AST from file
ASTNode* deserialize_ast(istream& in) {
    string cmd;
    int64_t val, num_children;
    in >> cmd >> val >> num_children;
    ASTNode* root = new ASTNode(cmd, val);
    for (size_t i = 0; i < num_children; ++i) {
        root->children.push_back(deserialize_ast(in));
    }
    return root;
}

// Garbage Collection: Free unused dynamic memory
void garbage_collect() {
    cout << "Running garbage collector...\n";
    for (auto ref : memory_references) {
        delete ref; // Free memory
    }
    memory_references.clear();
}

// Dynamic Memory Allocation
int64_t* allocate_dynamic(int64_t size) {
    int64_t* block = new int64_t[size];
    memory_references.insert(block);
    return block;
}

void free_dynamic(int64_t* block) {
    if (memory_references.find(block) != memory_references.end()) {
        delete[] block;
        memory_references.erase(block);
    } else {
        throw_error("Attempted to free unallocated memory.");
    }
}

// AST Execution
void execute_ast(ASTNode* root) {
    if (!root) return;
    if (root->command == "LET") {
        local_stack.top()[to_string(root->children[0]->value)] = root->children[1]->value;
    } else if (root->command == "ADD") {
        int64_t result = local_stack.top()[to_string(root->children[0]->value)] +
                         local_stack.top()[to_string(root->children[1]->value)];
        local_stack.top()[to_string(root->children[2]->value)] = result;
    } else if (root->command == "MALLOC") {
        int64_t size = root->children[0]->value;
        int64_t* block = allocate_dynamic(size);
        local_stack.top()[to_string(root->children[1]->value)] = reinterpret_cast<int64_t>(block);
    } else if (root->command == "FREE") {
        int64_t* block = reinterpret_cast<int64_t*>(local_stack.top()[to_string(root->children[0]->value)]);
        free_dynamic(block);
    } else if (root->command == "SWITCH") {
        bool matched = false;
        for (ASTNode* child : root->children) {
            if (child->command == "CASE" &&
                local_stack.top()[to_string(root->value)] == child->value) {
                execute_ast(child);
                matched = true;
                break;
            }
        }
        if (!matched) {
            for (ASTNode* child : root->children) {
                if (child->command == "DEFAULT") {
                    execute_ast(child);
                }
            }
        }
    } else if (root->command == "PRINT") {
        cout << "Output: " << local_stack.top()[to_string(root->children[0]->value)] << endl;
    }
}

// Enhanced REPL
void repl() {
    cout << "REPL with serialization, memory handling, and garbage collection. Type 'exit' to quit.\n";

    string input;
    while (true) {
        cout << ">> ";
        getline(cin, input);
        if (input == "exit") break;

        stringstream ss(input);
        if (input.substr(0, 4) == "SAVE") {
            ofstream out("program.sav");
            serialize_ast(function_table["main"], out); // Example: Save main function
            out.close();
            cout << "Program saved.\n";
        } else if (input.substr(0, 4) == "LOAD") {
            ifstream in("program.sav");
            function_table["main"] = deserialize_ast(in); // Example: Load main function
            in.close();
            cout << "Program loaded.\n";
        } else {
            ASTNode* ast = deserialize_ast(ss);
            execute_ast(ast);
            delete ast;
        }
    }
}

int main() {
    cout << "Starting enhanced Virtual Machine...\n";
    repl(); // Launch REPL
    garbage_collect(); // Cleanup
    return 0;
}

void check_valid_opcode(uint8_t opcode) {
    if (opcode_lookup.find(std::to_string(opcode)) == opcode_lookup.end()) {
        throw std::runtime_error("Error: Unknown opcode encountered - 0x" + std::to_string(opcode));
    }
}

void check_memory_bounds(size_t index) {
    if (index >= memory.size()) {
        throw std::out_of_range("Error: Memory access out of bounds!");
    }
}

void allocate_variable(const std::string &var, int64_t value) {
    if (reference_table.find(var) == reference_table.end()) {
        reference_table[var] = memory_index++;
    }
    check_memory_bounds(reference_table[var]);
    memory[reference_table[var]] = value;
    
    // Additional memory management steps
    // This could include dynamically resizing memory or implementing garbage collection
}

void execute_command(const std::string& command, int64_t operand1, int64_t operand2) {
    if (opcode_lookup.find(command) != opcode_lookup.end()) {
        std::cout << "Command: " << command << " executed with operands: "
                  << operand1 << ", " << operand2 << "\n";
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
    }
}

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
        execute_command(command, operand1, operand2);
    }
}

/**
 * Load JSON data from a file.
 * 
 * @param file_name The name of the file to load JSON data from.
 * @return The loaded JSON data.
 * @throws std::runtime_error if the file cannot be opened.
 */
json load_json(const std::string &file_name) {
    std::ifstream file(file_name);
    json data;
    if (!file) {
        throw std::runtime_error("Error: Could not open " + file_name);
    }
    file >> data;
    return data;
}

void load_binary_program(const std::string &file_name) {
    std::ifstream file(file_name);
    if (!file) {
        throw std::runtime_error("Error: Could not open binary program file.");
    }

    int64_t opcode;
    binary_program.reserve(256); // Reserve memory to avoid frequent reallocations
    while (file >> opcode) {
        std::vector<int64_t> instruction;
        instruction.reserve(4); // Reserve space for opcode and three parameters
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

#include <cassert>

void test_load_json() {
    json data = load_json("test.json");
    assert(data.is_object());
    assert(data["key"] == "value");
}

void test_opcode_lookup() {
    initialize_opcode_lookup();
    assert(opcode_lookup["LET"] == 0x10);
}

int main() {
    test_load_json();
    test_opcode_lookup();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}







// Libraries for core functionality
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <future>
#include <chrono>
#include <cassert>
#include <exception>
#include <stdexcept>
#include <openssl/aes.h>  // External SSL/TLS encryption library (example for AES)

// Utility macros for security, optimization, and memory control
#define FORCE_INLINE __attribute__((always_inline))
#define NO_OPTIMIZE __attribute__((optimize("no-tree-vectorize")))
#define SECURE_MEMORY __attribute__((section(".secure")))

// Type alias for easy use
using Uint = unsigned int;

// Define the secure register access mechanism
class SecureRegister {
public:
    static std::atomic<Uint> registerData;
    
    static void set(Uint value) {
        registerData.store(value, std::memory_order_relaxed);
    }

    static Uint get() {
        return registerData.load(std::memory_order_relaxed);
    }
};

// Initialize the atomic register
std::atomic<Uint> SecureRegister::registerData{0};

// RAII class for memory management (Garbage Collection)
class RAII_GarbageCollector {
public:
    RAII_GarbageCollector() {
        // Allocate resources dynamically or track memory
    }

    ~RAII_GarbageCollector() {
        // Ensure resources are cleaned up when going out of scope
    }

    void cleanup() {
        // Force manual garbage collection if necessary
    }
};

// A basic class structure to represent entities with async features
class AsyncEntity {
public:
    virtual void execute() = 0;
};

// The AsyncHandler will manage async processing with channel synchronization
class AsyncHandler {
private:
    std::vector<std::future<void>> futures;  // Use future for async task synchronization
    std::mutex mtx; // Mutex for safe memory access

public:
    template<typename Callable>
    void async_task(Callable&& task) {
        futures.push_back(std::async(std::launch::async, [=](){
            std::lock_guard<std::mutex> lock(mtx);
            task();
        }));
    }

    void wait() {
        for (auto& fut : futures) {
            fut.get();  // Ensure all tasks are completed
        }
    }

    ~AsyncHandler() {
        wait();
    }
};

// High-level structure for secure and optimized async communication
class SecureAsyncHandler : public AsyncHandler {
private:
    // Specific security mechanisms for data integrity
    void secure_async_task(std::function<void()> task) {
        // Encrypt and obfuscate logic before executing the task
        task();
    }

public:
    template<typename Callable>
    void secure_task(Callable&& task) {
        secure_async_task([=]() {
            std::lock_guard<std::mutex> lock(mtx);
            task();
        });
    }

    // Integrating external security libraries (e.g., SSL/TLS or custom encryption algorithms)
    static std::string encrypt_data(const std::string& data) {
        // Example of AES encryption (using OpenSSL)
        AES_KEY encrypt_key;
        unsigned char key[16] = {'S', 'e', 'c', 'r', 'e', 't', 'K', 'e', 'y', '1', '2', '3', '4', '5', '6', '7'};
        unsigned char iv[16] = {'I', 'n', 'i', 't', 'V', 'e', 'c', 't', 'o', 'r', '1', '2', '3', '4', '5', '6'};

        unsigned char enc_out[128];
        AES_set_encrypt_key(key, 128, &encrypt_key);
        AES_cbc_encrypt(reinterpret_cast<const unsigned char*>(data.c_str()), enc_out, data.size(), &encrypt_key, iv, AES_ENCRYPT);

        return std::string(reinterpret_cast<char*>(enc_out), data.size());  // Returning encrypted string
    }

    static bool verify_data_integrity(const std::string& original_data, const std::string& encrypted_data) {
        // Example integrity check (using simple hash function or checksum)
        return original_data.size() == encrypted_data.size();  // Simplified example
    }
};

// Dummy example of a class that could use zero-copy reusable cache nodes
class CacheSystem {
public:
    static void cache_data(const std::string& data) {
        // Zero-copy cache deposit logic
        std::cout << "Data cached: " << data << std::endl;
    }

    static std::string fetch_data(const std::string& key) {
        // Simulate fetching cached data
        return "Cached Data for: " + key;
    }
};

// Memory isolation and sandboxing class to prevent leaks and injections
class MemorySandbox {
public:
    void isolate_memory() {
        // Dummy code to show isolation process
        std::cout << "Memory isolated for sandboxing." << std::endl;
    }

    void defuse_injection_attempts() {
        // Handle potential intrusions with precise error handling and masking
        std::cout << "Injection detection and defusal." << std::endl;
    }
};

// Advanced error handling class
class ErrorHandler {
public:
    // Handle general exceptions and log the error
    static void handle_exception(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        // Perform security measures like masking sensitive information
    }

    // Handle custom errors for task-specific issues
    static void handle_task_error(const std::string& task_name, const std::exception& e) {
        std::cerr << "Task '" << task_name << "' failed with error: " << e.what() << std::endl;
    }
};

// Secure Memory with encryption and masking
class SecureMemory {
public:
    static std::string encrypt_data(const std::string& data) {
        // Example of AES encryption (using OpenSSL)
        return SecureAsyncHandler::encrypt_data(data);
    }

    static std::string mask_data(const std::string& data) {
        // Simulate data masking for sensitive information
        return "******";
    }
};

// Thread pool for parallel execution
class ThreadPool {
private:
    std::vector<std::thread> workers;
    bool is_running = true;

public:
    explicit ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.push_back(std::thread([this] {
                while (is_running) {
                    // Thread work here
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }));
        }
    }

    void stop() {
        is_running = false;
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ~ThreadPool() {
        stop();
    }
};

// Custom memory pool to manage dynamic allocation
class CustomMemoryPool {
private:
    std::vector<void*> allocated_memory;

public:
    void* allocate(size_t size) {
        void* memory = malloc(size);
        allocated_memory.push_back(memory);
        return memory;
    }

    void deallocate(void* memory) {
        free(memory);
        // Remove the memory from tracking list
        auto it = std::remove(allocated_memory.begin(), allocated_memory.end(), memory);
        allocated_memory.erase(it, allocated_memory.end());
    }

    ~CustomMemoryPool() {
        // Free all remaining memory
        for (void* memory : allocated_memory) {
            free(memory);
        }
    }
};

// Main function to demonstrate the framework
int main() {
    // Initialize memory sandbox
    MemorySandbox sandbox;
    sandbox.isolate_memory();

    // Handle a task with enhanced error handling
    try {
        // Simulate async task execution
        SecureAsyncHandler secureHandler;
        secureHandler.secure_task([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Secure Async Task Executed" << std::endl;
        });

        // Handling zero-copy cache with static store
        CacheSystem::cache_data("Important Data");
        std::cout << CacheSystem::fetch_data("SomeKey") << std::endl;

        // Demonstrating encryption and integrity check
        std::string sensitive_data = "Confidential Information";
        std::string encrypted_data = SecureMemory::encrypt_data(sensitive_data);
        std::cout << "Encrypted Data: " << encrypted_data << std::endl;
        
        // Verify data integrity
        bool integrity_check = SecureAsyncHandler::verify_data_integrity(sensitive_data, encrypted_data);
        std::cout << "Data Integrity Check Passed: " << (integrity_check ? "Yes" : "No") << std::endl;

        // Demonstrating secure register
        SecureRegister::set(12345);
        std::cout << "Secure Register Value: " << SecureRegister::get() << std::endl;

        // Create and start a thread pool
        ThreadPool pool(4);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        pool.stop();
    } catch (const std::exception& e) {
        ErrorHandler::handle_exception(e);
    }

    // Custom memory pool usage
    CustomMemoryPool memory_pool;
    void* block = memory_pool.allocate(1024);
    memory_pool.deallocate(block);

    return 0;
}
