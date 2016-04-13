import BaseHTTPServer
import SimpleHTTPServer

import json
import datetime
import os


class FifeHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
	def __init__(self,request, client_address, server):
		SimpleHTTPServer.SimpleHTTPRequestHandler.__init__(self,request, client_address, server)
		
	def serve_file(self,p,serverData=None):
		p = "html/"+p
		if os.path.exists(p):
			p=open(p,"r")
			try:
				content=p.read()
				if serverData:
					content=content.replace("SERVER___DATA",str(json.dumps(serverData)))
				self.wfile.write(content.encode())
			finally:
				p.close()
		else:
			self.send_response(404)
			
	def getTempData(date=None):
		
		dd = datetime.datetime.now()
			
		dn = "logs"
			
		fn = dn+"/meteopi."+str(dd.year)+"."+str(dd.month)+"."+str(dd.day)+".data"
		
		print("Leyendo fichero "+fn)
		
		res={"labels":[],"data":[]}
		
		f=open(fn,"r")
		
		for line in f:
			print line
			if len(line.strip())>0:
				row = json.loads(line.strip())
				res["labels"].append(row["fecha"])
				res["data"].append(row["temperatura"])
		
		f.close()
		
		
			
		
		return res
		
	def do_GET(self):
		if self.path.startswith("/meteopi/temp"):
			self.serve_file("/temp.html",self.getTempData())
		else:
			self.serve_file(self.path)
		
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

		


server = BaseHTTPServer.HTTPServer(("", 8001), FifeHandler)

server.serve_forever()
