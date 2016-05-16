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
    '/meteopi/history',"history",
    '/meteopi/.+',"meteopi"
)

def getData(fName=None):
    
    fn=getDataFile(fName)
    
    res={"labels":[],"data":{"temperatura":[],"humedad":[],"presion":[],"lluvia":[],"bateria":[]}}
    
    if os.path.exists(fn):
        f=open(fn,"r")
        
        try:
            for line in f:
                if len(line.strip())>0:
                    row = json.loads(line.strip())
                    res["labels"].append(row["fecha"][11:-10])
                    
                    for key in row:
                        if key!="fecha":
                            res["data"][key].append(row[key])
        finally:    
            f.close()
    
    return res

class history:
    def GET(self):
        render = web.template.frender('templates/history.html')
        
        return render(sorted(os.listdir(getLogDir()),reverse=True))
    
class meteopi:
    def GET(self):
        user_data = web.input()
        render = web.template.frender('templates'+web.ctx.path.replace('/meteopi',''))
        
        fName=None
        if user_data:
            fName=user_data['file']
        
        return render(json.dumps(getData(fName)))
                      
class index:
    def GET(self):
        render = web.template.render('templates')
        return render.temp(json.dumps(getData()))

class update:
    def GET(self):
        user_data = web.input()	
    
    
        fecha=user_data.get("date")
        if not fecha:
            fecha=str(datetime.datetime.now())
    
    
            
        data = {"temperatura":user_data.get("temp"),"humedad":user_data.get("hum"),"presion":user_data.get("pres"),"fecha":fecha,"lluvia":user_data.get("rain"),"bateria":user_data.get("bateria")}

        fn=getDataFile()
        
        f=open(fn,"a")
        try:
            f.write(json.dumps(data))
            f.write("\n")
            f.flush()
        finally:
            f.close()
            
        return "OK"

def getLogDir():
    dn= "logs"
    
    if not os.path.exists(dn):
        os.mkdir(dn)
        
    return dn

def getDataFile(fName=None):
    
    dn = getLogDir()
    
        
    if not fName:
        date=datetime.datetime.now()
        
        
        fn= dn+"/meteopi."+str(date.year)+"."+str(date.month)+"."+str(date.day)+".data" 
    else:
        fn= dn+"/"+fName
    
    
    
    return fn
    
if __name__ == "__main__":
    web.config.debug=True
    
    f = open('meteopi.out', 'w')
    sys.stdout = f
    
    f = open('meteopi.err', 'w')
    sys.stderr = f
    
    
    
    
    app = MeteoPiApp(urls, globals())
    app.run()
    
    
    
    