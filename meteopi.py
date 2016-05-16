#!/usr/bin/env python

import datetime
import urllib2

import Adafruit_BMP.BMP085 as BMP085
import RPi.GPIO as GPIO
import dht11
import urllib
import os


class MeteoPi:
	def __init__(self):
		print("Inicializando...")
		self.PINS={'DHT':25,"RAIN":24}

		self.DATA={'temperatura':0,'humedad':0,'presion':0,'rain':0}

		GPIO.setup(self.PINS['RAIN'],GPIO.IN)
		
	
		self.TMP = dht11.DHT11(pin = self.PINS['DHT'])
		try:
			self.BAR = BMP085.BMP085()
		except:
			print("Error inicializando BMP085")
			
			self.BAR=None

		print("Inicializado")


	def update(self):
		# read data using pin 14
		result = self.TMP.read()
		if result.is_valid():
			self.DATA['temperatura']=result.temperature
			self.DATA['humedad']=result.humidity
			
		if self.BAR:
			self.DATA['presion']=self.BAR.read_pressure()
			self.DATA['altitud']=self.BAR.read_altitude()
			self.DATA['presion_mar']=self.BAR.read_sealevel_pressure()
			self.DATA['temperatura']=self.BAR.read_temperature()
		self.DATA['fecha']=str(datetime.datetime.now())

		self.DATA['rain']=GPIO.input(self.PINS['RAIN'])
		

	def postData(self):
		url='http://192.168.0.201:8001/meteopi/update?'
		url=url+"temp="+str(self.DATA["temperatura"])+"&hum="+str(self.DATA["humedad"])+"&pres="+str(self.DATA['presion'])+"&rain=0"
		url=url+"&date="+urllib.quote(self.DATA['fecha'])
		req = urllib2.Request(url)
		#req.add_header('Content-Type', 'application/json')

		#jsd=json.dumps(self.DATA)
		
		print("Enviando "+url)
		
		response = urllib2.urlopen(req)
		if response.getcode()==200:
			print("OK"+response.read())
		else:
			print("NOK: "+str(response.getcode()))

data = {
        'ids': [12, 3, 4, 5, 6]
}


try:
	
	GPIO.cleanup()
	# initialize GPIO
	GPIO.setwarnings(False)
	GPIO.setmode(GPIO.BCM)
	GPIO.setup(13,GPIO.OUT)
	GPIO.output(13,True)
	GPIO.setup(23,GPIO.IN)
	
	if not GPIO.input(23):
		print("Bateria baja")
	else:	
		print("Bateria OK")
		mp=MeteoPi()
		
		mp.update()
		
		print("habilitando wifi...")
		cmd = 'sudo ifconfig wlan0 up'
		print(str(os.system(cmd)))
		
		print("deshabilitando eth0...")
		cmd = 'sudo ifconfig eth0 down'
		print(str(os.system(cmd)))
		
		mp.postData()
		
		now = datetime.datetime.now()
		'''
		lcd_i2c.lcd_init()
		lcd_i2c.lcd_string(str(mp.DATA["temperatura"])+"C "+str(mp.DATA["presion"])+" mb", lcd_i2c.LCD_LINE_2)
		lcd_i2c.lcd_string(str(mp.DATA["humedad"])+"% "+now.strftime("%H:%M"), lcd_i2c.LCD_LINE_1)
		'''
finally:
	GPIO.output(13,False)
	GPIO.cleanup()
	cmd = 'sudo ifconfig wlan0 down'
	os.system(cmd)
	