import sys
import time
import threading
import rc

class componentLog():
    def __init__(self):
        self.logName = "No Log name"
        self.verbositty = "NOTIFY"
        pass

    def setLogVerbosity(self, verbosity):
        self.verbosity = verbosity

    def setLogName(self, logName):
        self.logName = logName

    def startLog(self, logTargetHandle):
        self.logTargetHandle = logTargetHandle
        self.terminateLog = False
        self.logProducerHandle = threading.Thread(target=self.logProducer)
        self.logProducerHandle.start()

    def logProducer(self):
        while True:
            self.logTargetHandle("This is a " + self.logName + " log entry with Verbosity: " + self.logVerbosity)
            if self.terminateLog == True:
                break
            time.sleep(1)

    def stopLog(self):
        self.terminateLog = True
        self.logProducerHandle.join()
