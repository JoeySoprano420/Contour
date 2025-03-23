# Pseudo-code for a direct machine translation script

# Define the syntax-to-binary chart
syntax_to_binary = {
    'let x = 10;': 'MOV [0x1000], 10',
    'let y = x + 5;': 'MOV R1, [0x1000]; ADD R1, 5; MOV [0x1001], R1',
    'if x > 5 { ... }': 'CMP [0x1000], 5; JG TargetLabel; NOP',
    'for i in 0..10 { ... }': 'MOV R1, 0; LoopStart: CMP R1, 10; JG LoopEnd; INC R1; JMP LoopStart; LoopEnd:',
    'def add(a, b) { return a + b; }': 'CALL add; MOV result, R1;',
    'async fetchData(url);': 'CREATE THREAD fetchData(url); WAIT THREAD fetchData; MOV result, fetchedData;',
}

# Input source code (example)
source_code = """
let x = 10;
let y = x + 5;
if x > 5 {
    let z = x + 1;
}
"""

# Function to translate high-level syntax into binary instructions
def translate_to_binary(source_code):
    binary_code = []

    # Loop through each line in the source code
    for line in source_code.splitlines():
        line = line.strip()
        
        # Look up the corresponding binary instruction in the chart
        if line in syntax_to_binary:
            binary_code.append(syntax_to_binary[line])
        else:
            # Handle unknown lines or errors
            print(f"Error: Unrecognized syntax: {line}")
    
    return binary_code

# Generate binary code from the source code
binary_code = translate_to_binary(source_code)

# Output the binary instructions
for instruction in binary_code:
    print(instruction)

# In a real-world scenario, you would now write this binary code into an executable format
