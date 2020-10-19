import subprocess, os

test_cases = [
    {
        'description': 'reserved word i',
        'input': 'i i\n'
    },
    {
        'description': 'reserved word f',
        'input': 'f f\n'
    },
    {
        'description': 'reserved word p',
        'input': 'i p\n'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\na = 1++1\n'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\na = a//a\n'
    },
    {
        'description': 'unbalanced parentheses',
        'input': 'i a\na = (a))\n'
    },
    {
        'description': 'expect assignment operator',
        'input': 'i a\na a\n'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\na + a\n'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\np a = 1\n'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\np\n'
    },
]

def print_test_case(test_case):
    print('>>>>>>>>>>>>>>>>>>>>')
    print(test_case['descryption'])
    print('---------------------')
    print(test_case['input'])
    print('<<<<<<<<<<<<<<<<<<<<')

def main():
    subprocess.run(['make'], cwd='./src/')
    ok = 0
    for test_case in test_cases:
        with open('.tmp_input.ac', 'w') as f:
            f.write(test_case['input'])
        proc = subprocess.run(['./src/AcDc', '.tmp_input.ac', '.tmp_output.dc'])
        if proc.returncode == 0:
            print_test_case(test_case)
        else:
            ok += 1

    print('======= SUMMARY ========')
    print(f'test cases: {ok}/{len(test_cases)} OK')
    print('========================')

    os.remove('.tmp_input.ac')
    os.remove('.tmp_output.dc')
    subprocess.run(['make', 'clean'], cwd='./src/')

if __name__ == '__main__':
    main()