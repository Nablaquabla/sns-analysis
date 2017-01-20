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
    run = args[1]

    # Declare main and run dirs
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed/'
    runDir = mainDir + run

    # Get all days in given run folder
    daysInRun = [x.split('.')[0] for x in os.listdir(runDir)]

#    nBins = 100
#    spectra = {'Gauss': 'BeamOff': {'B': np.zeros(nBins), 'S': np.zeros(nBins)},
#                        'BeamOn': {'B': np.zeros(nBins), 'S': np.zeros(nBins)}}
#               'Polya': 'BeamOff': {'B': np.zeros(nBins), 'S': np.zeros(nBins)},
#                        'BeamOn': {'B': np.zeros(nBins), 'S': np.zeros(nBins)}}
    
    speCharges = {'Time': [], 'Gauss': [], 'Polya': []}
     
    # For each day in the run folder read the HDF5 file and fit SPEQ spectra
    for day in daysInRun:
        h5In = h5py.File(runDir + '/' + day + '.h5', 'r+')

        # Get SPE charge fits for the current day
        speCharges['Time'] = h5In['/SPEQ/Times'][...]
        speCharges['Gauss'] = h5In['/SPEQ/GaussBest'][...][0]
        speCharges['Polya'] = h5In['/SPEQ/PolyaBest'][...][0]

        # For both signal and background window calculate the number of PE from charge for both gaussian and polya spe dists
        for wd in ['S','B']:

            # Get charge data and timestamps from the hdf5 file
            charge = h5In['/%s/charge'%wd][...]
            times = h5In['/%s/timestamp'%wd][...]
           
            # Variables for getting the correct SPEQ from fits based on the timestamp
            speQidx = 0
            speQidxMax = len(speCharges['Time'])
            notLastQidx = True

            npe = {'Gauss':[],'Polya':[]}

            # For each event get the timestamp and charge. Get the correct SPEQ and convert the charge to NPE
            for q,t in zip(charge,times):
                
                # Check whether the current SPEQ is still valid                
                if notLastQidx and t >= speCharges['Time'][speQidx + 1]:
                    speQidx += 1
                    if speQidx == speQidxMax:
                        notLastQidx = False

                # Convert charge to NPE
                npe['Gauss'].append(q/speCharges['Gauss'][speQidx])
                npe['Polya'].append(q/speCharges['Polya'][speQidx])
                
            if '/%s/npe-gauss'%wd in h5In:
                del h5In['/%s/npe-gauss'%wd]
            h5In.create_dataset('/%s/npe-gauss'%wd, data=npe['Gauss'])

             if '/%s/npe-polya'%wd in h5In:
                del h5In['/%s/npe-polya'%wd]
            h5In.create_dataset('/%s/npe-polya'%wd, data=npe['Polya'])
        h5In.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
















