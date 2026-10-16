"""Serve only the bundled postcard editor on loopback; no device or filesystem API."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import webbrowser

ASSETS = {'/monitor.js':('monitor.js','text/javascript; charset=utf-8'),'/font-coverage.json':('font-coverage.json','application/json; charset=utf-8'),**{f'/page-{i}.png':(f'page-{i}.png','image/png') for i in range(4)},'/': ('index.html', 'text/html; charset=utf-8'),
          '/editor.js': ('editor.js', 'text/javascript; charset=utf-8'),
          '/codec.js': ('codec.js', 'text/javascript; charset=utf-8'),
          '/style.css': ('style.css', 'text/css; charset=utf-8'),
          '/defaults.js': ('defaults.js', 'text/javascript; charset=utf-8'),
          '/default-avatar.png': ('default-avatar.png','image/png'),
          '/default-qr.png': ('default-qr.png','image/png')}

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        entry = ASSETS.get(self.path)
        if not entry:
            self.send_error(404)
            return
        data = (Path(__file__).parent / 'postcard' / entry[0]).read_bytes()
        self.send_response(200)
        self.send_header('Content-Type', entry[1])
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Content-Security-Policy', "default-src 'self'; img-src 'self' blob: data:; style-src 'self'; script-src 'self'; connect-src 'self'; frame-ancestors 'none'")
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *args):
        pass

def serve(open_browser=True):
    with ThreadingHTTPServer(('127.0.0.1', 0), Handler) as server:
        url = f'http://127.0.0.1:{server.server_port}/'
        print('Postcard editor: ' + url, flush=True)
        if open_browser:
            webbrowser.open(url)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass

if __name__ == '__main__':
    serve()
