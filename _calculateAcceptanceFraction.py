#!/home/bjs66/anaconda2/bin/python
"""
Created on Mon Feb 01 15:03:56 2016

@author: Nablaquabla
"""
import h5py
import os
import numpy as np
import easyfit as ef
import sys


# ============================================================================
#                                Run program
# ============================================================================
def main(args):
    minPEinIW = int(args[1])
    minPEinPT = int(args[2])
    maxPEinPT = int(args[3])
    minRT050 = int(args[4])
    maxRT050 = int(args[5])
    minRT1090 = int(args[6])
    maxRT1090 = int(args[7])

    uncutKey = 'IW-%i/MinPT-%i/MaxPT-%i/MinRT050-%i/MaxRT050-%i/MinRT1090-%i/MaxRT1090-%i/'%(0,minPEinPT,maxPEinPT,0,1500,0,1500)
    cutKey = 'IW-%i/MinPT-%i/MaxPT-%i/MinRT050-%i/MaxRT050-%i/MinRT1090-%i/MaxRT1090-%i/'%(minPEinIW,minPEinPT,maxPEinPT,minRT050,maxRT050,minRT1090,maxRT1090)

    runDirs = ['Run-15-03-27-12-42-26','Run-15-03-30-13-33-05','Run-15-04-08-11-38-28',
               'Run-15-04-17-16-56-59','Run-15-04-29-16-34-44','Run-15-05-05-16-09-12',
               'Run-15-05-11-11-46-30','Run-15-05-19-17-04-44','Run-15-05-27-11-13-46'] 
    
    nBins = 40
    background = {'uncut': {'gauss': np.zeros(nBins), 'polya': np.zeros(nBins)},
                  'cut': {'gauss': np.zeros(nBins), 'polya': np.zeros(nBins)}}
    signal     = {'uncut': {'gauss': np.zeros(nBins), 'polya': np.zeros(nBins)},
                  'cut': {'gauss': np.zeros(nBins), 'polya': np.zeros(nBins)}}


    for run in runDirs:
        h5In = h5py.File('/home/bjs66/csi/bjs-analysis/Processed/Spectra/' + run + '.h5','r')
        
        for time in h5In['/'].keys():
            for speType in ['gauss','polya']:
                background['uncut'][speType] = background['uncut'][speType] + h5In['/%s/B/'%time + uncutKey + speType][...]
                signal['uncut'][speType] = signal['uncut'][speType] + h5In['/%s/S/'%time + uncutKey + speType][...]
                background['cut'][speType] = background['cut'][speType] + h5In['/%s/B/'%time + cutKey + speType][...]
                signal['cut'][speType] = signal['cut'][speType] + h5In['/%s/S/'%time + cutKey + speType][...]
        h5In.close()

    for speType in ['gauss','polya']:
        uncutBackground = background['uncut'][speType]
        uncutBackgroundError = np.sqrt(background['uncut'][speType])
        uncutSignal = signal['uncut'][speType]
        uncutSignalError = np.sqrt(signal['uncut'][speType])
        uncutResidual = uncutSignal - uncutBackground
        uncutResidualError = np.sqrt(uncutBackground + uncutSignal)

        cutBackground = background['cut'][speType]
        cutBackgroundError = np.sqrt(background['cut'][speType])
        cutSignal = signal['cut'][speType]
        cutSignalError = np.sqrt(signal['cut'][speType])
        cutResidual = cutSignal - cutBackground
        cutResidualError = np.sqrt(cutBackground + cutSignal)

        acceptance = 1.0 * cutResidual / uncutResidual
        _tUncutResidual = uncutResidual[:]
        _tUncutResidual[_tUncutResidual == 0] = 1
 
        _tCutResidual = cutResidual[:]
        _tCutResidual[_tCutResidual == 0] = 1
        acceptanceError = acceptance * np.sqrt((cutResidualError/_tCutResidual)**2 + (uncutResidualError/_tUncutResidual)**2)
    
        with open('/home/bjs66/csi/bjs-analysis/Processed/Spectra/AcceptanceFraction-%s-%d-%d-%d-%d-%d-%d-%d.dat'%(speType,minPEinIW,minPEinPT,maxPEinPT,minRT050,maxRT050,minRT1090,maxRT1090),'w') as f:
            f.write('Uncut reference spectrum: %s\n'%uncutKey)
            f.write('Cut reference spectrum: %s\n'%cutKey)
            f.write('NPE uncutB uncutBError uncutS uncutSError uncutRes uncutResError cutB cutBError cutS cutSError cutRes cutResError Acceptance AcceptanceError\n')
            for i in range(nBins):
                f.write('%d %d %.2f %d %.2f %d %.2f %d %.2f %d %.2f %d %.2f %.6f %.6f\n'%(i,uncutBackground[i],uncutBackgroundError[i],uncutSignal[i],uncutSignalError[i],uncutResidual[i],uncutResidualError[i],
                                                                                                    cutBackground[i],cutBackgroundError[i],cutSignal[i],cutSignalError[i],cutResidual[i],cutResidualError[i],
                                                                                                    acceptance[i],acceptanceError[i]))
            
        
# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
















