#! /usr/bin/env python

import mechanize

# Create browser
br = mechanize.Browser()

# Want debugging messages?
#br.set_debug_http(True)
#br.set_debug_redirects(True)
#br.set_debug_responses(True)

# User-Agent
br.addheaders = [('User-agent', 'Mozilla/5.0 (Raspberry Pi; Linux  raspbian/3.2.27; en-US) Python-mechanize/0.2.5-py2.7')]

# Open the site
br.open('http://wwwcimis.water.ca.gov/cimis/frontLogonData.do')

# Select the logon form
br.select_form("logonForm")
# User credentials
br.form['username'] = 'D Kimberling'
br.form['password'] = 'getET0'
br.submit()

# Clicking the link to My CIMIS
req = br.click_link(url_regex="myCimis.jsp")
br.open(req)

# Clicking the link to My Reports
req = br.click_link(url_regex="frontMyReport.do")
br.open(req)

# Download Daily report
f = br.retrieve('http://wwwcimis.water.ca.gov/cimis/frontQuickReport.do?type=DAY&list=1')[0]

# Log Off
br.follow_link(text='Log Off')

# Open and read downloaded CSV file
fh = open(f)
lines = fh.readlines()
fh.close()

# Locate data of interest (i.e. latest ETo)
hdrs = lines[0].split(",") #Split header line into a list
data = lines[-1].split(",") #Split last data line into a list
EToIdx = hdrs.index("CIMIS ETo (mm)") #Find index of ETo in headers
print "The latest ETo is: ", data[EToIdx]

