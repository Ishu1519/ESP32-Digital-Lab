with open('data/index.html', 'r', encoding='utf-8') as f:
    html = f.read()

with open('include/web_assets.h', 'w', encoding='utf-8') as f:
    f.write('#pragma once\n#include <pgmspace.h>\n\n')
    f.write('const char INDEX_HTML[] PROGMEM = R"rawliteral(\n')
    f.write(html)
    f.write('\n)rawliteral";\n')

print('Packed index.html into web_assets.h successfully')
