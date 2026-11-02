#!/usr/bin/env python3
"""
Script konversi Laporan Markdown ke Dokumen PDF Resmi Akademis
Menggunakan python-markdown, KaTeX/MathJax, dan Microsoft Edge headless.
Disesuaikan agar memenuhi standar: MAKSIMAL 15 HALAMAN.
"""

import os
import sys
import subprocess
import markdown
from pypdf import PdfReader

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MD_PATH = os.path.join(BASE_DIR, "report", "laporan.md")
HTML_PATH = os.path.join(BASE_DIR, "report", "laporan.html")
PDF_PATH = os.path.join(BASE_DIR, "report", "laporan.pdf")

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Laporan Komputasi Paralel - Muhamad Iksan Nur Akbar (2222105015)</title>
<!-- KaTeX for formula rendering -->
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/katex@0.16.8/dist/katex.min.css">
<script defer src="https://cdn.jsdelivr.net/npm/katex@0.16.8/dist/katex.min.js"></script>
<script defer src="https://cdn.jsdelivr.net/npm/katex@0.16.8/dist/contrib/auto-render.min.js"
        onload="renderMathInElement(document.body, {
            delimiters: [
                {left: '$$', right: '$$', display: true},
                {left: '$', right: '$', display: false}
            ],
            throwOnError: false
        });"></script>

<style>
@page {
    size: A4;
    margin: 1.1cm 1.3cm 1.1cm 1.3cm;
    @bottom-center {
        content: counter(page);
    }
}

body {
    font-family: 'Times New Roman', Times, serif;
    font-size: 8.8pt;
    line-height: 1.30;
    color: #111;
    background: #fff;
    margin: 0;
    padding: 0;
}

h1, h2, h3, h4 {
    font-family: 'Segoe UI', Arial, sans-serif;
    color: #0d233a;
    page-break-after: avoid;
    margin-top: 0.8em;
    margin-bottom: 0.2em;
}

h1 {
    font-size: 12pt;
    border-bottom: 1.2px solid #0d233a;
    padding-bottom: 2px;
    margin-top: 1.1em;
}

/* Judul Utama */
body > h1:first-of-type {
    text-align: center;
    border-bottom: none;
    margin-top: 0.2em;
    font-size: 13.5pt;
    line-height: 1.2;
}

body > h2:first-of-type {
    text-align: center;
    font-size: 10pt;
    color: #222;
    font-weight: 600;
    margin-top: 0.1em;
    margin-bottom: 0.6em;
    line-height: 1.2;
}

h2 { font-size: 10.5pt; }
h3 { font-size: 9.5pt; }
h4 { font-size: 8.8pt; font-style: italic; }

p {
    text-align: justify;
    text-justify: inter-word;
    margin-top: 0.15em;
    margin-bottom: 0.3em;
}

ul, ol {
    margin-top: 0.1em;
    margin-bottom: 0.3em;
    padding-left: 1.2em;
}

li {
    margin-bottom: 0.12em;
    text-align: justify;
}

table {
    width: 100%;
    border-collapse: collapse;
    margin: 0.35em 0;
    font-size: 7.6pt;
    page-break-inside: avoid;
}

th, td {
    border: 1px solid #aaa;
    padding: 2.2px 4.5px;
    text-align: left;
}

th {
    background-color: #eef3f8;
    color: #0d233a;
    font-weight: bold;
    text-align: center;
}

tr:nth-child(even) td {
    background-color: #fafbfc;
}

img {
    max-width: 58%;
    height: auto;
    display: block;
    margin: 0.35em auto 0.1em auto;
    page-break-inside: avoid;
}

em {
    font-size: 7.6pt;
    display: block;
    text-align: center;
    color: #444;
    margin-bottom: 0.35em;
}

blockquote {
    border-left: 3px solid #2b5797;
    background-color: #f4f7fb;
    margin: 0.3em 0;
    padding: 3px 8px;
    font-size: 8.2pt;
}

code {
    font-family: 'Consolas', 'Courier New', monospace;
    font-size: 7.8pt;
    background: #f1f3f5;
    padding: 1px 3px;
    border-radius: 2px;
    color: #b02a37;
}

pre {
    background: #f8f9fa;
    border: 1px solid #e9ecef;
    border-left: 3px solid #0d233a;
    padding: 3px 6px;
    font-family: 'Consolas', 'Courier New', monospace;
    font-size: 7.5pt;
    line-height: 1.18;
    overflow-x: hidden;
    white-space: pre-wrap;
    page-break-inside: avoid;
    margin: 0.25em 0;
}

pre code {
    background: none;
    padding: 0;
    color: #212529;
}

/* Page break rules */
#bab-i-pendahuluan { page-break-before: avoid; }
#bab-ii-desain-implementasi-dan-arsitektur-paralel { page-break-before: always; }
#bab-iii-hasil-pengujian-dan-benchmark { page-break-before: always; }
#bab-iv-analisis-kritis-dan-pembahasan { page-break-before: always; }
#bab-v-kesimpulan-dan-rekomendasi { page-break-before: always; }
#daftar-pustaka { page-break-before: avoid; }

</style>
</head>
<body>
{{HTML_BODY}}
</body>
</html>
"""

def main():
    print(f"[1/4] Membaca berkas {MD_PATH}...")
    with open(MD_PATH, "r", encoding="utf-8") as f:
        md_content = f.read()
    
    print("[2/4] Mengonversi Markdown ke HTML akademis...")
    extensions = [
        'extra',
        'tables',
        'fenced_code',
        'codehilite',
        'toc',
        'nl2br'
    ]
    html_body = markdown.markdown(md_content, extensions=extensions)
    html_doc = HTML_TEMPLATE.replace("{{HTML_BODY}}", html_body)
    
    with open(HTML_PATH, "w", encoding="utf-8") as f:
        f.write(html_doc)
    print(f"      -> Disimpan di {HTML_PATH}")
    
    print("[3/4] Mengonversi HTML ke PDF dengan Microsoft Edge headless...")
    edge_paths = [
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"
    ]
    edge_exe = None
    for p in edge_paths:
        if os.path.exists(p):
            edge_exe = p
            break
            
    if not edge_exe:
        print("Error: Microsoft Edge executable tidak ditemukan.")
        sys.exit(1)
        
    cmd = [
        edge_exe,
        "--headless",
        "--disable-gpu",
        "--no-pdf-header-footer",
        f"--print-to-pdf={PDF_PATH}",
        f"file:///{HTML_PATH.replace(os.sep, '/')}"
    ]
    
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error menjalankan Edge: {res.stderr}")
        sys.exit(1)
        
    print(f"      -> PDF berhasil dibuat: {PDF_PATH}")
    
    print("[4/4] Memverifikasi jumlah halaman PDF...")
    reader = PdfReader(PDF_PATH)
    total_pages = len(reader.pages)
    print(f"      -> TOTAL HALAMAN: {total_pages} Halaman")
    
    if total_pages <= 15:
        print(f"      -> [PASS] Sesuai ketentuan: Maksimal 15 halaman ({total_pages} <= 15).")
    else:
        print(f"      -> [WARNING] Melebihi batas 15 halaman ({total_pages} > 15)!")

if __name__ == "__main__":
    main()
