#!/usr/bin/python

import urllib, re, sys, cgi
from xml.dom import minidom

def getWeather(location, forecast):

  #create google weather api url
  url = "http://www.google.com/ig/api?weather=" + urllib.quote(location)

  try:
    # open google weather api url
    f = urllib.urlopen(url)
  except:
    # if there was an error opening the url, return
    print "err url"
    return

  # read contents to a string
  content_type = dict(f.info())["content-type"]
  charset = re.search('charset\=(.*)',content_type).group(1)
  if not charset:
    charset = 'utf-8'
  if charset.lower() != 'utf-8':
    xml_response = f.read().decode(charset).encode('utf-8')
  else:
    xml_response = f.read()
  f.close()

  # print weather information
  dom = minidom.parseString(xml_response)

  information = dom.getElementsByTagName('forecast_information')[0]
  try:
    city = information.getElementsByTagName('city')[0].getAttribute('data').encode(charset)
  except:
    city = location 

  if forecast == '0':
    current_conditions = dom.getElementsByTagName('current_conditions')[0]
    current_cond = current_conditions.getElementsByTagName('condition')[0].getAttribute('data')
    current_temp = current_conditions.getElementsByTagName('temp_f')[0].getAttribute('data')
    current_humd = current_conditions.getElementsByTagName('humidity')[0].getAttribute('data')
    current_icon = current_conditions.getElementsByTagName('icon')[0].getAttribute('data')
    try:
      current_wind = current_conditions.getElementsByTagName('wind_condition')[0].getAttribute('data')
    except:
      current_wind = 'Wind: N/A'

    page = ""
    page += "City: "+city+"<br />"
    page += "Weather: "+current_cond+"<br />"
    page += "TempF: "+current_temp+"<br />"
    page += current_humd+"<br />"
    page += current_wind+"<br />"
    page += "<img src=\"http://www.google.com"+current_icon+"\" /><br />"
    print page

  else:
    forecast_conditions = dom.getElementsByTagName('forecast_conditions')[int(forecast)-1]
    forecast_cond = forecast_conditions.getElementsByTagName('condition')[0].getAttribute('data')
    forecast_low = forecast_conditions.getElementsByTagName('low')[0].getAttribute('data')
    forecast_high = forecast_conditions.getElementsByTagName('high')[0].getAttribute('data')
    forecast_icon = forecast_conditions.getElementsByTagName('icon')[0].getAttribute('data')

    page = ""
    page += "City: "+city+"<br />"
    page += forecast+" day forecast<br />"
    page += "Weather: "+forecast_cond+"<br />"
    page += "HighF: "+forecast_high+"<br />"
    page += "LowF: "+forecast_low+"<br />"
    page += "<img src=\"http://www.google.com"+forecast_icon+"\" /><br />"
    print page

  return 


def main():
  print "Content-Type: text/html"
  print

  form = cgi.FieldStorage()
  location = form.getfirst('location', 'new york')
  forecast = form.getfirst('forecast', '0')
  try:
    getWeather(location, forecast)
  except:
    print 'err'

if __name__ == "__main__":
    sys.exit(main())

