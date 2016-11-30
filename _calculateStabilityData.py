#!/home/bjs66/anaconda2/bin/python
"""
Created on Mon Feb 01 15:03:56 2016

@author: Nablaquabla
"""

import h5py
import numpy as np
import datetime
import os
import sys

# Keys available: B,S,I/time/key
# B,S:
# 'timestamp','bl-csi','bl-mv','pe-pt','pe-roi','pe-iw','arrival','charge',
# 'rt10','rt50','rt90','muon1','muon2','muon3','pe-total'
# I:
# waveformsProcessed, linearGates, overflows, bWindowsAnalyzed, sWindowsAnalyzed,
# speChargeDist, bPeaksPTDist, sPeaksPTDist, csiBaselineDist, csiBaseline, peakWidthDist
# peakAmplitudeDist, bigPulseOnset, bigPulseCharge, muonHits, chargeDist2d

def main(args):
    run = args[1]

    # Create hdf5 file
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed'
    h5Out = h5py.File(mainDir + '/Stability/%s.h5'%run,'w')
    
    # Determine day files corresponding to the run
    h5DataFiles = [x for x in os.listdir(mainDir + '/%s'%run) if '.h5' in x]
    
    # Data we want to track for a run
    dataDict = {}
    dataDict['elapsedTime'] = np.array([])
    
    distributionKeys = ['peakWidthDist','peakAmplitudeDist','speChargeDist',
                        'csiBaselineDist','sPeaksPTDist','bPeaksPTDist']
                 
    distributionLimits = {'peakWidthDist': [0,49],
                          'peakAmplitudeDist': [0,98],
                          'speChargeDist': [-50,248],
                          'sPeaksPTDist': [0,50],
                          'bPeaksPTDist': [0,50],
                          'csiBaselineDist': [85,115],
                          'muonHitDist': [0,35000],
                          'bigPulseChargeDist': [0,100000],
                          'bigPulseOnsetDist': [0,35000],
                          'waveformChargeDist': [0,35000],
                          'highNPEChargeDist': [0,35000]}

    dataDict['peakWidthDist'] = np.zeros(51)
    dataDict['peakAmplitudeDist'] = np.zeros(100)
    dataDict['speChargeDist'] = np.zeros(300)
    dataDict['csiBaselineDist'] = np.zeros(32)
    dataDict['sPeaksPTDist'] = np.zeros(52)
    dataDict['bPeaksPTDist'] = np.zeros(52)
    dataDict['muonHitDist'] = np.zeros(350)
    dataDict['bigPulseChargeDist'] = np.zeros(1000)
    dataDict['bigPulseOnsetDist'] = np.zeros(350)
    dataDict['waveformChargeDist'] = np.zeros(350)
    dataDict['highNPEChargeDist'] = np.zeros(350)
    
    scalarKeys = ['csiBaseline','linearGates','overflows','muonHits','waveformsProcessed',
                  'bigPulses','sWindowsAnalyzed','bWindowsAnalyzed']
                  
    scalarKeyConversion = {'csiBaseline': 'csiBaseline',
                           'linearGates': 'linearGates',
                           'overflows': 'overflows',
                           'muonHits': 'muonHits',
                           'waveformsProcessed': 'waveformsProcessed',
                           'bigPulses': 'bigPulseOnset',
                           'sWindowsAnalyzed': 'sWindowsAnalyzed',
                           'bWindowsAnalyzed': 'bWindowsAnalyzed'}
                           
    scalarFunctions = {'csiBaseline': lambda x: np.mean(x),
                       'linearGates': lambda x: np.sum(x),
                       'overflows': lambda x: np.sum(x),
                       'muonHits': lambda x: len(x),
                       'waveformsProcessed': lambda x: np.sum(x)/(60.0**3),
                       'bigPulses': lambda x: len(x),
                       'sWindowsAnalyzed': lambda x: np.sum(x),
                       'bWindowsAnalyzed': lambda x: np.sum(x)}

    for sKey in scalarKeys:
        dataDict[sKey] = np.array([])

    # Reference time stamp
    t0 = datetime.datetime(2015,06,25,0,0,0)   
    
    # Read each day file and process the hourly data
    for h5f in np.sort(h5DataFiles):
        day = h5f.split('.')[0]
        f = h5py.File(mainDir + '/%s/%s'%(run,h5f),'r')
        for time in np.sort(f['/I/'].keys()):
            elapsedTime = np.round((datetime.datetime.strptime(day+time,'%y%m%d%H%M%S') - t0).total_seconds()/60.0)
            dataDict['elapsedTime'] = np.append(dataDict['elapsedTime'],elapsedTime)
            
            # Process scalar data
            for sKey in scalarKeys:
                dataDict[sKey] = np.append(dataDict[sKey],map(scalarFunctions[sKey],[f['/I/%s/%s'%(time,scalarKeyConversion[sKey])]]))
        
            # Process primary distribution data
            for dKey in distributionKeys:
                dataDict[dKey] = np.vstack((dataDict[dKey],f['/I/%s/%s'%(time,dKey)]))
            
            # Process secondary distribution data
            dKey = 'muonHitDist'
            dataDict[dKey] = np.vstack((dataDict[dKey],np.histogram(f['/I/%s/%s'%(time,'muonHits')],350,[0,35000])[0]))
            dKey = 'bigPulseChargeDist'
            dataDict[dKey] = np.vstack((dataDict[dKey],np.histogram(f['/I/%s/%s'%(time,'bigPulseCharge')],1000,[0,100000])[0]))
            dKey = 'bigPulseOnsetDist'
            dataDict[dKey] = np.vstack((dataDict[dKey],np.histogram(f['/I/%s/%s'%(time,'bigPulseOnset')],350,[0,35000])[0]))
#            dKey = 'waveformChargeDist'
#            dataDict[dKey] = np.vstack((dataDict[dKey],np.histogram(f['/I/%s/%s'%(time,dKey)],350,[0,35000])[0]))
            dKey = 'highNPEChargeDist'
            dataDict[dKey] = np.vstack((dataDict[dKey],np.sum(f['/I/%s/%s'%(time,'chargeDist2d')][35:],axis=0)))
        f.close()

    # Write all distribution data to file
    for dKey in distributionKeys:
        dataDict[dKey] = dataDict[dKey][1:].T
        dataDict[dKey] = 1.0 * dataDict[dKey][:-1] / np.sum(dataDict[dKey][:-1],axis=0)
        dataExtent = [dataDict['elapsedTime'][0],dataDict['elapsedTime'][-1],distributionLimits[dKey][0],distributionLimits[dKey][1]]

        h5Out.create_dataset(dKey,data=dataDict[dKey])
        h5Out[dKey].attrs.create('extent',data=dataExtent)
        h5Out[dKey].attrs.create('maximum',data=np.max(dataDict[dKey]))
    
    # Write all secondary distribution data to file
    for dKey in ['muonHitDist','bigPulseChargeDist','bigPulseOnsetDist','highNPEChargeDist']:
        dataDict[dKey] = dataDict[dKey][1:].T
        dataExtent = [dataDict['elapsedTime'][0],dataDict['elapsedTime'][-1],distributionLimits[dKey][0],distributionLimits[dKey][1]]

        h5Out.create_dataset(dKey,data=dataDict[dKey])
        h5Out[dKey].attrs.create('extent',data=dataExtent)
        h5Out[dKey].attrs.create('maximum',data=np.max(dataDict[dKey]))
    
    # Write all scalar data to file
    for sKey in scalarKeys:
        h5Out.create_dataset(sKey,data=dataDict[sKey])

    # Write time data to file
    h5Out.create_dataset('elapsedTime',data=dataDict['elapsedTime'])
    
    # Properly close hdf5 file
    h5Out.close()
    
# ============================================================================
#                                Run program
# ============================================================================        
if __name__ == '__main__':
    main(sys.argv)

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
