#!/usr/bin/env python3
"""
json_to_cpp_tests.py

Generates C++ header file with test functions from JSON test suite.
Output is designed to be #included in ChordDetectionTests.cpp

Usage:
    python json_to_cpp_tests.py chord_tests.json > generated_tests_from_json.h
"""

import json
import sys
from typing import List, Dict

def escape_string(s: str) -> str:
    """Escape string for C++ string literals."""
    return s.replace('\\', '\\\\').replace('"', '\\"')

def generate_cpp_test_function(category: Dict) -> str:
    """Generate a C++ function for a single test category."""
    
    # Create function name from category name
    func_name = category['category'].replace(' ', '').replace('/', '').replace('-', '')
    func_name = f"getGenerated{func_name}Tests"
    
    cpp_code = f"// {category['category']} ({len(category['tests'])} tests)\n"
    cpp_code += f"std::vector<TestCase> {func_name}()\n"
    cpp_code += "{\n"
    cpp_code += "    return {\n"
    
    for test in category['tests']:
        name = escape_string(test['name'])
        expected = escape_string(test['expected'])
        min_conf = test['minConfidence']
        
        # Format notes list
        notes_list = ', '.join([f'"{note}"' for note in test['notes']])
        
        cpp_code += f'        makeTest("{name}", {{{notes_list}}}, "{expected}", {min_conf}f),\n'
    
    cpp_code += "    };\n"
    cpp_code += "}\n\n"
    
    return cpp_code

def generate_runner_function(categories: List[str]) -> str:
    """Generate function to run all JSON-based tests."""
    
    cpp_code = "// Run all JSON-generated tests\n"
    cpp_code += "void runAllGeneratedTests(ChordTestRunner& runner)\n"
    cpp_code += "{\n"
    
    for category_name in categories:
        func_name = category_name.replace(' ', '').replace('/', '').replace('-', '')
        func_name = f"getGenerated{func_name}Tests"
        
        cpp_code += f'    std::cout << std::endl << "--- {category_name} (Generated) ---" << std::endl;\n'
        cpp_code += f'    for (const auto& tc : {func_name}())\n'
        cpp_code += f'        runner.runTest(tc);\n'
        cpp_code += '\n'
    
    cpp_code += "}\n\n"
    
    return cpp_code

def main():
    if len(sys.argv) < 2:
        print("Usage: python json_to_cpp_tests.py <json_file>", file=sys.stderr)
        sys.exit(1)
    
    json_file = sys.argv[1]
    
    try:
        with open(json_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File '{json_file}' not found", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON - {e}", file=sys.stderr)
        sys.exit(1)
    
    # Generate header guard
    print("// AUTO-GENERATED HEADER - DO NOT EDIT")
    print("// Generated from: chord_tests.json")
    print(f"// Version: {data.get('metadata', {}).get('version', 'unknown')}")
    print("")
    print("#ifndef GENERATED_TESTS_FROM_JSON_H")
    print("#define GENERATED_TESTS_FROM_JSON_H")
    print("")
    print("// This file contains test functions generated from chord_tests.json")
    print("// It provides comprehensive chromatic coverage for chord detection")
    print("")
    
    # Generate test functions
    category_names = []
    for chord_category in data['chords']:
        category_names.append(chord_category['category'])
        print(generate_cpp_test_function(chord_category))
    
    # Generate runner function
    print(generate_runner_function(category_names))
    
    # Generate statistics
    total_tests = sum(len(cat['tests']) for cat in data['chords'])
    print(f"// Statistics: {len(data['chords'])} categories, {total_tests} total tests")
    print("")
    print("#endif // GENERATED_TESTS_FROM_JSON_H")

if __name__ == '__main__':
    main()
