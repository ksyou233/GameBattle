import pathlib

tot = cmt = blank = 0
files = list(pathlib.Path('include').rglob('*.hpp')) + \
        list(pathlib.Path('src').rglob('*.cpp')) + [pathlib.Path('main.cpp')]
for p in files:
    for line in p.read_text(encoding='utf-8').splitlines():
        s = line.strip()
        tot += 1
        if not s:
            blank += 1
            continue
        if s.startswith('//') or s.startswith('/*') or s.startswith('*'):
            cmt += 1

code = tot - cmt - blank
print('files      =', len(files))
print('total      =', tot)
print('comment    =', cmt)
print('blank      =', blank)
print('code       =', code)
print('cmt/code   = {:.1f}%'.format(cmt / code * 100))
print('cmt/total  = {:.1f}%'.format(cmt / tot * 100))
