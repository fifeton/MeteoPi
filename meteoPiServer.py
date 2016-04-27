#!/usr/bin/env python

import web
import os
import json
import datetime
import sys



class MeteoPiApp(web.application):
    def run(self, port=8001, *middleware):
        func = self.wsgifunc(*middleware)
        return web.httpserver.runsimple(func, ('0.0.0.0', port))

urls=(
    '/', 'index',
    '/meteopi/update',"update",
    '/meteopi/.+',"meteopi"
)

def getData(date=None):
    
    fn=getDataFile()
    
    res={"labels":[],"data":{"temperatura":[],"humedad":[],"presion":[]}}
    
    if os.path.exists(fn):
        f=open(fn,"r")
        
        try:
            for line in f:
                if len(line.strip())>0:
                    row = json.loads(line.strip())
                    res["labels"].append(row["fecha"][11:-10])
                    res["data"]["temperatura"].append(row["temperatura"])
                    res["data"]["humedad"].append(row["humedad"])
                    res["data"]["presion"].append(row["presion"])
        finally:    
            f.close()
    
    return res

class meteopi:
    def GET(self):
        
        render = web.template.frender('templates'+web.ctx.path.replace('/meteopi',''))
        return render(json.dumps(getData()))
        


class index:
    def GET(self):
        
        render = web.template.render('templates')
        return render.temp(json.dumps(getData()))

class update:
    def GET(self):
        user_data = web.input()		
    
        data = {"temperatura":user_data.temp,"humedad":user_data.hum,"presion":user_data.pres,"fecha":str(datetime.datetime.now())}

        fn=getDataFile()
        
        f=open(fn,"a")
        try:
            f.write(json.dumps(data))
            f.write("\n")
            f.flush()
        finally:
            f.close()
            
        return "OK"

def getDataFile():
    dd = datetime.datetime.now()
        
    dn = "logs"
    if not os.path.exists(dn):
        os.mkdir(dn)
    
    fn= dn+"/meteopi."+str(dd.year)+"."+str(dd.month)+"."+str(dd.day)+".data"
    
    return fn
    
if __name__ == "__main__":
    web.config.debug=False
    
    f = open('meteopi.out', 'w')
    sys.stdout = f
    
    f = open('meteopi.err', 'w')
    sys.stderr = f
    
    app = MeteoPiApp(urls, globals())
    app.run()