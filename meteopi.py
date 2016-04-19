#!/usr/bin/env python

import datetime
import json
import sys
import time
import urllib2

import Adafruit_BMP.BMP085 as BMP085
import RPi.GPIO as GPIO
import dht11
import lcd_i2c


class MeteoPi:
	def __init__(self):
		print("Inicializando...")
		self.PINS={'DHT':26}

		self.DATA={'temperatura':0,'humedad':0,'presion':0}

		
	
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


	def postData(self):
		req = urllib2.Request('http://192.168.0.201:8001/meteopi/update')
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


try:
	GPIO.cleanup()
	# initialize GPIO
	GPIO.setwarnings(False)
	GPIO.setmode(GPIO.BCM)
	GPIO.setup(13,GPIO.OUT)
	GPIO.output(13,True)
	
	mp=MeteoPi()
	
	mp.update()
	mp.postData()
	
	now = datetime.datetime.now()
	
	lcd_i2c.lcd_init()
	lcd_i2c.lcd_string(str(mp.DATA["temperatura"])+"C "+str(mp.DATA["presion"])+" mb", lcd_i2c.LCD_LINE_2)
	lcd_i2c.lcd_string(str(mp.DATA["humedad"])+"% "+now.strftime("%H:%M"), lcd_i2c.LCD_LINE_1)

finally:
	GPIO.output(13,False)
	