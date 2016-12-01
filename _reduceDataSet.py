#!/home/bjs66/anaconda2/bin/python

import h5py
import numpy as np
import os
import sys

def main(args):
    run = args[1]

    # Determine day files corresponding to the run
    h5DataFiles = [x for x in os.listdir('./%s'%run) if '.h5' in x]  
    
    # Read each day file and process the hourly data
    for h5f in h5DataFiles:
        day = h5f.split('.')[0]
        h5Out = h5py.File('./Processed/%s/%s-ReducedData.h5'%day,'w')
        f = h5py.File('./%s/%s'%(run,h5f),'r')
        for wd in ['S','B']:
            piw = f['/%s/pe-iw'%wd][...]
            ciw = piw >= 5
            mu = f['/%s/muon2'%wd][...]
            cmu = mu == -1
            cut = ciw * cmu

            for data in ['timestamp','pe-pt','pe-iw','pe-roi','rt10','rt50','rt90','muon1','arrival','charge','pe-total']:
                h5Out.create_dataset('/%s/%s'%(wd,data),data=f['/%s/%s'%(wd,data)][...][cut])
            for time in np.sort(h5Out['/I'].keys()):
                h5Out.create_dataset('/I/BigPulses',data=f['/I/%s/bigPulseCharge'%time][...])

        # Properly close hdf5 file
        h5Out.close()
    
# ============================================================================
#                                Run program
# ============================================================================        
if __name__ == '__main__':
    main(sys.argv)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  