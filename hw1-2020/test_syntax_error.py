import subprocess, os

test_cases = [
    {
        'description': 'reserved word i',
        'input': 'i i\n',
        'result': 'error'
    },
    {
        'description': 'reserved word f',
        'input': 'f f\n',
        'result': 'error'
    },
    {
        'description': 'reserved word p',
        'input': 'i p\n',
        'result': 'error'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\na = 1++1\n',
        'result': 'error'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\na = a//a\n',
        'result': 'error'
    },
    {
        'description': 'unbalanced parentheses',
        'input': 'i a\na = (a))\n',
        'result': 'error'
    },
    {
        'description': 'expect assignment operator',
        'input': 'i a\na a\n',
        'result': 'error'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\na + a\n',
        'result': 'error'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\np a = 1\n',
        'result': 'error'
    },
    {
        'description': 'invalid syntax',
        'input': 'i a\np\n',
        'result': 'error'
    },
    {
        'description': 'identifier not declared',
        'input': 'a = 1\n',
        'result': 'error'
    },
    {
        'description': 'convert float to int',
        'input': 'i a\na = 1.1\n',
        'result': 'error'
    },
    {
        'description': 'invalid token',
        'input': 'i a\na = 1;\n',
        'result': 'error'
    },
    {
        'description': 'floating number representation',
        'input': 'f a\na = 1.\n',
        'result': 'error'
    },
    {
        'description': 'floating number representation',
        'input': 'f a\na = .0\n',
        'result': 'error'
    },
    {
        'description': 'empty file',
        'input': '',
        'result': 'success'
    },
    {
        'description': 'sample test 2',
        'input': 'i a\nf b\ni c\ni d\na = 1\nb = 2.1\nc = 1\nd = 2\nc = a + d\np c\np d\n',
        'result': 'success'
    },
    {
        'description': 'declaration after statement',
        'input': 'f b\ni a\nb = 3.5\na = 2\nf c\n',
        'result': 'error'
    },
    {
        'description': 'variable not declared before use',
        'input': 'a = 5 + 3.2\nb = 3.5\n',
        'result': 'error'
    },
    {
        'description': 'sample test 5',
        'input': 'i a\nf b\ni c\ni d\n\na = 1\nb = 2.1\nc = 1\nd = 2\n\nc = a + d\nb = 3.3\n\np c\n',
        'result': 'success'
    },
    {
        'description': 'variable name with multiple characters',
        'input': 'i xxxabce\nf _xxx_brrrr\ni xxxc_moon\ni xxxdemonslayer\n\nxxxabce = 1\n_xxx_brrrr = 2.1\nxxxc_moon = 1\nxxxdemonslayer = 2\n\nxxxc_moon = xxxabce + xxxdemonslayer\n_xxx_brrrr = 3.3\n\np xxxc_moon\n',
        'result': 'success'
    }
]

def print_test_case(test_case):
    print('>>>>>>>>>>>>>>>>>>>>')
    print(test_case['description'])
    print('---------------------')
    print(test_case['input'])
    print('<<<<<<<<<<<<<<<<<<<<')

def main():
    subprocess.run(['make'], cwd='./src/')
    ok = 0
    for test_case in test_cases:
        with open('.tmp_input.ac', 'w') as f:
            f.write(test_case['input'])
        proc = subprocess.run(['./src/AcDc', '.tmp_input.ac', '.tmp_output.dc'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        test_passed = (test_case['result'] == 'success' and proc.returncode == 0) or (test_case['result'] == 'error' and proc.returncode != 0)
        if not test_passed:
            print_test_case(test_case)
        else:
            ok += 1

    print('======= SUMMARY ========')
    print(f'test cases: {ok}/{len(test_cases)} OK')
    print('========================')

    os.remove('.tmp_input.ac')
    os.remove('.tmp_output.dc')
    subprocess.run(['make', 'clean'], cwd='./src/')
    assert(ok == len(test_cases))

if __name__ == '__main__':
    main()
