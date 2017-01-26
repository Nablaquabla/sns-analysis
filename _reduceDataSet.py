#!/home/bjs66/anaconda2/bin/python

import h5py
import numpy as np
import os
import sys

sbKeys = ['charge','arrival','pe-pt','pe-iw','speQindex','rt10','rt50','rt90',
          'beamOn','muon1','timestamp']
speqKeys = ['GaussBest','PolyaBest']

def main(args):
    run = args[1]

    # Get all days in input run dir
    h5DataFiles = [x for x in os.listdir('/home/bjs66/csi/bjs-analysis/Processed/%s'%run) if '.h5' in x]
    
    # Read each day file and process the hourly data
    for h5f in h5DataFiles:
        day = h5f.split('.')[0]
        h5Out = h5py.File('/home/bjs66/csi/bjs-analysis/Processed/ReducedData/%s/%s-ReducedData.h5'%(run,day),'w')
        h5In = h5py.File('/home/bjs66/csi/bjs-analysis/Processed/%s/%s'%(run,h5f),'r')
        
        speqG = h5In['SPEQ/GaussBest'][...][:,0]
        speqP = h5In['SPEQ/PolyaBest'][...][:,0]
        speqTimes = h5In['SPEQ/Times'][...]

        for wd in ['S','B']:
            # Convert charge to number if photoelectrons in ROI
            speQindex = h5In['/%s/speQindex'%wd][...]
            charge = h5In['/%s/charge'%wd][...]
            npe = 1.0*charge/speqG[speQindex]
    
            # Get number of peaks in the ROI
            piw = h5In['/%s/pe-iw'%wd][...]
    
            # Get potential index for second muon
            mu = h5In['/%s/muon2'%wd][...]

            # There are less than 40 NPE from charge integration in the ROI
            cutNPE = (npe < 40)

            # There are less than 4 peaks in the ROI
            cutPIW = (piw < 4)

            # There are more than one muon in the trace
            cutMuon = (mu != -1)

            # Full cut:
            # Cut everything with more than one muon
            # Cut all events with less than 4 peaks in IW iff there are less than 40 NPE from charge integration in IW
            cutTotal = np.logical_not(np.asarray(cutMuon + cutNPE * cutPIW,dtype=bool))

            # Copy cut signal and background data to new HDF5 file
            for data in sbKeys:
                h5Out.create_dataset('/%s/%s'%(wd,data),data=h5In['/%s/%s'%(wd,data)][...][cutTotal])
        h5Out.create_dataset('/speq/gauss',data=speqG)
        h5Out.create_dataset('/speq/polya',data=speqP)
        h5Out.create_dataset('/speq/times',data=speqTimes)
        # Properly close hdf5 files
        h5Out.close()
        h5In.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)




