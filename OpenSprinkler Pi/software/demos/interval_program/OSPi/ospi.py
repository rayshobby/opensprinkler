#!/usr/bin/python
"""Updated 21/July/2013."""

import web, re, os, json, time, base64, thread, gv
import RPi.GPIO as GPIO

GPIO.setwarnings(False)

  #### pin defines ####
#pin_sr_dat = 21 #Use for rev 1 Pi
pin_sr_dat = 27 #Use for rev 2 Pi
pin_sr_clk =  4
pin_sr_noe = 17
pin_sr_lat = 22

  #### Import ospi_addon module if it exists. ####
try:
    import ospi_addon #This provides a stub for adding custom features to ospi.py.
except ImportError:
    pass

 #########################
urls = [
    '/',  'home',
    '/cv', 'change_values',
    '/vo', 'view_options',
    '/co', 'change_options',
    '/vs', 'view_stations',
    '/cs', 'change_stations', # name and master association
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
    

def prog_match(prog):
    if not prog[0]: return 0   
    devday = int(time.time()/86400) # Check day match
    lt = time.localtime(time.time())
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
    if gv.sd['rd']:
        return
    accumulate_time = curr_time #+ 1
    for s in range(gv.sd['nst']):
        if gv.rs[s][2]: # if station has a duration value
            gv.rs[s][0] = accumulate_time # start at accumulated time
            accumulate_time += gv.rs[s][2] # add duration
            gv.rs[s][1] = accumulate_time # set new stop time
            accumulate_time += gv.sd['sdt'] # add station delay
    gv.sd['bsy'] = 1
    return 

def main_loop(): # Runs in a seperate thread
    print 'Starting main loop \n'
    last_min = 0
    while True: # infinite loop
        match = 0
        now = time.time()
        if not gv.sd['mm'] and not gv.sd['bsy'] and not gv.sd['rd'] and gv.sd['en']:
            lt = time.localtime(now)
            if (lt[3]*60)+lt[4] != last_min: # only check programs once a minute
                last_min = (lt[3]*60)+lt[4]
                load_programs()
                for i, p in enumerate(pd): # get both index and prog item 
                    if prog_match(p) and p[0] and p[6]: # check if program time matches now and is active
                        for b in range(gv.sd['nbrd']): # check each station 
                            for s in range(8):
                                sid = b*8+s # station index
                                if gv.sd['mas'] == sid+1: continue # skip if this is master valve
                                if gv.srvals[sid]: continue # skip if currently on ??
                                if p[7+b]&1<<s: # if this station is scheduled in this program
                                    gv.rs[sid][2] = p[6]*gv.sd['wl']/100 # duration scaled by water level
                                    gv.rs[sid][3] = i+1 # store program number
                                    gv.ps[sid][0] = i+1 # store program number
                                    gv.ps[sid][1] = gv.rs[sid][2] # duration
                                    match = 1
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
                                if gv.sd['mas'] and gv.sd['m'+str(b)]&1<<s-(s/8)*8: # Master settings
                                    masid = gv.sd['mas'] - 1 # master index
                                    gv.rs[masid][0] = gv.rs[sid][0] + gv.sd['mton']
                                    gv.rs[masid][1] = gv.rs[sid][1] + gv.sd['mtoff']
                                    gv.rs[masid][3] = gv.rs[sid][3]
                            elif gv.sd['mas'] == sid+1:
                                gv.sbits[b] = gv.sbits[b]|2**sid #(gv.sd['mas'] - 1)
                                gv.srvals[masid] = 1                              
                                set_output()                   
            
            for s in range(gv.sd['nst']):
                if gv.rs[s][1]: # if any program is running
                    program_running = True
                    break              
                program_running = False

            if program_running:
                for idx in range(len(gv.ps)): # loop through program schedule (gv.ps)
                    if gv.ps[idx][1] == 0: # skip stations with no duration
                        continue
                    if gv.srvals[idx]: # If station is on, decrement time remaining
                        gv.ps[idx][1] -= 1
                        gv.pon = gv.ps[idx][0] # Program that is currently running
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

        if gv.sd['rd'] and now+((gv.sd['tz']/4)-12)*3600 >= gv.sd['rdst']:
            gv.sd['rd'] = 0
            gv.sd['rdst'] = 0
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
    global pd
    pf = open('./data/programs.json', 'r')
    pd = json.load(pf)
    pf.close()
    return pd

def output_prog():
    lpd = []
    dse = int((time.time()-time.timezone)/86400) # days since epoch
    for p in pd:
        op = p[:] # Make local copy of each program
        if op[1] >= 128 and op[2] > 1:
            rel_rem = (((op[1]-128) + op[2])-(dse%op[2]))%op[2]
            op[1] = rel_rem + 128
        lpd.append(op)    
    progstr = 'var nprogs='+str(len(lpd))+',nboards='+str(gv.sd['nbrd'])+',ipas='+str(gv.sd['ipas'])+',mnp='+str(gv.sd['mnp'])+',pd=[];'
    for i, pro in enumerate(lpd): #gets both index and object
        progstr += 'pd['+str(i)+']='+str(pro).replace(' ', '')+';'
    return progstr      

    #####  GPIO  #####
def set_output():
        disableShiftRegisterOutput()
        setShiftRegister(gv.srvals) # gv.srvals stores shift register state
        enableShiftRegisterOutput()

def to_sec(d=0, h=0, m=0, s=0):
    secs = d*86400
    secs += h*3600
    secs += m*60
    secs += s
    return secs
            
    ##################
    

  #### Global vars #####
sdf = open('./data/sd.json', 'r')
gv.sd = json.load(sdf) #Settings Dictionary. A set of vars kept in memory
sdf.close()

try:
    gv.lg = gv.sd['lg'] # Controlls logging
except KeyError:
    pass
try:
    gv.lr = int(gv.sd['lr'])
except KeyError:
    pass

sdref = {'15':'nbrd', '18':'mas', '21':'urs', '23':'wl', '25':'ipas'} #lookup table

gv.srvals = [0]*(gv.sd['nst']) #Shift Register values

gv.rovals = [0]* gv.sd['nbrd']*7 #Run Once Durations

pd = load_programs() # Load program data from file

gv.ps = [] #Program schedule (used for UI diaplay)
for i in range(gv.sd['nst']):
    gv.ps.append([0,0]) # program, duration

gv.pon = None

gv.sbits = [0] * (gv.sd['nbrd'] +1) # Used to display stations that are on in UI 

gv.rs = [] #run schedule
for i in range(gv.sd['nst']):
    gv.rs.append([0,0,0,0]) #scheduled start time, scheduled stop time, duration, program index
    
gv.lrun=[0,0,0,0] #station index, program number, duration, end time (Used in UI)

gv.scount = 0 # Station count, used in set station to track on stations with master association.

  ####  GPIO  #####

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

  #### Class Definitions ####
class home:
    """Open Home page."""
    def GET(self):
        homepg = '<!DOCTYPE html>\n'
        homepg += data('meta')+'\n'
        homepg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        homepg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        homepg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        homepg += '<script>var ver=182,devt='+str(time.time()+((gv.sd['tz']/4)-12)*3600)+';var nbrd='+str(gv.sd['nbrd'])+',tz='+str(gv.sd['tz'])+';</script>\n'
        homepg += '<script>var en='+str(gv.sd['en'])+',rd='+str(gv.sd['rd'])+',rs='+str(gv.sd['rs'])+',mm='+str(gv.sd['mm'])+',rdst='+str(gv.sd['rdst'])+',mas='+str(gv.sd['mas'])+',urs='+str(gv.sd['urs'])+',wl='+str(gv.sd['wl'])+',ipas='+str(gv.sd['ipas'])+',loc="'+str(gv.sd['loc'])+'";</script>\n'
        homepg += '<script>var sbits='+str(gv.sbits).replace(' ', '')+',ps='+str(gv.ps).replace(' ', '')+';</script>\n'
        homepg += '<script>var lrun='+str(gv.lrun).replace(' ', '')+';</script>\n'
        homepg += '<script>var snames='+data('snames')+';</script>\n'
        homepg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/home.js\"></script>'
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
            gv.srvals = [0]*(gv.sd['nst'])
            set_output()            
            gv.ps = []
            for i in range(gv.sd['nst']):
                gv.ps.append([0,0])   
            gv.sbits = [0] * (gv.sd['nbrd'] +1)
            gv.sd['bsy'] = 0
            gv.rs = [] #run schedule
            for i in range(gv.sd['nst']):
                gv.rs.append([0,0,0,0])
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
            gv.srvals = [0]*(gv.sd['nst']) # turn off all stations
            set_output()
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
        optpg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        optpg += '<script>var opts=["Time zone:",0,'+str(gv.sd['tz'])+',1,"HTTP port:",0,'+str(gv.sd['htp'])+',12,"",0,0,13,"Ext. boards:",\
0,'+str(gv.sd['nbrd']-1)+',15,"Station delay:",0,'+str(gv.sd['sdt'])+',17,"Master station:",0,'+str(gv.sd['mas'])+',18,"Mas. on adj.:",0,'+str(gv.sd['mton'])+',19,"Mas. off adj.:",0,'+str(gv.sd['mtoff'])+',20,\
"Use rain sensor:",1,'+str(gv.sd['urs'])+',21,"Normally open:",1,'+str(gv.sd['rst'])+',22,"Water level (%):",0,'+str(gv.sd['wl'])+',23,\
"Ignore password:",1,'+str(gv.sd['ipas'])+',25,0];</script>\n'
        optpg += '<script>var nopts='+str(gv.sd['nopts'])+',loc="'+str(gv.sd['loc'])+'";</script>\n'
        optpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewoptions.js\"></script>'
        return optpg

class change_options:
    """Save changes to options made on the options page."""
    def GET(self):
        qdict = web.input()
        try:
            if not qdict.has_key('o25') and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
            elif qdict.has_key('o25') and gv.sd['ipas'] == 0 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
            elif qdict.has_key('o25') and gv.sd['ipas'] == 0 and qdict['pw'] == base64.b64decode(gv.sd['pwd']):
                gv.sd['ipas'] = 1
        except KeyError:
            pass
        try:
            if qdict['cpw'] !='' and qdict['cpw'] == qdict['npw']:
                gv.sd['pwd'] = base64.b64encode(qdict['npw'])
        except KeyError:
            pass 
        vstr = data('options')
        ops = vstr.index('[')+1
        ope = vstr.index(']')
        optstr = vstr[ops:ope]
        optlst = optstr.split(',')
        onumlst = []
        i=3
        while i < len(optlst):
            onumlst.append(optlst[i].replace(' ', ''))
            if optlst[i-2] == '1': #clear check box items
                optlst[i-1]= '0'
                try:
                  sdref[optlst[i]];  
                  gv.sd[sdref[optlst[i]]]=0
                except KeyError:
                    pass
            i+=4
        for key in qdict.keys():
            if key[:1] == 'o':
                oidx = onumlst.index(key[1:])
                if qdict[key] == 'on' or '':
                    qdict[key] = '1'
                optlst[(oidx*4)+2] = qdict[key]   
        optstr = ','.join(optlst)
        optstr = optstr.replace(', ', ',')
        vstr = vstr.replace(vstr[ops:ope], optstr)
        save('options', vstr)
        if int(qdict['o15'])+1 != gv.sd['nbrd']: self.update_scount(qdict)
        if int(qdict['o18']) != gv.sd['mas']:
            clear_mm()
        self.update_sd(qdict)
        raise web.seeother('/')
        #alert = '<script>alert("Options values saved.");window.location="/";</script>'
        return #alert # ---- Alerts are not considered good interface progrmming. Use sparingly!

    def update_sd(self, qdict):
        gv.sd['nbrd'] = int(qdict['o15'])+1
        gv.sd['nst'] = gv.sd['nbrd']*8
        gv.sd['sdt']= int(qdict['o17'])
        gv.sd['mas'] = int(qdict['o18'])
        gv.sd['mton']= int(qdict['o19'])
        gv.sd['mtoff']= int(qdict['o20'])
        gv.sd['tz'] = int(qdict['o1'])
        if qdict.has_key('o21'): gv.sd['urs'] = int(qdict['o21'])
        gv.sd['wl'] = int(qdict['o23'])
        if qdict.has_key('o25'): gv.sd['ipas'] = int(qdict['o25'])
        gv.sd['loc'] = qdict['loc'] 
        gv.srvals = [0]*(gv.sd['nst']) # Shift Register values
        gv.rovals = [0]*(gv.sd['nst']) # Run Once Durations
        jsave(gv.sd, 'sd')
        return

    def update_scount(self, qdict):
        """Increase or decrease the number of stations shown when expansion boards are added in options."""
        if int(qdict['o15'])+1 > gv.sd['nbrd']: # Lengthen lists
            incr = int(qdict['o15']) - (gv.sd['nbrd']-1)
            snames = data('snames')
            nlst = re.findall('[\'"].*?[\'"]', snames)
            ln = len(nlst)
            nlst.pop()
            for i in range((incr*8)+1):
                nlst.append("'S"+('%d'%(i+ln)).zfill(2)+"'")
            nstr = '['+','.join(nlst)
            nstr = nstr.replace("', ", "',")+",'']"
            save('snames', nstr)         
        elif int(qdict['o15'])+1 < gv.sd['nbrd']: # Shorten lists
            decr = gv.sd['nbrd'] - (int(qdict['o15'])+1)
            snames = data('snames')
            nlst = re.findall('[\'"].*?[\'"]', snames)
            nstr = '['+','.join(nlst[:8+(int(qdict['o15'])*8)])+','']'
            save('snames', nstr)
        gv.srvals = [0] * (int(qdict['o15'])+1) * 8
        gv.ps = []
        for i in range((int(qdict['o15'])+1) * 8):
            gv.ps.append([0,0])
        gv.rs = []
        for i in range((int(qdict['o15'])+1) * 8):
            gv.rs.append([0,0,0,0])    
        gv.sbits = [0] * (int(qdict['o15'])+2)
        return

class view_stations:
    """Open a page to view and edit station names and master association."""
    def GET(self):
        stationpg = '<!DOCTYPE html>\n'
        stationpg += data('meta')+'\n'
        stationpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        stationpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        stationpg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        stationpg += '<script>var nboards='+str(gv.sd['nbrd'])+',maxlen=12,mas='+str(gv.sd['mas'])+',ipas='+str(gv.sd['ipas'])+';</script>\n'
        mo = ''
        for i in range(gv.sd['nbrd']):
            mo += str(gv.sd['m'+str(i)])+','
        stationpg += '<script>var masop=['+mo+'0];</script>\n'
        stationpg += '<script>snames='+data('snames')+';</script>\n'
        stationpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewstations.js\"></script>'
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
        for i in range(4): # capture master associations
            if qdict.has_key('m'+str(i)):
                try:
                    gv.sd['m'+str(i)] = int(qdict['m'+str(i)])
                except ValueError:
                    gv.sd['m'+str(i)] = 0
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
            try:
                if int(gv.sd['m'+str(b)])&1<<sid -(sid/8)*8: # If this has a master association
                    gv.rs[gv.sd['mas']-1][1] = time.time()
            except ValueError:
                pass
            time.sleep(1.5)
        raise web.seeother('/')        

class view_runonce:
    """Open a page to view and edit a run once program."""
    def GET(self):
        ropg = '<!DOCTYPE html>\n'
        ropg += data('meta')+'\n'
        ropg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        ropg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        ropg += '<script >var baseurl=\"'+baseurl()+'\"</script>\n'
        ropg += '<script >var nboards='+str(gv.sd['nbrd'])+',mas='+str(gv.sd['mas'])+',ipas='+str(gv.sd['ipas'])+',dur='+str(gv.rovals).replace(' ', '')+';</script>\n'
        ropg += '<script >snames='+data('snames')+';</script>\n'
        ropg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewro.js\"></script>'
        return ropg

class change_runonce:
    """Start a Run Once program. This wil. override any running program."""
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
        programpg += '<script >'+output_prog()+'</script>\n'
        programpg += '<script >snames='+data('snames')+';</script>\n'
        programpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewprog.js\"></script>'
        return programpg
    
class modify_program:
    """Open page to allow program modification"""
    def GET(self):
        qdict = web.input()
        modprogpg = '<!DOCTYPE html>\n'
        modprogpg += data('meta')+'\n'
        modprogpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        modprogpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        modprogpg += '<script >var baseurl=\"'+baseurl()+'\"</script>\n'
        modprogpg += '<script >var nboards='+str(gv.sd['nbrd'])+',ipas='+str(gv.sd['ipas'])+';\n'
        if qdict['pid'] != '-1':
            mp = pd[int(qdict['pid'])][:]
            if mp[1] >= 128 and mp[2] > 1: # If this is an interval program
                dse = int((time.time()-time.timezone)/86400)
                rel_rem = (((mp[1]-128) + mp[2])-(dse%mp[2]))%mp[2] # Convert absolute to relative days remaining for display
                mp[1] = rel_rem + 128
            modprogpg += 'var pid='+qdict['pid']+', prog='+str(mp).replace(' ', '')+';</script>\n'
        else:
           modprogpg += 'var pid=-1;</script>\n'
        modprogpg += '<script >var snames='+data('snames').replace(' ', '')+';</script>\n'
        modprogpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/modprog.js\"></script>'
        return modprogpg

class change_program:
    """Add a program or modify an existing one."""
    def GET(self):
        qdict = web.input()
        #print qdict
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
                print i
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
            pd.append(cp)
        else:
            pd[int(qdict['pid'])] = cp #replace program
        jsave(pd, 'programs')
        gv.sd['nprogs'] = len(pd)
        raise web.seeother('/vp')      
        return

class delete_program:
    """Delete one or all existing program(s)."""
    def GET(self):
        global pd
        qdict = web.input()
        try:
            if gv.sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(gv.sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if qdict['pid'] == '-1':
            del pd[:]
            jsave(pd, 'programs')
        else:    
            del pd[int(qdict['pid'])]
        jsave(pd, 'programs')
        gv.sd['nprogs'] = len(pd)
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
        graphpg = '<script >var baseurl=\"'+baseurl()+'\"</script>\n'
        graphpg += '<link href="./static/images/icons/favicon.ico" rel="icon" type="image/x-icon" />\n'
        graphpg += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />\n'
        graphpg += '<script >var mas='+str(gv.sd['mas'])+',wl='+str(gv.sd['wl'])+',sdt='+str(gv.sd['sdt'])+',mton='+str(gv.sd['mton'])+',mtoff='+str(gv.sd['mtoff'])+',devday='+str(int(t/86400))+',devmin='+str((lt.tm_hour*60)+lt.tm_min)+',dd='+dd+',mm='+mm+',yy='+yy+';var masop=['+str(gv.sd['m0'])+',0];'+output_prog()+'</script>\n'
        graphpg += '<script >var snames='+data('snames').replace(' ', '')+';</script>\n'
        graphpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/plotprog.js\"></script>'
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
