import re

filepath = r'C:\Users\roman\.claude\projects\c--Users-roman-Dropbox-Engine\7d8f9e0e-3769-4890-aa41-862f29beced7\tool-results\toolu_01YL52pZkUEVnwT4zgUZ9YUG.txt'
with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
    text = f.read()

# Match lines with error/warning pattern: filepath(line): error/warning
# Extract just the relative source path and line number
pattern = re.compile(r'c:\Users\roman\Dropbox\Engine\(source[^(]+)\((\d+)\)')
matches = pattern.findall(text)
results = set()
for path, line in matches:
    path = path.replace('\', '/')
    results.add(f'{path}:{line}')

print(f'Found {len(results)} unique file:line locations:')
print()
for r in sorted(results):
    print(r)
