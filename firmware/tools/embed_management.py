"""Embed the actual management site, with deterministic gzip and no credentials."""
from pathlib import Path
import gzip,sys
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'companion'))
from postcard_server import ASSETS

def render():
    parts=['/* Generated management assets; never contains device credentials. */\n#include "network_http.h"\n']
    entries=[]
    for i,(name,mime) in enumerate(sorted(set(ASSETS.values()))):
        data=(ROOT/'companion/postcard'/name).read_bytes();uri='/' if name=='index.html' else '/'+name
        if name=='index.html':data=data.replace(b'</head>',b'<meta name="passport-device" content="1"><meta name="passport-token" content="@PASSPORT_TOKEN@"></head>')
        packed=gzip.compress(data,compresslevel=9,mtime=0)
        compressed=name!='index.html' and len(packed)+32<len(data)
        if compressed:data=packed
        parts.append('static const uint8_t asset_%d[]={\n'%i)
        parts.extend(','.join(str(b) for b in data[p:p+64])+',\n' for p in range(0,len(data),64))
        # The index is a bounded C string for replacing its one token marker.
        parts.append('0};\n')
        entries.append('{"%s","%s",asset_%d,%d,%s}'%(uri,mime,i,len(data),'true' if compressed else 'false'))
    parts.append('const network_asset_t network_assets[]={'+','.join(entries)+'};\nconst size_t network_assets_count=sizeof(network_assets)/sizeof(*network_assets);\n')
    return ''.join(parts)
if __name__=='__main__':
    dest=Path(sys.argv[1]);dest.parent.mkdir(parents=True,exist_ok=True);text=render()
    if not dest.exists() or dest.read_text(encoding='utf-8')!=text:dest.write_text(text,encoding='utf-8')
