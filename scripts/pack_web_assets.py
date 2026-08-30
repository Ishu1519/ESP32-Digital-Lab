import os

def main():
    html_path = os.path.join("data", "index.html")
    header_path = os.path.join("include", "web_assets.h")

    with open(html_path, "r", encoding="utf-8") as f:
        html_content = f.read()

    header_content = '#pragma once\n#include <pgmspace.h>\n\nconst char INDEX_HTML[] PROGMEM = R"rawliteral(\n'
    header_content += html_content
    header_content += '\n)rawliteral";\n'

    with open(header_path, "w", encoding="utf-8") as f:
        f.write(header_content)

    print(f"Successfully packed {html_path} ({len(html_content)} bytes) into {header_path}")

if __name__ == "__main__":
    main()
