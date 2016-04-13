import SimpleHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import json
import datetime
import os

class FifeHandler(SimpleHTTPRequestHandler):
	def __init__(self,request, client_address, server):
		SimpleHTTPRequestHandler.__init__(self,request, client_address, server)
		
		
		
	def do_POST(self):
		if self.path == '/meteopi/update':
			
			self.data_string = self.rfile.read(int(self.headers['Content-Length']))

			self.send_response(200)
			self.end_headers()

			data = json.loads(self.data_string)
			
			dd = datetime.datetime.now()
			
			dn = "logs"
			if not os.path.exists(dn):
				os.mkdir(dn)
				
			fn = dn+"/meteopi."+str(dd.year)+"."+str(dd.month)+"."+str(dd.day)+".data"
			f=open(fn,"a")
			
			f.write(json.dumps(data))
			f.write("\n")
			f.flush()
			f.close()
		else:
			self.send_response(404)
			self.end_headers()

		


server = HTTPServer(("", 8001), FifeHandler)

server.serve_forever()
