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

    # Prepare output file
    h5Out = h5py.File(mainDir + 'Spectra/' + run + '-Uncut.h5','a')

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

            # Beam on (True) and off (False) status
            beamPowerStatus = h5In['/%s/beamOn'%wd][...]

            # Split peaks in pretrace data set into beam on and off subsets
            peInPT = {}
            _t_peInPT = h5In['/%s/pe-pt'%wd][...]
            peInPT['BeamOn'] = _t_peInPT[beamPowerStatus]
            peInPT['BeamOff'] = _t_peInPT[np.logical_not(beamPowerStatus)]

            # Get and split charges
            charge = {}
            _t_charge = h5In['/%s/charge'%wd][...]
            charge['BeamOn'] = _t_charge[beamPowerStatus]
            charge['BeamOff'] = _t_charge[np.logical_not(beamPowerStatus)]

            # Get and split time windows
            speWindow = {}
            _t_speWindow = h5In['/%s/speQindex'][...]
            speWindow['BeamOn'] = _t_speWindow[beamPowerStatus]
            speWindow['BeamOff'] = _t_speWindow[np.logical_not(beamPowerStatus)]

            # Maximum number of PE in spectrum
            nBins = 200

            # For each hour
            for i in range(len(speCharges['time'])):

                # Get beam on and off data sets
                for bP in ['BeamOn','BeamOff']:

                    # Determine which data actually was taken during beam on/off periods
                    cutTime = (speWindow[bP] == i)

                    # Select the maximum number of peaks in the pretrace
                    for pePT in [4,3,2,1,0]:

                        # And apply the pretrace cut
                        cutPEPT = (peInPT[bP] <= pePT)

                        # Combine pretrace and time cut such that only events within the correct time period are taken into account
                        totalCut = cutTime * cutPEPT

                        # Convert the collected charge using the current mean spe charge into number of photoelectrons
                        spectrumGauss = np.histogram(charge[bP][totalCut]/speCharges['gauss'][i],nBins,[-0.5,nBins-0.5])[0]
                        spectrumPolya = np.histogram(charge[bP][totalCut]/speCharges['polya'][i],nBins,[-0.5,nBins-0.5])[0]

                    h5Out.create_dataset('/%s/PEMax-%s/%s/%s/%s'%(bP,pePT,speCharges['time'][i],wd,'gauss'),data=spectrumGauss)
                    h5Out.create_dataset('/%s/PEMax-%s/%s/%s/%s'%(bP,pePT,speCharges['time'][i],wd,'polya'),data=spectrumPolya)

        h5In.close()
    h5Out.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
















