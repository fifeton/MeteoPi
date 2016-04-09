
import RPi.GPIO as GPIO
import dht11
import time
import datetime
import Adafruit_BMP.BMP085 as BMP085

import json
import urllib2

class MeteoPi:
	def __init__(self):
		print("Inicializando...")
		self.PINS={'DHT':4}

		self.DATA={'temperatura':0,'humedad':0,'presion':0}

		# initialize GPIO
		GPIO.setwarnings(False)
		GPIO.setmode(GPIO.BCM)
		GPIO.cleanup()
	
		self.TMP = dht11.DHT11(pin = self.PINS['DHT'])
		self.BAR = BMP085.BMP085()
		
		print("Inicializado")


	def update(self):
		# read data using pin 14
		result = self.TMP.read()
		if result.is_valid():
			self.DATA['temperatura']=result.temperature
			self.DATA['humedad']=result.humidity
			
		
		self.DATA['presion']=self.BAR.read_pressure()
		self.DATA['altitud']=self.BAR.read_altitude()
		self.DATA['presion_mar']=self.BAR.read_sealevel_pressure()
		self.DATA['fecha']=str(datetime.datetime.now())


	def postData(self):
		req = urllib2.Request('http://127.0.0.1:8001/meteopi/update.html')
		req.add_header('Content-Type', 'application/json')

		jsd=json.dumps(self.DATA)
		
		print("Enviando "+jsd)
		
		response = urllib2.urlopen(req, jsd)
		if response.getcode()==200:
			print("OK"+response.read())
		else:
			print("NOK: "+str(response.getcode()))

data = {
        'ids': [12, 3, 4, 5, 6]
}






mp=MeteoPi()

mp.update()
'''
print("Last valid input: " + str(datetime.datetime.now()))
print("Temperature: %d C" % mp.DATA['temperatura'])
print("Humidity: %d %%" % mp.DATA['humedad'])


print('Pressure = %0.2f Pa' % mp.DATA['presion'])
print('Altitude = %0.2f m' % mp.DATA['altitud'])
print('Sealevel Pressure = %0.2f Pa' % mp.DATA['presion_mar'])
print(" ")
print("=====================================")
'''
mp.postData()
