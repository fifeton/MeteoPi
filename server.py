import SimpleHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import json
import datetime


class FifeHandler(SimpleHTTPRequestHandler):
	def __init__(self,request, client_address, server):
		SimpleHTTPRequestHandler.__init__(self,request, client_address, server)
		
		
		
	def do_POST(self):
		print("post")
		
		self.data_string = self.rfile.read(int(self.headers['Content-Length']))

		self.send_response(200)
		self.end_headers()

		data = json.loads(self.data_string)
		
		dd = datetime.datetime.now()
		
		f=open("meteopi."+str(dd.year)+"."+str(dd.month)+"."+str(dd.day)+".data","a")
		
		f.write(json.dumps(data))
		f.write("\n")
		f.flush()
		f.close()

		print(data)




server = HTTPServer(("", 8001), FifeHandler)

server.serve_forever()