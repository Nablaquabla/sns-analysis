#!/home/bjs66/anaconda2/bin/python2.7
"""
Created on Mon Feb 01 15:03:56 2016

@author: Nablaquabla
"""

import h5py
import numpy as np
import easyfit as ef
import datetime
import pytz
import os

# Prepare timezones and beginning of epcoch for later use
utc = pytz.utc
eastern = pytz.timezone('US/Eastern')
epochBeginning = utc.localize(datetime.datetime(1970,1,1))

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    runDirs = ['Run-15-06-25-12-53-44','Run-15-06-26-11-23-13','Run-15-07-31-18-30-14',
               'Run-15-08-18-14-51-18','Run-15-08-31-00-23-36','Run-15-09-21-20-58-01',
               'Run-15-09-23-21-16-00','Run-15-10-03-09-26-22','Run-15-10-13-13-27-09',
               'Run-15-10-21-13-12-27','Run-15-10-29-15-56-36','Run-15-11-09-11-30-13',
               'Run-15-11-20-11-34-48','Run-15-11-24-15-35-32','Run-15-12-14-11-21-45',
               'Run-15-12-26-08-30-40','Run-16-01-07-12-16-36','Run-16-02-02-16-26-26',
               'Run-16-02-15-13-46-34','Run-16-02-29-11-54-20','Run-16-03-09-13-00-14',
               'Run-16-03-22-18-09-33','Run-16-03-30-12-44-57','Run-16-04-12-11-54-27',
               'Run-16-04-20-11-22-48','Run-16-05-05-14-08-52','Run-16-05-17-14-40-34',
               'Run-16-06-02-12-35-56','Run-16-06-17-12-09-12','Run-16-06-27-17-50-08',
               'Run-16-07-06-18-25-19','Run-16-07-12-11-44-55','Run-16-07-18-11-50-24',
               'Run-16-07-21-11-59-39','Run-16-07-28-12-49-17']
#    runDirs = ['Run-15-06-25-12-53-44']

    dataDir = '/home/bjs66/csi/bjs-analysis/Processed/'
    powerDir = '/home/bjs66/csi/bjs-analysis/BeamPowerHistory/'

    # Read stop times of runs:
    data = np.loadtxt('/home/bjs66/GitHub/sns-analysis/start-stop-times-of-runs.txt',dtype=str)

    # Prepare output file
    h5Out = h5py.File(dataDir + 'Stability/Acceptances.h5','w')

    # Prepare beam power file
    h5Power= h5py.File(powerDir + 'BeamPowerHistory.h5','r')

    # For each run to be analyzed
    for run in runDirs:
        print run
        # Find each day in the run
        dayNames = np.sort(os.listdir(dataDir + run))

        # Get the correct stopping time for the run
        for d in data:
            if run in d:
                easternEndTS = eastern.localize(datetime.datetime.strptime(d[1],'%y-%m-%d-%H-%M-%S'))
                utcEndTS = (easternEndTS.astimezone(utc) - epochBeginning).total_seconds()

        # Prepare output arrays:
        dOut = {'time': [], 'sPT': [], 'bPT': [], 'muon': [], 'linGate': [], 'overflow': [], 'power': [], 'duration': [], 'beamOnDuration': [], 'beamOffDuration': []}


        # For each day in the run
        for dN in dayNames:

            # Open the day as input file from which all the data will be derived
            h5In = h5py.File(dataDir + run + '/' + dN)

            # Crop the time from the filename
            day = '20' + dN.split('.')[0]

            # Check if this is the last day in a run, if so set the last timestamp for this day to the end
            # otherwise set last tiemstamp for this day to the second before midnight
            easternDayTS = eastern.localize(datetime.datetime.strptime(day,'%Y%m%d'))
            if easternDayTS.date() == easternEndTS.date():
                lastTSForThisDay = utcEndTS
            else:
                lastTSForThisDay = ((easternDayTS + datetime.timedelta(days=1)).astimezone(utc) - epochBeginning).total_seconds()

            # Read power data for current day
            timeData = h5Power['/%s/time'%day][...]
            powerData = h5Power['/%s/power'%day][...]

            # Get all time data in the info file, convert it to seconds since epoch
            # and add the proper last timestamp of the day as determined above
            timeArray = np.sort(h5In['/I'].keys())
            utcTS = []
            for time in timeArray:
                dayTS = datetime.datetime.strptime('%s%s'%(day,time),'%Y%m%d%H%M%S')
                easternTS = eastern.localize(dayTS)
                utcTS.append((easternTS.astimezone(utc) - epochBeginning).total_seconds())
            utcTS.append(lastTSForThisDay)

            # For each time in the info file get the data needed to calculate the acceptances for those
            # periods and integrate the power for that timefram
            for i in range(len(timeArray)):
                # Get the time as eastern time string
                time = timeArray[i]

                # Get the proper start and stop timestamps in utc seconds since epoch
                tsStart = utcTS[i]
                tsStop = utcTS[i+1] - 1
                duration = tsStop - tsStart

                # Read data
                nWaveforms = np.sum(h5In['/I/%s/waveformsProcessed'%time][...])
                sPTAccept = np.sum(h5In['/I/%s/sPeaksPTDist'%time][...][:3])
                bPTAccept = np.sum(h5In['/I/%s/bPeaksPTDist'%time][...][:3])
                muonHits = len(h5In['/I/%s/muonHits'%time][...])
                linGates = np.sum(h5In['/I/%s/linearGates'%time][...])
                overflows = np.sum(h5In['/I/%s/overflows'%time][...])

                # Integrate the total power on target between the start and stop time
                cutPowerTimes = (timeData>=tsStart) * (timeData<=tsStop)
                beamOnSeconds = np.sum(powerData[cutPowerTimes] >= 5e-5)
                beamOffSeconds = (tsStop - tsStart + 1) - beamOnSeconds
                beamPower = np.sum(powerData[cutPowerTimes])/3600.0 # In MWhr
                
                dOut['time'].append(tsStart)
                dOut['sPT'].append(1.0*sPTAccept/nWaveforms)
                dOut['bPT'].append(1.0*bPTAccept/nWaveforms)
                dOut['muon'].append(1.0*muonHits/nWaveforms)
                dOut['linGate'].append(1.0*linGates/nWaveforms)
                dOut['overflow'].append(1.0*overflows/nWaveforms)
                dOut['power'].append(beamPower)
                dOut['duration'].append(duration)
                dOut['beamOnDuration'].append(beamOnSeconds)
                dOut['beamOffDuration'].append(beamOffSeconds)

            h5In.close()
                
        for key in ['time','sPT','bPT','muon','linGate','overflow','power','duration','beamOnDuration','beamOffDuration']:
            if '/%s/%s'%(run,key) in h5Out:
                del h5Out['/%s/%s'%(run,key)]
            h5Out.create_dataset('/%s/%s'%(run,key),data=dOut[key])
    h5Out.close()
    h5Power.close()












