import web
import os
import json
import datetime

urls = (
    '/', 'index',
    '/meteopi/update',"update",
    '/meteopi/.+',"meteopi"
)

    
def getData(date=None):
    
    fn=getDataFile()
    
    res={"labels":[],"data":{"temperatura":[],"humedad":[]}}
    
    f=open(fn,"r")
    try:
        for line in f:
            print line
            if len(line.strip())>0:
                row = json.loads(line.strip())
                res["labels"].append(row["fecha"])
                res["data"]["temperatura"].append(row["temperatura"])
                res["data"]["humedad"].append(row["humedad"])
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
        return render.temp(json.dumps(getData("temperatura")))

class update:
    def POST(self):
        data = web.data()
    
        data = json.loads(data.decode())
                
        fn=getDataFile()
        
        f=open(fn,"a")
        try:
            f.write(json.dumps(data))
            f.write("\n")
            f.flush()
        finally:
            f.close()
            
            
def getDataFile():
    dd = datetime.datetime.now()
        
    dn = "logs"
    if not os.path.exists(dn):
        os.mkdir(dn)
        
    return dn+"/meteopi."+str(dd.year)+"."+str(dd.month)+"."+str(dd.day)+".data"
    
if __name__ == "__main__":
    app = web.application(urls, globals(),port=8001)
    app.run()