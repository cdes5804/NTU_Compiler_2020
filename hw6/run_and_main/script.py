import subprocess as sp
import os
from random import randint
import re
from termcolor import colored

test_data_directory = '../testdata/'
source_directory = '../src/'
skip_case = ['long-long-jump.c']

def random_input():
    return '\n'.join(str(randint(-100, 100)) for _ in range(100))

def compile(file_name):
    script_path = './run.sh'
    parser_path = os.path.join(source_directory, 'parser')
    test_path = os.path.join(test_data_directory, file_name)
    # Compile
    p = sp.Popen([script_path, parser_path, test_path], stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.STDOUT)
    output = p.communicate()[0].decode('ascii')[: 200]
    if len(output):
        print(output)
        return False
    else:
        print(colored('successfully compiled', 'green'))
        return True

def run(file_name):
    test_path = os.path.join(test_data_directory, file_name)
    p = sp.Popen(['qemu-riscv64', './a.out'], stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.STDOUT)
    i = bytearray(random_input(), 'ascii')
    try:
        output = p.communicate(input=i, timeout=3)[0].decode('ascii')
        output = re.sub('-nan', 'nan', output)
    except:
        print(colored('Time Limit Exceeded', 'blue'))
        return False

    # Run Real Answer
    try:
        f = open('tmp.c', 'w')
        sp.call(['cat', 'lib.c'], stdout=f)
        sp.call(['cat', test_path], stdout=f)
        sp.call(['g++', 'tmp.c'], stdout=sp.DEVNULL, stderr=sp.DEVNULL)
        f.close()
        sp.call(['rm', 'tmp.c'])
        p = sp.Popen(['./a.out'], stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.STDOUT)
        answer = p.communicate(input=i)[0].decode('ascii')
        answer = re.sub('-nan', 'nan', answer)
    except:
        return False

    if output != answer:
        print(colored('Wrong Answer', 'red'))
        print('Correct answer:')
        print(answer)
        print('Your answer:')
        print(output)
        return False
    else:
        print(colored('Accepted', 'green'))
        return True

def main():
    ok = 0
    try:
        sp.call(['make'], cwd=source_directory, stdout=sp.DEVNULL, stderr=sp.DEVNULL)
    except:
        print(colored('make failed', 'red'))
        return

    for file_name in os.listdir(test_data_directory):
        if file_name in skip_case:
            continue

        print('=========================================================')
        print(f'testcase: {file_name}')
        
        success = compile(file_name)
        if not success:
            continue
        success = run(file_name)

        if success:
            ok += 1

    sp.call(['make', 'clean'], cwd=source_directory, stdout=sp.DEVNULL, stderr=sp.DEVNULL)
    
    print('==========Summary===========')
    print(f'pass/total: {ok}/{len(os.listdir(test_data_directory))}')
    print(f'score: {ok * 2}')

if __name__ == '__main__':
    main()