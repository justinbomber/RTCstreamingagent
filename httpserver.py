from http.server import SimpleHTTPRequestHandler, HTTPServer
import argparse

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', '*')  # Allow all methods
        self.send_header('Access-Control-Allow-Headers', '*')  # Allow all headers
        self.send_header('Access-Control-Max-Age', '86400')  # Cache preflight response for 24 hours
        SimpleHTTPRequestHandler.end_headers(self)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='CORS-enabled HTTP server')
    parser.add_argument('-i', '--ip', default='10.1.1.128', help='IP address to bind to')
    parser.add_argument('-p', '--port', type=int, default=8080, help='Port to listen on')
    
    args = parser.parse_args()
    
    httpd = HTTPServer((args.ip, args.port), CORSRequestHandler)
    print(f"Serving at {args.ip}:{args.port}")
    httpd.serve_forever()
