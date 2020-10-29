import subprocess, os

test_cases = [
    {
        'description': 'sample test',
        'input': 'test/1.in',
        'output': 'test/1.out',
        'result': 'success'
    },
    {
        'description': 'invalid character',
        'input': 'test/2.in',
        'output': 'test/2.out',
        'result': 'error'
    }
]

def print_test_case(test_case, proc):
    print('<<<<<<<<<<<<<<<<<<<<<<<')
    with open(test_case['input'], 'r') as f:
        input_data = f.read().strip()
    with open(test_case['output'], 'r') as f:
        output_data = f.read().strip()
    if len(input_data) > 300:
        print(f'testcase {test_case["input"]} too large to display')
    else:
        print('input:')
        print(input_data)
    print('----------------------')
    print('expected answer:')
    print(output_data)
    print('----------------------')
    print('expected execution result:')
    print(test_case['result'])
    print('----------------------')
    print('your answer:')
    print(proc.stdout.decode().strip())
    print('----------------------')
    print('your return code:')
    print(proc.returncode)
    print('>>>>>>>>>>>>>>>>>>>>>>')


def main():
    subprocess.run(['make'], cwd='./src/')
    ok = 0
    for test_case in test_cases:
        proc = subprocess.run(['./src/scanner', test_case['input']], capture_output=True)
        with open(test_case['output'], 'r') as f:
            output = f.read().strip()
        return_code_matched = (test_case['result'] == 'success' and proc.returncode == 0) or (test_case['result'] == 'error' and proc.returncode != 0)
        output_matched = output == proc.stdout.decode().strip()
        if return_code_matched and output_matched:
            ok += 1
        else:
            print_test_case(test_case, proc)

    subprocess.run(['make', 'clean'], cwd='./src')
    assert ok == len(test_cases)

if __name__ == '__main__':
    main()
