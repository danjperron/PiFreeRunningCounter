import freeRunningCounter as counter;
import time

class ElapseCounter:
      
    def __init__(self):
        self.initCount= counter.get()

    def start(self):
        self.initCount= counter.get()

    def getLapse(self):
        return (counter.get() - self.initCount)

    def getSecondLapse(self):
        return (self.getLapse() / 1.0e6)

class AlarmeCounter:

    def __init__(self, ID, i2cAddr):
        self.initCount= counter.get()
        self.Target= 0
        self.Enable = False
        self.Alarm= False
        self.NewAlarm= False
        self.ID = ID
        self.i2cAddr= i2cAddr

    def start(self, targetSec):
        self.initCount= counter.get()
        self.TargetSec = targetSec
        self.Enable = True
        self.Alarm = False
        self.NewAlarm= False

    def stop(self):
        self.Alarm= False
        self.Enable= False
        self.NewAlarm= False

    def  getSecond(self):
        return (counter.get() - self.initCount) / 1.0e6

    def  getDeciSecond(self):
        #utilisation de la division en entier //
        return (counter.get() - self.initCount)//100000


    def checkAlarm(self):
        self.NewAlarm=False
        if not self.Enable:
           self.Alarm = False
        elif not self.Alarm:
           tempsecoule = self.getSecond()
           if(tempsecoule >= self.TargetSec):
              self.Alarm=True
              self.NewAlarm=True
        return self.Alarm

    def refreshDisplay(self):
        #send display value
        lapse = self.getDeciSecond()
        #ok arrange la valeur pour affiche
        # send to display i2c= self.i2cAddr
        

print("Une boucle de 10  avec intervale de 1 sec")

lapse = ElapseCounter()

print("start")
lapse.start()
timet = time.time();

#set target pour 1 seconde
target = 1.0 
for loop in range(10):
   while True:
     actuel = lapse.getSecondLapse()
     if actuel >= target:
       print("time function = {:.3f}  64bits free running = {:.3f}".format(time.time()-timet, actuel))
       target = target + 1.0
       break
     time.sleep(0.001)     
  

print("\nUtilise la classe AlarmCounter pour creer 8  alarmes donc chacunes auront une seconde de l'autre")

# creons les 8 alarmes  avec un ID incrementant  (utlisons le i du for-range)

displayI2C = [ 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77]

NbAlarme=8
alarmes = [AlarmeCounter(i,displayI2C[i]) for i in range(NbAlarme)]

timet= time.time();

#1 seconde d'intervalle chacune
intervale = 1.0
for a in alarmes:
   a.start(intervale)
   intervale=intervale + 1 

# boucle de servitude

AlarmeDetecter = 0
timet = time.time();

MainAlarme = False
while  AlarmeDetecter < NbAlarme:

     # verifions les alarmes
     alarmFlag = False
     for a in alarmes:
        a.refreshDisplay()
        if a.checkAlarm():
           AlarmeDetecter = AlarmeDetecter + 1
           print("Alarme detecte ID:{}  time function = {:.3f} 64bits free running = {:.3f}".format(a.ID, time.time()-timet,a.getSecond() ))
           a.stop()
           alarmFlag = True

     MainAlarme = alarmFlag;

     # SetBuzzerON(MainAlarm)

     # gerons le reste ajouton un delai raisonable
     time.sleep(0.05) 


     
print("Done")

  


        
