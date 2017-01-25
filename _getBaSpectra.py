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
maxPeInPTDict = {0: [20,30],
                 5: [15,20,30],
                 10: [15,20],
                 15: [20]}
nBins = 40

# ============================================================================
#                                Run program
# ============================================================================
def main(args):
    run = args[1]

    # Declare main and run dirs
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed/'
    runDir = mainDir + run

    # Get all days in given run folder
    daysInRun = [x.split('.')[0] for x in os.listdir(runDir)]

    # Prepare output file
    h5Out = h5py.File(mainDir + 'Spectra/' + run + '.h5','a')

    # For each day in the run folder read the HDF5 file and fit SPEQ spectra
    for day in daysInRun:
        h5In = h5py.File(runDir + '/' + day + '.h5', 'r')

        # Get time and spe charges of current day
        speCharges = {}
        speCharges['time'] = h5In['/SPEQ/Times'][...]
        speCharges['gauss'] = h5In['/SPEQ/GaussBest'][...][:,0]
        speCharges['polya'] = h5In['/SPEQ/PolyaBest'][...][:,0]

        # For both signal and background windows calculate the spectra, where data is split into
        # beam on and off periods for both signal and background regions.
        for wd in ['S','B']:

            # Get data from file
                # Used in cuts
            peInPT = h5In['/%s/pe-pt'%wd][...] 
            peInIW = h5In['/%s/pe-iw'%wd][...]
            rt1090 = h5In['/%s/rt90'%wd][...] - h5In['/%s/rt10'%wd][...]
            rt050 = h5In['/%s/rt50'%wd][...]

                # Used for energy spectra
            charge = h5In['/%s/charge'%wd][...]
            speWindow = h5In['/%s/speQindex'%wd][...]
            evtTime = h5In['/%s/timestamp'%wd][...]
            
            # Cycle through all different times for which we have different SPE charges
            for i in range(len(speCharges['time'])):
    
                # Determine which data actually was taken in those charge windows
                cutTime = (speWindow == i)
                
                # Convert charge subset to NPE
                currentNpeGauss = charge[cutTime]/speCharges['gauss'][i]
                currentNpePolya = charge[cutTime]/speCharges['polya'][i]

                # Separate the data that is necessary from the other data arrays                
                currentPeInPT = peInPT[cutTime]
                currentPeInIW = peInIW[cutTime]
                currentRt1090 = rt1090[cutTime]
                currentRt050 = rt050[cutTime]
                
                for minPEIW in [0,4,5,6]:
                    print 'Working on day:', day
                    print 'Window:', wd
                    print 'Cherenkov:', minPEIW
                    cutCherenkov = (currentPeInIW >= minPEIW)

                    for minPEPT in [0,5]:
                        cutMinPEPT = (currentPeInPT >= minPEPT)

                        for maxPEPT in maxPeInPTDict[minPEPT]:
                            cutMaxPEPT = (currentPeInPT <= maxPEPT)

                            for minRT050 in [0,50]:
                                cutMinRT050 = (currentRt050 >= minRT050)

                                for maxRT050 in [1250,1500]:
                                    cutMaxRT050 = (currentRt050 <= maxRT050)

                                    for minRT1090 in [0,125]:
                                        cutMinRT1090 = (currentRt1090 >= minRT1090)

                                        for maxRT1090 in [1375,1500]:
                                            cutMaxRT1090 = (currentRt1090 <= maxRT1090)
                                            
                                            cutTotal = cutCherenkov * cutMinPEPT * cutMaxPEPT * cutMinRT050 * cutMaxRT050 * cutMinRT1090 * cutMaxRT1090
                                            hdf5Key = '/%d/%s/IW-%i/MinPT-%i/MaxPT-%i/MinRT050-%i/MaxRT050-%i/MinRT1090-%i/MaxRT1090-%i/'%(speCharges['time'][i],wd,minPEIW,minPEPT,maxPEPT,minRT050,maxRT050,minRT1090,maxRT1090)

                                            spectrumGauss = np.histogram(currentNpeGauss[cutTotal],nBins,[-0.5,nBins-0.5])[0]
                                            spectrumPolya = np.histogram(currentNpePolya[cutTotal],nBins,[-0.5,nBins-0.5])[0]

                                            h5Out.create_dataset(hdf5Key + 'gauss', data=spectrumGauss,dtype=np.int)
                                            h5Out.create_dataset(hdf5Key + 'polya', data=spectrumPolya,dtype=np.int)
        h5In.close()
    h5Out.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
















