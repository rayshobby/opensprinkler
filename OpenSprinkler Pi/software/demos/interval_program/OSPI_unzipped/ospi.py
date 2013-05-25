#!/usr/bin/python

import web, re, os, json, time, base64, thread

   ####  GPIO  #####
import RPi.GPIO as GPIO
GPIO.setwarnings(False)

  #### pin defines ####
#pin_sr_dat = 21 #Use for rev 1 Pi
pin_sr_dat = 27 #Use for rev 2 Pi
pin_sr_clk =  4
pin_sr_noe = 17
pin_sr_lat = 22

 #########################
urls = (
    '/',  'home',
    '/cv', 'change_values',
    '/vo', 'view_options',
    '/co', 'change_options',
    '/vs', 'view_stations',
    '/cs', 'change_stations', # name and master
    '/sn=(\d+?\Z)', 'get_station', # regular expression, accepts any station number
    '/sn(\d+?=\d(&t=\d+?\Z)?)', 'set_station', # regular expression, accepts any digits
    '/vr', 'view_runonce',
    '/cr', 'change_runonce',
    '/vp', 'view_programs',
    '/mp', 'modify_program', # open 'Modify program' window
    '/cp', 'change_program',
    '/dp', 'delete_program',
    '/gp', 'graph_programs',
    )

  #### Function Definitions ####
def baseurl():
    """Return URL app is running under.""" 
    baseurl = web.ctx['home']
    return baseurl

def log_run(lastrun, datetime):
    """add run data to csv file - most recent first."""
    datastr = ('P'+str(lrun[1])+', S'+str(lrun[0]+1)+', '+str(lrun[2]/60)+':'+str(lrun[2]%60)+
               ', '+time.strftime("%H:%M:%S, %d/%m/%y", time.localtime(datetime))+'\n')
    f = open('./static/log/water_log.csv', 'r')
    log = f.readlines()
    f.close()
    log.insert(1, datastr)
    f = open('./static/log/water_log.csv', 'w')
    f.writelines(log[:100])
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
    global rs, sd
    #print 'scheduling stations' # for testing
    accumulate_time = curr_time #+ 1
    for s in range(sd['nst']):
        if rs[s][2]: # if station has a duration value
            rs[s][0] = accumulate_time # set accumulated start time
            accumulate_time += rs[s][2] # add duration
            rs[s][1] = accumulate_time # set new stop time
            accumulate_time += sd['sdt'] # add station delay
    sd['bsy'] = 1
    #print 'controller buisy' # for testing

def check_globals():
    print 'lrun = ' + str(lrun)
    print 'ps = '+ str(ps)
    print 'sbits = '+str(sbits)

def main_loop(): # Runs in a seperate thread
    global rovals, srvals, pd, ps, sd, sbits, rs, lrun, ml_thread_started
    try:
        rpdb2.settrace() # for debugging
    except:
        pass
    print 'Starting main loop \n'
    ml_thread_started = True
    last_min = 0
    while True: # infinite loop
        match = 0
        now = time.time()
        if not sd['mm'] and not sd['bsy'] and not sd['rd'] and sd['en']:
            lt = time.localtime(now)
            if (lt[3]*60)+lt[4] != last_min: # only check programs once a minute
                last_min = (lt[3]*60)+lt[4]
                load_programs()
                for i, p in enumerate(pd): # get both index and prog item 
                    if prog_match(p) and p[0] and p[6]: # check if program time matches now and is active
                        for b in range(sd['nbrd']): # check each station 
                            for s in range(8):
                                sid = b*8+s # station index
                                if sd['mas'] == sid+1: continue # skip if this is master valve
                                if srvals[sid]: continue # skip if currently on ??
                                if p[7+b]&1<<s: # if this station is scheduled in this program
                                    lock.acquire()
                                    rs[sid][2] = p[6]*sd['wl']/100 # duration scaled by water level
                                    rs[sid][3] = i # store program index
                                    ps[sid][0] = i+1 # store program number
                                    ps[sid][1] = rs[sid][2] # duration
                                    lock.release()
                                    match = 1
            if match:
                schedule_stations(now)

        if sd['bsy']:
            for b in range(sd['nbrd']): 
                for s in range(8):
                    sid = b*8 + s
                    if srvals[sid]: # if this station is on
                        if now >= rs[sid][1] and not any(rovals): # check if time is up
                            lock.acquire()
                            srvals[sid] = 0
                            lock.release()
                            set_output()
                            if sd['mas'] != sid+1: # if not master, fill out log
                                lock.acquire()
                                lrun[0] = sid
                                lrun[1] = rs[sid][3] + 1
                                lrun[2] = int(now - rs[sid][0])
                                lrun[3] = now-(time.timezone-(time.daylight*3600))
                                lock.release()
                                #log_run(lrun, now)
                            elif sd['mas'] == sid+1:
                                #print 'stopping master valve' # for testing
                                lock.acquire()
                                sbits[b] -= 2**(sid)
                                lock.release()
                    else:
                        if now >= rs[sid][0] and now < rs[sid][1] and not any(rovals):
                            if sd['mas'] != sid+1:
                                lock.acquire()
                                srvals[sid] = 1 # station is turned on
                                lock.release()
                                set_output()
                                lock.acquire()
                                sbits[b] = 2**sid
                                ps[sid][0] = rs[sid][3] + 1
                                ps[sid][1] = rs[sid][2]
                                lock.release()
                                if sd['mas'] and sd['mas'] != sid+1 and int(sd['m'+str(b)])&1<<s and sd['mm'] == 0:
                                    masid = sd['mas'] - 1
                                    lock.acquire()
                                    rs[masid][0] = rs[sid][0] + sd['mton']
                                    rs[masid][1] = rs[sid][1] - sd['mtoff']
                                    rs[masid][3] = rs[sid][3]
                                    lock.release()    
                            elif sd['mas'] == sid+1:
                                #print 'starting master valve' # for testing
                                lock.acquire()
                                srvals[masid] = 1
                                lock.release()
                                set_output()
                                lock.acquire()
                                sbits[b] += 2**masid
                                lock.release()
            #print srvals #for testing
            
            for s in range(sd['nst']):
                if ps[s][1]:
                    program_running = True
                    break              
                program_running = False

            if program_running == True:
                for idx in range(len(ps)): # loop through program schedule (ps)
                    if sd['rsn'] == 1:
        ####                #stop irrigation and clean up
                        lock.acquire()
                        srvals = [0]*(sd['nst'])
                        lock.release()
                        set_output()
                        lock.acquire()
                        ps = []
                        for i in range(sd['nst']):
                            ps.append([0,0])
                        rs = []
                        for i in range(sd['nst']):
                            rs.append([0,0,0,0])    
                        sbits = [0] * (sd['nbrd'] +1)
                        sd['bsy'] = 0
                        lock.release()
                        break
                    if ps[idx][1] == 0: # skip stations with no duration
                        continue
                    if srvals[idx]: # If station is on, decrement time remaining
                        lock.acquire()
                        ps[idx][1] -= 1
                        if ps[idx][1] == 0:
                            ps[idx][0] = 0
                        lock.release()    

            if program_running == False:
                lock.acquire()
                srvals = [0]*(sd['nst'])
                lock.release()
                set_output()
                lock.acquire()
                sbits = [0] * (sd['nbrd'] +1)
                ps = []
                for i in range(sd['nst']):
                    ps.append([0,0])
                rs = []
                for i in range(sd['nst']):
                    rs.append([0,0,0,0])
                sd['bsy'] = 0
                lock.release()
                #print 'controller free' # for testing

        if sd['rd'] and now-(time.timezone-(time.daylight*3600)) >= sd['rdst']:
            lock.acquire()
            sd['rd'] = 0
            sd['rdst'] = 0
            lock.release()
            jsave(sd, 'sd')
            
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
    progstr = 'var nprogs='+str(len(lpd))+',nboards='+str(sd['nbrd'])+',ipas='+str(sd['ipas'])+',mnp='+str(sd['mnp'])+',pd=[];'
    for i, pro in enumerate(lpd): #gets both index and object
        progstr += 'pd['+str(i)+']='+str(pro).replace(' ', '')+';'
    return progstr      

    #####  GPIO  #####
def set_output():
        disableShiftRegisterOutput()
        setShiftRegister(srvals)
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
sd = json.load(sdf) #Settings Dictionary. A set of vars kept in memory
sdf.close()

sdref = {'15':'nbrd', '18':'mas', '21':'urs', '23':'wl', '25':'ipas'} #lookup table

srvals = [0]*(sd['nst']) #Shift Register values

rovals = [0]* sd['nbrd']*7 #Run Once Durations

pd = load_programs() #Program data

ps = [] #Program shedule (used for UI diaplay)
for i in range(sd['nst']):
    ps.append([0,0]) # station, duration

sbits = [0] * (sd['nbrd'] +1) # Used to display stations that are on in UI 

rs = [] #run schedule
for i in range(sd['nst']):
    rs.append([0,0,0,0]) #scheduled start time, scheduled stop time, duration, program index
    
lrun=[0,0,0,0] #station index, program number, duration, end time (Used in UI)

ml_thread_started = False

  ####  GPIO  #####

  #### NUMBER OF STATIONS
num_stations = sd['nst']

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
        homepg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        homepg += '<script>var ver=182,devt='+str(time.time()-(time.timezone-(time.daylight*3600)))+';var nbrd='+str(sd['nbrd'])+',tz='+str(sd['tz'])+';</script>\n'
        homepg += '<script>var en='+str(sd['en'])+',rd='+str(sd['rd'])+',rs='+str(sd['rs'])+',mm='+str(sd['mm'])+',rdst='+str(sd['rdst'])+',mas='+str(sd['mas'])+',urs='+str(sd['urs'])+',wl='+str(sd['wl'])+',ipas='+str(sd['ipas'])+',loc="";</script>\n'
        homepg += '<script>var sbits='+str(sbits).replace(' ', '')+',ps='+str(ps).replace(' ', '')+';</script>\n'
        homepg += '<script>var lrun='+str(lrun).replace(' ', '')+';</script>\n'
        homepg += '<script>var snames='+data('snames')+';</script>\n'
        homepg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/home.js\"></script>'
        return homepg

class change_values:
    """Save controller values, return browser to home page."""
    def GET(self):
        global sd, srvals, sbits, ps
        qdict = web.input()
        try:
            if sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if qdict.has_key('rsn') and qdict['rsn'] == '1':
            srvals = [0]*(sd['nst'])
            set_output()            
            ps = []
            for i in range(sd['nst']):
                ps.append([0,0])   
            sbits = [0] * (sd['nbrd'] +1)
            sd['bsy'] = 0
            raise web.seeother('/')
            return
        if qdict['en'] == '':
            qdict['en'] = '1' #default
        elif qdict['en'] == '0':
            srvals = [0]*(sd['nst']) # turn off all stations
            set_output()
        if qdict['mm'] == '0': self.clear_mm()
        if qdict['rd'] != '0':
            sd['rdst'] = ((time.time()-(time.timezone-(time.daylight*3600)))
                          +(int(qdict['rd'])*3600))
        else: sd['rdst'] = 0    
        if  qdict['rbt'] == '1':
            jsave(sd, 'sd')
            srvals = [0]*(sd['nst'])
            set_output()
            os.system('reboot')
            raise web.seeother('/')
        for key in qdict.keys():
            try:
                sd[key] = int(qdict[key])
            except:
                pass
        jsave(sd, 'sd')
        if sd['mm'] == 1:
            thread.start_new_thread(self.mm_timer, ())
        raise web.seeother('/')# Send browser back to home page
        return
    
    def mm_timer(self):
        """Threaded timer for manual mode."""
        global ps, sd, sbits, srvals
        while sd['mm'] == 1:
            for i in range(len(ps)):
                if ps[i][1] == 1:
                    srvals[i] = 0
                    ps[i][0]=0
                    sbidx = (i/8)
                    sbits[sbidx] -= 2**(i-(sbidx*8))
                if ps[i][1] > 0:
                    ps[i][1] -= 1
            time.sleep(1)
            set_output()
        return

    def clear_mm(self):
        """Clear manual mode settings."""
        global sbits, ps, srvals
        sbits = [0] * (sd['nbrd'] +1)
        ps = []
        for i in range(sd['nst']):
            ps.append([0,0])
        srvals = [0]*(sd['nst'])
        set_output()     
        return

class view_options:
    """Open the options page for viewing and editing."""
    def GET(self):
        optpg = '<!DOCTYPE html>\n'
        optpg += data('meta')+'\n'
        optpg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        optpg += '<script>var opts=["Time zone:",0,'+str(sd['tz'])+',1,"HTTP port:",0,'+str(sd['htp'])+',12,"",0,0,13,"Ext. boards:",\
0,'+str(sd['nbrd']-1)+',15,"Station delay:",0,'+str(sd['sdt'])+',17,"Master station:",0,'+str(sd['mas'])+',18,"Mas. on adj.:",0,'+str(sd['mton'])+',19,"Mas. off adj.:",0,'+str(sd['mtoff'])+',20,\
"Use rain sensor:",1,'+str(sd['urs'])+',21,"Normally open:",1,'+str(sd['rst'])+',22,"Water level (%):",0,'+str(sd['wl'])+',23,\
"Ignore password:",1,'+str(sd['ipas'])+',25,0];</script>\n'
        optpg += '<script>var nopts='+str(sd['nopts'])+',loc="'+str(sd['loc'])+'";</script>\n'
        optpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewoptions.js\"></script>'
        return optpg

class change_options:
    """Save changes to options made on the options page."""
    def GET(self):
        qdict = web.input()
        try:
            if not qdict.has_key('o25') and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
            elif qdict.has_key('o25') and sd['ipas'] == 0 and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
            elif qdict.has_key('o25') and sd['ipas'] == 0 and qdict['pw'] == base64.b64decode(sd['pwd']):
                sd['ipas'] = 1
        except KeyError:
            pass
        if qdict['cpw'] !='' and qdict['cpw'] == qdict['npw']: save('pwd', base64.b64encode(qdict['npw']))
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
                  sd[sdref[optlst[i]]]=0
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
        if int(qdict['o15'])+1 != sd['nbrd']: self.update_scount(qdict)
        self.update_sd(qdict)
        raise web.seeother('/')
        #alert = '<script>alert("Options values saved.");window.location="/";</script>'
        return #alert # ---- Alerts are not considered good interface progrmming. Use sapringly!

    def update_sd(self, qdict):
        sd['nbrd'] = int(qdict['o15'])+1
        sd['nst'] = sd['nbrd']*8
        sd['sdt']= int(qdict['o17'])
        sd['mas'] = int(qdict['o18'])
        sd['mton']= int(qdict['o19'])
        sd['mtoff']= int(qdict['o20'])
        sd['tz'] = int(qdict['o1'])
        if qdict.has_key('o21'): sd['urs'] = int(qdict['o21'])
        sd['wl'] = int(qdict['o23'])
        if qdict.has_key('o25'): sd['ipas'] = int(qdict['o25'])
        sd['loc'] = qdict['loc'] 
        srvals = [0]*(sd['nst']) # Shift Register values
        rovals = [0]*(sd['nst']) # Run Once Durations
        jsave(sd, 'sd')
        return

    def update_scount(self, qdict):
        """Increase or decrease the number of stations shown when expansion boards are added in options."""
        global ps, sbits, srvals
        if int(qdict['o15'])+1 > sd['nbrd']: # Lengthen lists
            incr = int(qdict['o15']) - (sd['nbrd']-1)
            snames = data('snames')
            nlst = re.findall('[\'"].*?[\'"]', snames)
            ln = len(nlst)
            nlst.pop()
            for i in range((incr*8)+1):
                nlst.append("'S"+('%d'%(i+ln)).zfill(2)+"'")
            nstr = '['+','.join(nlst)
            nstr = nstr.replace("', ", "',")+",'']"
            save('snames', nstr)         
        elif int(qdict['o15'])+1 < sd['nbrd']: # Shorten lists
            decr = sd['nbrd'] - (int(qdict['o15'])+1)
            snames = data('snames')
            nlst = re.findall('[\'"].*?[\'"]', snames)
            nstr = '['+','.join(nlst[:8+(int(qdict['o15'])*8)])+','']'
            save('snames', nstr)
        srvals = [0] * (int(qdict['o15'])+1) * 8
        ps = []
        for i in range((int(qdict['o15'])+1) * 8):
            ps.append([0,0])
        sbits = [0] * (int(qdict['o15'])+2)
        return

class view_stations:
    """Open a page to view and edit station names and master association."""
    def GET(self):
        stationpg = '<!DOCTYPE html>\n'
        stationpg += data('meta')+'\n'
        stationpg += '<script>var baseurl=\"'+baseurl()+'\"</script>\n'
        stationpg += '<script>var nboards='+str(sd['nbrd'])+',maxlen=12,mas='+str(sd['mas'])+',ipas='+str(sd['ipas'])+';</script>\n'
        mo = ''
        for i in range(sd['nbrd']):
            mo += str(sd['m'+str(i)])+','
        stationpg += '<script>var masop=['+mo+'0];</script>\n'
        stationpg += '<script>snames='+data('snames')+';</script>\n'
        stationpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewstations.js\"></script>'
        return stationpg

class change_stations:
    """Save changes to station names and master associations."""
    def GET(self):
        qdict = web.input()
        try:
            if sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        for i in range(4): # capture master associations
            if qdict.has_key('m'+str(i)):
                sd['m'+str(i)] = qdict['m'+str(i)]  
        names = '['
        for i in range(sd['nst']):
            names += "'" + qdict['s'+str(i)] + "',"
        names += ']'
        save('snames', names)
        jsave(sd, 'sd')
        raise web.seeother('/')
        return

class get_station:
    """Return a page containing a number representing the state of a station or all stations if 0 is entered as statin number."""
    def GET(self, sn):
        if sn == '0':
            status = '<!DOCTYPE html>\n'
            status += ''.join(str(x) for x in srvals)
            return status
        elif int(sn)-1 <= sd['nbrd']*7:
            status = '<!DOCTYPE html>\n'
            status += str(srvals[int(sn)-1])
            return status
        else:
            return 'Station '+sn+' not found.'

class set_station:
    """turn a station (valve/zone) on=1 or off=0."""
    def GET(self, nst, t=None): # nst = number, status, time
        nstlst = re.split('=|&', nst)
        global ps, sd, srvals
        srvals[int(nstlst[0])-1] = int(nstlst[1])
        sbidx = ((int(nstlst[0])-1)/8)
        if sbidx:
            snum = int(nstlst[0])-(sbidx*8)
        else:
            snum = int(nstlst[0])
        if int(nstlst[1]):
            ps[(int(nstlst[0]))-1][0] = 99
            ps[(int(nstlst[0]))-1][1] = int(nstlst[3])
            sbits[sbidx] += int(2**(snum-1))
        else:
            sbits[sbidx] -= int(2**(snum-1))
        if sbits[sbidx] < 0:
            sbits[sbidx] = 0
        set_output()   
        raise web.seeother('/')
        return

class view_runonce:
    """Open a page to view and edit a run once program."""
    def GET(self):
        ropg = '<!DOCTYPE html>\n'
        ropg += data('meta')+'\n'
        ropg += '<script >var baseurl=\"'+baseurl()+'\"</script>\n'
        ropg += '<script >var nboards='+str(sd['nbrd'])+',mas='+str(sd['mas'])+',ipas='+str(sd['ipas'])+',dur='+str(rovals).replace(' ', '')+';</script>\n'
        ropg += '<script >snames='+data('snames')+';</script>\n'
        ropg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/viewro.js\"></script>'
        return ropg

class change_runonce:
    """Start a Run Once program."""
    def GET(self):
        qdict = web.input()
        global rovals, srvals, ps, sd
        try:
            if sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        if not sd['en']: return # check operation status
        sd['rsn'] = 0
        sd['bsy'] = 1
        rovals = json.loads(qdict['t'])
        rovals.pop()
        ps = []
        for i in range(sd['nst']):
            ps.append([0,0])
        for i, t in enumerate(rovals):
            if t != 0:
                ps[i][0] = 98
                ps[i][1] = t
                rs[i][1] = time.time() + t
        thread.start_new_thread(self.run, ())
        raise web.seeother('/')
        return

    def run(self):
        global srvals, ps, sd, sbits
        sd['bsy'] = 1
        idx = 0
        while idx < len(ps): # loop through program schedule (ps)
            if sd['rsn'] == 1:
                #### stop irrigation and clean up ####
                srvals = [0]*(sd['nst'])
                set_output()
                ps = []
                for i in range(sd['nst']):
                    ps.append([0,0])   
                sbits = [0] * (sd['nbrd'] +1)
                sd['bsy'] = 0
                break
            if ps[idx][1] == 0: # skip stations with no duration
                idx += 1
                continue
            #### start irrigation ####
            srvals[idx]=1
            set_output()
            sbits[idx/8] = 2**(idx%8)
            while ps[idx][1] > 0:
                if sd['rsn'] == 1:
                    break
                time.sleep(1)               
                #ps[idx][1] -= 1
            ps[idx][0] = 0
            sbits[idx/8] = 0
            #### stop irrigation ####
            srvals[idx]=0
            set_output()
            idx += 1
        sd['bsy'] = 0
        return

class view_programs:
    """Open programs page."""
    def GET(self):
        programpg = '<!DOCTYPE html>\n'
        programpg += data('meta')+'\n'
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
        modprogpg += '<script >var baseurl=\"'+baseurl()+'\"</script>\n'
        modprogpg += '<script >var nboards='+str(sd['nbrd'])+',ipas='+str(sd['ipas'])+';\n'
        if qdict['pid'] != '-1':
            mp = pd[int(qdict['pid'])][:]
            if mp[1] >= 128 and mp[2] > 1: # If this is an interval program
                dse = int((time.time()-time.timezone)/86400)
                rel_rem = (((mp[1]-128) + mp[2])-(dse%mp[2]))%mp[2] # Convert absolute to relative days remaining for displa
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
        try:
            if sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(sd['pwd']):
                raise web.unauthorized()
                return
        except KeyError:
            pass
        cp = json.loads(qdict['v'])
        if cp[1] >= 128 and cp[2] > 1:
            dse = int((time.time()-time.timezone)/86400)
            ref = dse + cp[1]-128
            cp[1] = (ref%cp[2])+128            
        if int(qdict['pid']) > sd['mnp']:
            alert = '<script>alert("Maximum number of programs\n has been reached.");window.location="/";</script>'
            return alert
        elif qdict['pid'] == '-1': #add new program
            pd.insert(0, cp)
        else:
            pd[int(qdict['pid'])] = cp #replace program
        jsave(pd, 'programs')
        sd['nprogs'] = len(pd)
        raise web.seeother('/vp')      
        return

class delete_program:
    """Delete one or all existing program(s)."""
    def GET(self):
        global pd, sd
        qdict = web.input()
        try:
            if sd['ipas'] != 1 and qdict['pw'] != base64.b64decode(sd['pwd']):
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
        sd['nprogs'] = len(pd)
        raise web.seeother('/vp')
        return
                          
class graph_programs:
    """Open page to display program schedule"""
    def GET(self):
        qdict = web.input()
        lt = time.localtime(time.time())
        if qdict['d'] == '0': dd = str(lt.tm_mday)
        else: dd = str(qdict['d'])
        if qdict.has_key('m'): mm = str(qdict['m'])
        else: mm = str(lt.tm_mon)
        if qdict.has_key('y'): yy = str(qdict['y'])
        else: yy = str(lt.tm_year)
        graphpg = '<script >var baseurl=\"'+baseurl()+'\"</script>\n'
        graphpg += '<script >var mas='+str(sd['mas'])+',wl='+str(sd['wl'])+',sdt='+str(sd['sdt'])+',mton='+str(sd['mton'])+',mtoff='+str(sd['mtoff'])+',devday='+str(int(time.time()/86400))+',devmin='+str(lt.tm_min)+',dd='+dd+',mm='+mm+',yy='+yy+';var masop=['+str(sd['m0'])+',0];'+output_prog()+'</script>\n'
        graphpg += '<script >var snames='+data('snames').replace(' ', '')+';</script>\n'
        graphpg += '<script src=\"'+baseurl()+'/static/scripts/java/svc1.8/plotprog.js\"></script>'
        return graphpg

if not ml_thread_started:
    lock=thread.allocate_lock()
    thread.start_new_thread(main_loop, ())

if __name__ == '__main__':
##    lock=thread.allocate_lock()
##    thread.start_new_thread(main_loop, ())
    app = web.application(urls, globals())
    app.run()
