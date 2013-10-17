#!/usr/bin/python
"""Updated 24/August/2013."""

import re, os, json, time, base64, thread # standard Python modules
import web # the Web.py module. See webpy.org (Enables the OpenSprinkler web interface)
import gv # 'global vars' An empty module, used for storing vars (as attributes), that need to be 'global' across threads.
import RPi.GPIO as GPIO # Required for accessing General Purpose Input Output pins on Raspberry Pi

  #### Import ospi_addon module (ospi_addon.py) if it exists. ####
try:
    import ospi_addon #This provides a stub for adding custom features to ospi.py as external modules.
except ImportError:
    pass

 #### urls is a feature of web.py. When a GET request is recieved, the corresponding class is exicuted.
urls = [
    '/',  'home',
    '/cv', 'change_values',
    '/vo', 'view_options',
    '/co', 'change_options',
    '/vs', 'view_stations',
    '/cs', 'change_stations',
    '/sn(\d+?\Z)', 'get_station', # regular expression, accepts any station number
    '/sn(\d+?=\d(&t=\d+?\Z)?)', 'set_station', # regular expression, accepts any digits
    '/vr', 'view_runonce',
    '/cr', 'change_runonce',
    '/vp', 'view_programs',
    '/mp', 'modify_program',
    '/cp', 'change_program',
    '/dp', 'delete_program',
    '/gp', 'graph_programs',
    '/vl', 'view_log',
    '/cl', 'clear_log',
    '/lo', 'log_options',
    ]

  #### Function Definitions ####

def baseurl():
    """Return URL app is running under.""" 
    baseurl = web.ctx['home']
    return baseurl

def board_rev():
    """Auto-detect the Raspberry Pi board rev."""
    revision = "unknown"
    with open('/proc/cmdline', 'r') as f:
        line = f.readline()
    m = re.search('bcm2708.boardrev=(0x[0123456789abcdef]*) ', line)
    revision = m.group(1)
    revcode = int(revision, 16)
    if revcode <= 3:
        rev = 1
    else:
        rev = 2   
    return rev

def clear_mm():
    """Clear manual mode settings."""
    if gv.sd['mm']:
        gv.sbits = [0] * (gv.sd['nbrd'] +1)
        gv.ps = []
        for i in range(gv.sd['nst']):
            gv.ps.append([0,0])
        gv.rs = []
        for i in range(gv.sd['nst']):
            gv.rs.append([0,0,0,0])    
        gv.srvals = [0]*(gv.sd['nst'])
        set_output()
    return

def log_run(datetime):
    """add run data to csv file - most recent first."""
    if gv.lg:
        snames = data('snames')
        zones=re.findall(r"\'(.+?)\'",snames)
        if gv.lrun[1] == 98:
            pgr = 'Run-once'
        elif gv.lrun[1] == 99:
            pgr = 'Manual'
        else:
            pgr = str(gv.lrun[1])
        datastr = (pgr +', '+str(zones[gv.lrun[0]])+', '+str(gv.lrun[2]/60)+'m'+str(gv.lrun[2]%60)+
                   's, '+time.strftime("%H:%M:%S, %a. %d %b %Y", time.localtime(datetime))+'\n')
        f = open('./static/log/water_log.csv', 'r')
        log = f.readlines()
        f.close()
        log.insert(1, datastr)
        f = open('./static/log/water_log.csv', 'w') 
        if gv.lr:
            f.writelines(log[:gv.lr+1])
        else:
            f.writelines(log)
        f.close  
    return
    

def prog_match(now, prog):
    """Test a program for current date and time match."""
    if not prog[0]: return 0 # Skip if program is not enabled
    devday = int(now/86400) # Check day match
    lt = time.localtime(now)
    if (prog[1]>=128) and (prog[2]>1): #Inverval program
        if (devday %prog[2]) != (prog[1] - 128): return 0
    else: # Weekday program
        if not prog[1]-128 & 1<<lt[6]: return 0
        if prog[1]>=128 and prog[2] == 0: #even days
            if lt[2]%2 != 0: return 0
        if prog[1]>=128 and prog[2] == 1: #Odd days
            if lt[2]==31 or (lt[1]==2 and lt[2]==29): return 0
            elif lt[2]%2 !=1: return 0   
    this_minute = (lt[3]*60)+lt[4] # Check time match
    if this_minute < prog[3] or this_minute > prog[4]: return 0
    if prog[5] == 0: return 0
    if ((this_minute - prog[3]) / prog[5]) * prog[5] == this_minute - prog[3]:
        return 1 # Program matched
    return 0

def schedule_stations(curr_time):
    """Schedule stattions/valves/zones to run."""
    if gv.sd['rd']: # Skip if rain delay
        return
    accumulate_time = curr_time
    if gv.sd['seq']: #sequential mode, stations run one after another
        for sid in range(gv.sd['nst']):
            if gv.rs[sid][2]: # if station has a duration value
                gv.rs[sid][0] = accumulate_time # start at accumulated time
                accumulate_time += gv.rs[sid][2] # add duration
                gv.rs[sid][1] = accumulate_time # set new stop time
                accumulate_time += gv.sd['sdt'] # add station delay

    else: # concurrent mode, stations allowed to run in parallel
        for sid in range(gv.sd['nst']):
            if gv.rs[sid][2]and not gv.srvals[sid]: # if station has a duration value and is not running
                gv.rs[sid][0] = accumulate_time # set start time
                gv.rs[sid][1] = accumulate_time + gv.rs[sid][2] # Stop time = Start time + duration
    gv.sd['bsy'] = 1
    return

def stop_stations():
        gv.srvals = [0]*(gv.sd['nst'])
        set_output()            
        gv.ps = []
        for i in range(gv.sd['nst']):
            gv.ps.append([0,0])   
        gv.sbits = [0] * (gv.sd['nbrd'] +1)
        gv.rs = []
        for i in range(gv.sd['nst']):
            gv.rs.append([0,0,0,0])
        gv.sd['bsy'] = 0
        return

def main_loop(): # Runs in a seperate thread
    """ ***** Main algorithm.***** """
    print 'Starting main loop \n'
    last_min = 0
    while True: # infinite loop
        match = 0
        now = time.time()
        if gv.sd['en'] and not gv.sd['mm'] and (not gv.sd['bsy'] or not gv.sd['seq']) and not gv.sd['rd']:
            lt = time.localtime(now)
            if (lt[3]*60)+lt[4] != last_min: # only check programs once a minute
                last_min = (lt[3]*60)+lt[4]
                for i, p in enumerate(gv.pd): # get both index and prog item 
                    if prog_match(now, p) and p[0] and p[6]: # check if program time matches now, is active, and has a duration
                        for b in range(gv.sd['nbrd']): # check each station 
                            for s in range(8):
                                sid = b*8+s # station index
                                if sid+1 == gv.sd['mas']: continue # skip if this is master valve
                                if gv.srvals[sid]: continue # skip if currently on
                                
                                if p[7+b]&1<<s: # if this station is scheduled in this program
                                    gv.rs[sid][2] = p[6]*gv.sd['wl']/100 # duration scaled by water level
                                    gv.rs[sid][3] = i+1 # store program number
                                    gv.ps[sid][0] = i+1 # store program number for display
                                    gv.ps[sid][1] = gv.rs[sid][2] # duration
                                    match = True
            if match:
                schedule_stations(now) # turns on gv.sd['bsy']

        if gv.sd['bsy']:
            for b in range(gv.sd['nbrd']): 
                for s in range(8):
                    sid = b*8 + s # station index
                    if gv.srvals[sid]: # if this station is on
                        if now >= gv.rs[sid][1]: # check if time is up
                            gv.srvals[sid] = 0
                            set_output()
                            if gv.sd['mas']-1 != sid: # if not master, fill out log
                                gv.sbits[b] = gv.sbits[b]&~2**s
                                gv.ps[sid] = [0,0]
                                gv.lrun[0] = sid
                                gv.lrun[1] = gv.rs[sid][3]
                                gv.lrun[2] = int(now - gv.rs[sid][0])
                                gv.lrun[3] = now+((gv.sd['tz']/4)-12)*3600
                                log_run(now)
                                gv.pon = None # Program has ended
                            elif gv.sd['mas']-1 == sid:
                                gv.sbits[b] = gv.sbits[b]&~2**s
                            gv.rs[sid] = [0,0,0,0]
                    else: # if this station is not yet on
                        if now >= gv.rs[sid][0] and now < gv.rs[sid][1]:
                            if gv.sd['mas']-1 != sid: # if not master
                                gv.srvals[sid] = 1 # station is turned on
                                set_output()
                                gv.sbits[b] = gv.sbits[b]|2**s # Set display to on
                                gv.ps[sid][0] = gv.rs[sid][3]
                                gv.ps[sid][1] = gv.rs[sid][2]
                                if gv.sd['mas'] and gv.sd['mo'][b]&1<<(s-(s/8)*80): # and not gv.sd['mm'] and gv.sd['seq']: # Master settings
                                    masid = gv.sd['mas'] - 1 # master index
                                    gv.rs[masid][0] = gv.rs[sid][0] + gv.sd['mton']
                                    gv.rs[masid][1] = gv.rs[sid][1] + gv.sd['mtoff']
                                    gv.rs[masid][3] = gv.rs[sid][3]
                            elif gv.sd['mas'] == sid+1:
                                gv.sbits[b] = gv.sbits[b]|2**sid #(gv.sd['mas'] - 1)
                                gv.srvals[masid] = 1                              
                                set_output()                   
            
            for s in range(gv.sd['nst']):
                if gv.rs[s][1]: # if any station is running
                    program_running = True
                    gv.pon = gv.rs[s][3] # Store number of running program
                    break              
                program_running = False
                gv.pon = None

            if program_running:
                for idx in range(len(gv.ps)): # loop through program schedule (gv.ps)
                    if gv.ps[idx][1] == 0: # skip stations with no duration
                        continue
                    if gv.srvals[idx]: # If station is on, decrement time remaining
                        gv.ps[idx][1] -= 1
##                        gv.pon = gv.ps[idx][0] # Program that is currently running
                        if gv.ps[idx][1] == 0:
                            gv.ps[idx][0] = 0

            if not program_running:
                gv.srvals = [0]*(gv.sd['nst'])
                set_output()
                gv.sbits = [0] * (gv.sd['nbrd'] +1)
                gv.ps = []
                for i in range(gv.sd['nst']):
                    gv.ps.append([0,0])
                gv.rs = []
                for i in range(gv.sd['nst']):
                    gv.rs.append([0,0,0,0])
                gv.sd['bsy'] = 0

            if gv.sd['mas'] and (gv.sd['mm'] or not gv.sd['seq']): # handle master for maual or consecutave mode.
                mval = 0
                for sid in range(gv.sd['nst']):
                    bid = sid/8
                    s = sid-bid*8
                    if gv.sd['mas'] != sid +1 and (gv.srvals[sid] and gv.sd['mo'][bid]&1<<s):
                        mval = 1
                        break
                #gv.srvals[gv.sd['mas'] - 1] = mval
                if not mval:
                    gv.rs[gv.sd['mas']-1][1] = time.time() # turn off master

        if gv.sd['rd'] and now+((gv.sd['tz']/4)-12)*3600 >= gv.sd['rdst']:
            gv.sd['rd'] = 0
            gv.sd['rdst'] = 0 # Rain delay stop time
            jsave(gv.sd, 'sd')
        time.sleep(1) # End of main loop       

def data(dataf):
    """Return contents of requested text file as string."""
    f = open('./data/'+dataf+'.txt', 'r')
    data = f.read()
    f.close()
    return data

def save(dataf, datastr):
    """Save data to text file. dataf = file to save to, datastr = data string to save."""
    f = open('./data/'+dataf+'.txt', 'w')
    f.write(datastr)
    f.close()
    return

def jsave(data, fname):
    """Save data to a json file."""
    f = open('./data/'+fname+'.json', 'w')
    json.dump(data, f)
    f.close()

def load_programs():
    """Load program data from json file into memory."""
    pf = open('./data/programs.json', 'r')
    gv.pd = json.load(pf)
    pf.close()
    return gv.pd

def output_prog():
    """Converts program data to text string."""
    lpd = []
    dse = int((time.time()-time.timezone)/86400) # days since epoch
    for p in gv.pd:
        op = p[:] # Make local copy of each program
        if op[1] >= 128 and op[2] > 1:
            rel_rem = (((op[1]-128) + op[2])-(dse%op[2]))%op[2]
            op[1] = rel_rem + 128
        lpd.append(op)    
    progstr = 'var nprogs='+str(len(lpd))+',pd=[];'
    for i, pro in enumerate(lpd): #gets both index and object
        progstr += 'pd['+str(i)+']='+str(pro).replace(' ', '')+';'
    return progstr      

    #####  GPIO  #####
def set_output():
    """Activate triacs according to shift register state."""
    disableShiftRegisterOutput()
    setShiftRegister(gv.srvals) # gv.srvals stores shift register state
    enableShiftRegisterOutput()

def to_sec(d=0, h=0, m=0, s=0):
    """Convert Day, Hour, minute, seconds to number of seconds."""
    secs = d*86400
    secs += h*3600
    secs += m*60
    secs += s
    return secs
            
    ##################
    

  #### Global vars #####
sdf = open('./data/sd.json', 'r')
gv.sd = json.load(sdf) #Settings Dictionary. A set of vars kept in memory and persisted in a file
sdf.close()

try:
    gv.lg = gv.sd['lg'] # Controlls logging
except KeyError:
    pass
try:
    gv.lr = int(gv.sd['lr'])
except KeyError:
    pass

sdref = {'15':'nbrd', '16':'seq', '18':'mas', '21':'urs', '23':'wl', '25':'ipas'} #lookup table (Dictionary)

gv.srvals = [0]*(gv.sd['nst']) #Shift Register values

gv.rovals = [0]* gv.sd['nbrd']*7 #Run Once durations

gv.pd = load_programs() # Load program data from file

gv.ps = [] #Program schedule (used for UI diaplay)
for i in range(gv.sd['nst']):
    gv.ps.append([0,0])

gv.pon = None #Program on (Holds program number of a running program

gv.sbits = [0] * (gv.sd['nbrd'] +1) # Used to display stations that are on in UI 

gv.rs = [] #run schedule
for i in range(gv.sd['nst']):
    gv.rs.append([0,0,0,0]) #scheduled start time, scheduled stop time, duration, program index
    
gv.lrun=[0,0,0,0] #station index, program number, duration, end time (Used in UI)

gv.scount = 0 # Station count, used in set station to track on stations with master association.

  ####  GPIO  #####

GPIO.setwarnings(False)

  #### pin defines ####

if board_rev() == 1:
    pin_sr_dat = 21
else:
    pin_sr_dat = 27
pin_sr_clk =  4
pin_sr_noe = 17
pin_sr_lat = 22

  #### NUMBER OF STATIONS
num_stations = gv.sd['nst']

def enableShiftRegisterOutput():
    GPIO.output(pin_sr_noe, False)

def disableShiftRegisterOutput():
    GPIO.output(pin_sr_noe, True)

GPIO.cleanup()
  #### setup GPIO pins to interface with shift register ####
GPIO.setmode(GPIO.BCM)
GPIO.setup(pin_sr_clk, GPIO.OUT)
GPIO.setup(pin_sr_noe, GPIO.OUT)
disableShiftRegisterOutput()
GPIO.setup(pin_sr_dat, GPIO.OUT)
GPIO.setup(pin_sr_lat, GPIO.OUT)

def setShiftRegister(srvals):
    GPIO.output(pin_sr_clk, False)
    GPIO.output(pin_sr_lat, False)
    for s in range(num_stations):
        GPIO.output(pin_sr_clk, False)
        GPIO.output(pin_sr_dat, srvals[num_stations-1-s])
        GPIO.output(pin_sr_clk, True)
    GPIO.output(pin_sr_lat, True)

  ##################

def pass_options(opts):
    optstring = "var sd = {\n"
    for o in opts:
        optstring += "\t" + o + " : "
        if (type(gv.sd[o]) == unicode):
            optstring += "'" + gv.sd[o] + "'"
        else:
            optstring += str(gv.sd[o])
        optstring += ",\n" 
    optstring = optstring[:-2] + "\n}\n"   
    return optstring
    
  #### Class Definitions ####
class home:
    """Open Home page."""
    def GET(self):
        homepg = '<!DOCTYPE html>\n'
        homepg += data('meta')+'\n'
        homepg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        homepg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        homepg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        homepg += '<script>var ver=183,devt='+str(time.time()+((gv.sd['tz']/4)-12)*3600)+';</script>\n'
        homepg += '<script>' + pass_options(["nbrd","tz","en","rd","rs","mm","rdst","mas","urs","wl","ipas","nopts","loc","name"]) + '</script>\n'
        homepg += '<script>var sbits='+str(gv.sbits).replace(' ', '')+',ps='+str(gv.ps).replace(' ', '')+';</script>\n'
        homepg += '<script>var lrun='+str(gv.lrun).replace(' ', '')+';</script>\n'
        homepg += '<script>var snames='+data('snames')+';</script>\n'
        homepg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/home.js\"></script>'
        return homepg

class change_values:
    """Save controller values, return browser to home page."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if qdict.has_key('rsn') and qdict['rsn'] == '1':
            stop_stations()    
            raise web.seeother('/')
            return
        if qdict.has_key('en') and qdict['en'] == '':
            qdict['en'] = '1' #default
        elif qdict.has_key('en') and qdict['en'] == '0':
            gv.srvals = [0]*(gv.sd['nst']) # turn off all stations
            set_output()
        if qdict.has_key('mm') and qdict['mm'] == '0': clear_mm() #self.clear_mm()
        if qdict.has_key('rd') and qdict['rd'] != '0':
            gv.sd['rdst'] = ((time.time()+((gv.sd['tz']/4)-12)*3600)
                             +(int(qdict['rd'])*3600))
            stop_stations()
        elif qdict.has_key('rd') and qdict['rd'] == '0': gv.sd['rdst'] = 0   
        if qdict.has_key('rbt') and qdict['rbt'] == '1':
            jsave(gv.sd, 'sd')
            gv.srvals = [0]*(gv.sd['nst'])
            set_output()
            os.system('reboot')
            raise web.seeother('/')
        for key in qdict.keys():
            try:
                gv.sd[key] = int(qdict[key])
            except:
                pass
        jsave(gv.sd, 'sd')
        raise web.seeother('/')# Send browser back to home page
        return

class view_options:
    """Open the options page for viewing and editing."""
    def GET(self):
        optpg = '<!DOCTYPE html>\n'
        optpg += data('meta')+'\n'
        optpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        optpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        optpg += '<script>var baseurl=\"'+baseurl()+'\";\n' + pass_options(["tz","htp","nbrd","sdt","seq","mas","mton","mtoff","urs","wl","ipas","rst","loc","name"]) + data('options')+ '</script>\n'
        optpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/viewoptions.js\"></script>'
        return optpg

class change_options:
    """Save changes to options made on the options page."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] == 0 and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
            if qdict.has_key('oipas') and (qdict['oipas'] == 'on' or qdict['oipas'] == ''):
                gv.sd['ipas'] = 1
            else:
                gv.sd['ipas'] = 0
        except KeyError:
            pass
        try:
            if qdict['cpw'] !='' and qdict['cpw'] == qdict['npw']:
                save('pwd', base64.b64encode(qdict['npw']))
        except KeyError:
            pass
        if int(qdict['onbrd'])+1 != gv.sd['nbrd']: self.update_scount(qdict)
        
        gv.sd['tz'] = int(qdict['otz'])
        gv.sd['nbrd'] = int(qdict['onbrd'])+1
        gv.sd['nst'] = gv.sd['nbrd']*8
        gv.sd['htp']= int(qdict['ohtp'])
        gv.sd['sdt']= int(qdict['osdt'])
        gv.sd['mas'] = int(qdict['omas'])
        gv.sd['mton']= int(qdict['omton'])
        gv.sd['mtoff']= int(qdict['omtoff'])
        gv.sd['wl'] = int(qdict['owl'])
        if qdict.has_key('ours') and (qdict['ours'] == 'on' or qdict['ours'] == ''):
          gv.sd['urs'] = 1
        else:
          gv.sd['urs'] = 0
        
        if qdict.has_key('orst') and (qdict['orst'] == 'on' or qdict['orst'] == ''):
          gv.sd['rst'] = 1
        else:
          gv.sd['rst'] = 0
        
        gv.sd['loc'] = qdict['oloc']
        gv.sd['name'] = qdict['oname']
        srvals = [0]*(gv.sd['nst']) # Shift Register values
        rovals = [0]*(gv.sd['nst']) # Run Once Durations
        jsave(gv.sd, 'sd')
        
        raise web.seeother('/')
        #alert = '<script>alert("Options values saved.");window.location="/";</script>'
        return #alert # -- Alerts are not considered good interface progrmming. Use sparingly!

    def update_scount(self, qdict):
        """Increase or decrease the number of stations shown when expansion boards are added in options."""
        if int(qdict['onbrd'])+1 > gv.sd['nbrd']: # Lengthen lists
            incr = int(qdict['onbrd']) - (gv.sd['nbrd']-1)
            for i in range(incr):
                gv.sd['mo'].append(0)
            snames = data('snames')
            nlst = re.findall('[\'"].*?[\'"]', snames)
            ln = len(nlst)
            nlst.pop()
            for i in range((incr*8)+1):
                nlst.append("'S"+('%d'%(i+ln)).zfill(2)+"'")
            nstr = '['+','.join(nlst)
            nstr = nstr.replace("', ", "',")+",'']"
            save('snames', nstr)         
        elif int(qdict['onbrd'])+1 < gv.sd['nbrd']: # Shorten lists
            onbrd = qdict['onbrd']
            decr = gv.sd['nbrd'] - (onbrd+1)
            gv.sd['mo'] = gv.sd['mo'][:(onbrd+1)]
            snames = data('snames')
            nlst = re.findall('[\'"].*?[\'"]', snames)
            nstr = '['+','.join(nlst[:8+(onbrd*8)])+','']'
            save('snames', nstr)
        gv.srvals = [0] * (onbrd+1) * 8
        gv.ps = []
        for i in range((onbrd+1) * 8):
            gv.ps.append([0,0,0,0])
        gv.sbits = [0] * (onbrd+2)
        return

class view_stations:
    """Open a page to view and edit station names and master associations."""
    def GET(self):
        stationpg = '<!DOCTYPE html>\n'
        stationpg += data('meta')+'\n'
        stationpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        stationpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        stationpg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        stationpg += '<script>var baseurl=\"'+baseurl()+'\"\n' + pass_options(["nbrd","station_name_length","mas","ipas"]) + '</script>\n'
        stationpg += '<script>var masop='+str(gv.sd['mo'])+';</script>\n'
        stationpg += '<script>snames='+data('snames')+';</script>\n'
        stationpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/viewstations.js\"></script>'
        return stationpg

class change_stations:
    """Save changes to station names and master associations."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        for i in range(gv.sd['nbrd']): # capture master associations
            if qdict.has_key('m'+str(i)):
                try:
                    gv.sd['mo'][i] = int(qdict['m'+str(i)])
                except ValueError:
                    gv.sd['mo'][i] = 0
        names = '['
        for i in range(gv.sd['nst']):
            names += "'" + qdict['s'+str(i)] + "',"
        names += ']'
        save('snames', names.encode('ascii', 'backslashreplace'))
        jsave(gv.sd, 'sd')
        raise web.seeother('/')
        return

class get_station:
    """Return a page containing a number representing the state of a station or all stations if 0 is entered as statin number."""
    def GET(self, sn):
        if sn == '0':
            status = '<!DOCTYPE html>\n'
            status += ''.join(str(x) for x in gv.srvals)
            return status
        elif int(sn)-1 <= gv.sd['nbrd']*7:
            status = '<!DOCTYPE html>\n'
            status += str(gv.srvals[int(sn)-1])
            return status
        else:
            return 'Station '+sn+' not found.'

class set_station:
    """turn a station (valve/zone) on=1 or off=0 in manual mode."""
    def GET(self, nst, t=None): # nst = station number, status, optional duration
        nstlst = [int(i) for i in re.split('=|&t=', nst)]
        sid = int(nstlst[0])-1 # station index
        b = sid/8 #board index
        if nstlst[1] == 1: # if status is on
            gv.rs[sid][0] = time.time() # set start time to current time
            if nstlst[2]: # if an optional duration time is given
                gv.rs[sid][2] = nstlst[2]
                gv.rs[sid][1] = gv.rs[sid][0] + nstlst[2] # stop time = start time + duration
            else:
                gv.rs[sid][1] = float('inf') # stop time = infinity
            gv.rs[sid][3] = 99 # set program index
            gv.ps[sid][1] = nstlst[2]
            gv.sd['bsy']=1
            time.sleep(1.5)
        if nstlst[1] == 0: # If status is off
            gv.rs[sid][1] = time.time()
            time.sleep(1.5)
        raise web.seeother('/')        

class view_runonce:
    """Open a page to view and edit a run once program."""
    def GET(self):
        ropg = '<!DOCTYPE html>\n'
        ropg += data('meta')+'\n'
        ropg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        ropg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        ropg += '<script >var baseurl=\"'+baseurl()+'\"\n' + pass_options(["nbrd","mas","ipas"]) + '</script>\n'
        ropg += '<script >var dur='+str(gv.rovals).replace(' ', '')+';</script>\n'
        ropg += '<script >snames='+data('snames')+';</script>\n'
        ropg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/viewro.js\"></script>'
        return ropg

class change_runonce:
    """Start a Run Once program. This will override any running program."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if not gv.sd['en']: return # check operation status
        gv.rovals = json.loads(qdict['t'])
        gv.rovals.pop()
        gv.ps = []
        for i in range(gv.sd['nst']):
            gv.ps.append([0,0])
        gv.rs = [] #run schedule
        for i in range(gv.sd['nst']): # clear run schedule
            gv.rs.append([0,0,0,0])
        ro_now = time.time()    
        for i, v in enumerate(gv.rovals):
            if v: # if this element has a value
                gv.rs[i][0] = ro_now
                gv.rs[i][2] = v
                gv.rs[i][3] = 98
                gv.ps[i][0] = 98
                gv.ps[i][1] = v
        schedule_stations(ro_now)
        raise web.seeother('/')

class view_programs:
    """Open programs page."""
    def GET(self):
        programpg = '<!DOCTYPE html>\n'
        programpg += data('meta')+'\n'
        programpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        programpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        programpg += '<script >var baseurl=\"'+baseurl()+'\"</script>\n'       
        programpg += '<script >'+ pass_options(["nbrd","ipas","mnp"]) + output_prog()+'</script>\n'
        programpg += '<script >snames='+data('snames')+';</script>\n'
        programpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/viewprog.js\"></script>'
        return programpg
    
class modify_program:
    """Open page to allow program modification"""
    def GET(self):
        qdict = web.input()
        modprogpg = '<!DOCTYPE html>\n'
        modprogpg += data('meta')+'\n'
        modprogpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        modprogpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        modprogpg += '<script >var baseurl=\"'+baseurl()+'\"\n' + pass_options(["nbrd","ipas"]) + '\n'
        if qdict['pid'] != '-1':
            mp = gv.pd[int(qdict['pid'])][:]
            if mp[1] >= 128 and mp[2] > 1: # If this is an interval program
                dse = int((time.time()-time.timezone)/86400)
                rel_rem = (((mp[1]-128) + mp[2])-(dse%mp[2]))%mp[2] # Convert absolute to relative days remaining for display
                mp[1] = rel_rem + 128
            modprogpg += 'var pid='+qdict['pid']+', prog='+str(mp).replace(' ', '')+';</script>\n'
        else:
           modprogpg += 'var pid=-1;</script>\n'
        modprogpg += '<script >var snames='+data('snames').replace(' ', '')+';</script>\n'
        modprogpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/modprog.js\"></script>'
        return modprogpg

class change_program:
    """Add a program or modify an existing one."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        pnum = int(qdict['pid'])+1 # program number
        cp = json.loads(qdict['v'])      
        if cp[0] == 0 and pnum == gv.pon: # if disabled and program is running
            for i in range(len(gv.ps)):
                if gv.ps[i][0] == pnum:
                    gv.ps[i] = [0,0]
                if gv.srvals[i]:
                    gv.srvals[i] = 0
            for i in range(len(gv.rs)):
                if gv.rs[i][3] == pnum:
                    gv.rs[i] = [0,0,0,0]
        if cp[1] >= 128 and cp[2] > 1:
            dse = int((time.time()-time.timezone)/86400)
            ref = dse + cp[1]-128
            cp[1] = (ref%cp[2])+128            
        if int(qdict['pid']) > gv.sd['mnp']:
            alert = '<script>alert("Maximum number of programs\n has been reached.");window.location="/";</script>'
            return alert
        elif qdict['pid'] == '-1': #add new program
            gv.pd.append(cp)
        else:
            gv.pd[int(qdict['pid'])] = cp #replace program
        jsave(gv.pd, 'programs')
        gv.sd['nprogs'] = len(gv.pd)
        raise web.seeother('/vp')      
        return

class delete_program:
    """Delete one or all existing program(s)."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if qdict['pid'] == '-1':
            del gv.pd[:]
            jsave(gv.pd, 'programs')
        else:    
            del gv.pd[int(qdict['pid'])]
        jsave(gv.pd, 'programs')
        gv.sd['nprogs'] = len(gv.pd)
        raise web.seeother('/vp')
        return
                          
class graph_programs:
    """Open page to display program schedule"""
    def GET(self):
        qdict = web.input()
        t = time.time()
        lt = time.gmtime(t+((gv.sd['tz']/4)-12)*3600)
        if qdict['d'] == '0': dd = str(lt.tm_mday)
        else: dd = str(qdict['d'])
        if qdict.has_key('m'): mm = str(qdict['m'])
        else: mm = str(lt.tm_mon)
        if qdict.has_key('y'): yy = str(qdict['y'])
        else: yy = str(lt.tm_year)
        graphpg = '<!DOCTYPE html>\n'
        graphpg += data('meta')+'\n'
        graphpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        graphpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        graphpg += '<script>var baseurl=\"'+baseurl()+'\";\n' + pass_options(["mas","seq","wl","sdt","mton","mtoff","nbrd","ipas","mnp"]) + '</script>\n'
        graphpg += '<script>var devday='+str(int(t/86400))+',devmin='+str((lt.tm_hour*60)+lt.tm_min)+',dd='+dd+',mm='+mm+',yy='+yy+';var masop='+str(gv.sd['mo'])+';'+output_prog()+'</script>\n'
        graphpg += '<script>var snames='+data('snames').replace(' ', '')+';</script>\n'
        graphpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8.3/plotprog.js\"></script>'
        return graphpg

class view_log:
    def __init__(self):
        self.render = web.template.render('templates/', globals={'sd':gv.sd})
 
    def GET(self):
        logf = open('static/log/water_log.csv')
        records = logf.readlines()
        logf.close()
        data = []
        for r in records:
            t = r.split(', ')
            t[1] = t[1].decode('unicode-escape')
            data.append(t)    
        return self.render.log(data)

class clear_log:
    """Delete all log records"""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        f = open('./static/log/water_log.csv', 'w')
        f.write('Program, Zone, Duration, Finish Time, Date'+'\n')
        f.close
        raise web.seeother('/vl')
        return

class log_options:
    """Set log options from dialog."""
    def GET(self):
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if qdict.has_key('log'): gv.sd['lg'] = "checked"
        else: gv.sd['lg'] = ""
        gv.lg = gv.sd['lg'] # necessary to make logging work correctly on Pi (see run_log())        
        gv.sd['lr'] = qdict['nrecords']
        gv.lr = int(gv.sd['lr'])
        jsave(gv.sd, 'sd')
        raise web.seeother('/vl')
        return
        

if __name__ == '__main__':
    app = web.application(urls, globals())
    thread.start_new_thread(main_loop, ())
    app.run()
