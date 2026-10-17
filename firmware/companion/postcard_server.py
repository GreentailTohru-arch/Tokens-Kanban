"""Serve only the bundled postcard editor on loopback; no device or filesystem API."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import webbrowser
from urllib.parse import urlsplit

ASSETS = {'/layout.css':('layout.css','text/css; charset=utf-8'),'/brand-light.svg':('brand-light.svg','image/svg+xml'),'/brand-dark.svg':('brand-dark.svg','image/svg+xml'),'/theme.js':('theme.js','text/javascript; charset=utf-8'),'/themes.css':('themes.css','text/css; charset=utf-8'),'/passport-front.png':('passport-front.png','image/png'),'/atelier.css':('atelier.css','text/css; charset=utf-8'),'/notices.js':('notices.js','text/javascript; charset=utf-8'),'/gif-frames.js':('gif-frames.js','text/javascript; charset=utf-8'),'/vendor/omggif.js':('vendor/omggif.js','text/javascript; charset=utf-8'),'/vendor/omggif-LICENSE.txt':('vendor/omggif-LICENSE.txt','text/plain; charset=utf-8'),'/local-connection.js':('local-connection.js','text/javascript; charset=utf-8'),'/draft-state.js':('draft-state.js','text/javascript; charset=utf-8'),**{f'/vendor/{n}':(f'vendor/{n}','text/plain; charset=utf-8') for n in ['pako-LICENSE.txt','pako-zlib-LICENSE.txt','atob-lite-LICENSE.txt']},'/manage.css':('manage.css','text/css; charset=utf-8'),'/about.js':('about.js','text/javascript; charset=utf-8'),'/logo-editor.js':('logo-editor.js','text/javascript; charset=utf-8'),'/vendor/esptool-0.7.0.js':('vendor/esptool-0.7.0.js','text/javascript; charset=utf-8'),'/vendor/esptool-LICENSE.txt':('vendor/esptool-LICENSE.txt','text/plain; charset=utf-8'),'/device-info.js':('device-info.js','text/javascript; charset=utf-8'),'/simulator.js':('simulator.js','text/javascript; charset=utf-8'),'/sim-startup.png':('sim-startup.png','image/png'),**{f'/sim-audio/{n}':(f'sim-audio/{n}','audio/wav') for n in ['codex-notification.wav','codex-error.wav','opencode-complete.wav','opencode-error.wav','antigravity-complete.wav','antigravity-error.wav','startup.wav']},'/monitor.js':('monitor.js','text/javascript; charset=utf-8'),'/font-coverage.json':('font-coverage.json','application/json; charset=utf-8'),**{f'/page-{i}.png':(f'page-{i}.png','image/png') for i in range(4)},'/': ('index.html', 'text/html; charset=utf-8'),
          '/editor.js': ('editor.js', 'text/javascript; charset=utf-8'),
          '/codec.js': ('codec.js', 'text/javascript; charset=utf-8'),
          '/style.css': ('style.css', 'text/css; charset=utf-8'),
          '/defaults.js': ('defaults.js', 'text/javascript; charset=utf-8'),
          '/default-avatar.png': ('default-avatar.png','image/png'),
          '/default-qr.png': ('default-qr.png','image/png')}

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        entry = ASSETS.get(urlsplit(self.path).path)
        if not entry:
            self.send_error(404)
            return
        data = (Path(__file__).parent / 'postcard' / entry[0]).read_bytes()
        self.send_response(200)
        self.send_header('Content-Type', entry[1])
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Content-Security-Policy', "default-src 'self'; img-src 'self' blob: data:; style-src 'self'; script-src 'self'; connect-src 'self' https://api.github.com https://raw.githubusercontent.com https://github.com https://release-assets.githubusercontent.com https://objects.githubusercontent.com; frame-ancestors 'none'")
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
