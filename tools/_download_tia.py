#!/usr/bin/env python3
"""Bulk-download every file in the TIA P25 Library to ./TIA/<ident>/<filename>."""
import json, os, sys, re, time, urllib.request, urllib.error

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(ROOT, 'TIA')
RAW = os.path.join(ROOT, 'tia_library_raw.json')
LOG = os.path.join(ROOT, '_download_tia.log')

def safe(name):
    name = re.sub(r'[\\/:*?"<>|]', '_', name).strip()
    return name[:200] or 'file'

def log(msg):
    with open(LOG, 'a') as f:
        f.write(msg + '\n')

docs = json.load(open(RAW))['data']['documents']
parsed = {d['title']: d['ident'] for d in json.load(open(os.path.join(ROOT,'tia_library_parsed.json')))['library']}

# Clear log
open(LOG, 'w').close()

total_files = sum(len(d.get('files') or []) for d in docs)
log(f"Starting bulk download: {len(docs)} entries, {total_files} files")

done = 0
skipped = 0
failed = 0
for d in docs:
    ident = parsed.get(d['title'].strip(), 'UNPARSED')
    folder = os.path.join(OUT, safe(ident))
    os.makedirs(folder, exist_ok=True)
    for fi in (d.get('files') or []):
        fname = safe(fi.get('fileName') or fi.get('title') or 'file')
        if not fname.lower().endswith('.' + (fi.get('extension') or '').lower()) and fi.get('extension'):
            fname = f"{fname}.{fi['extension']}"
        dest = os.path.join(folder, fname)
        url = fi.get('downloadUrl')
        if not url:
            log(f"NO URL: {ident} / {fname}")
            failed += 1
            continue
        if os.path.exists(dest) and os.path.getsize(dest) > 0:
            skipped += 1
            log(f"SKIP: {ident}/{fname}")
            continue
        try:
            req = urllib.request.Request(url, headers={
                'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36',
                'Referer': 'https://connect.tiaonline.org/communities/community-home/librarydocuments?LibraryKey=4b0bb9c9-ac3c-4969-9425-01904a786047',
            })
            with urllib.request.urlopen(req, timeout=120) as r:
                data = r.read()
            with open(dest, 'wb') as f:
                f.write(data)
            done += 1
            log(f"OK  {len(data):>10} {ident}/{fname}")
        except Exception as e:
            failed += 1
            log(f"ERR {ident}/{fname}: {e}")
        time.sleep(0.25)  # polite throttle

log(f"\nDone. ok={done} skipped={skipped} failed={failed} total={total_files}")
print(f"ok={done} skipped={skipped} failed={failed}")
