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
import easyfit as ef

# Keys available: B,S,I/time/key
# B,S:
# 'timestamp','bl-csi','bl-mv','pe-pt','pe-roi','pe-iw','arrival','charge',
# 'rt10','rt50','rt90','muon1','muon2','muon3','pe-total'
# I:
# waveformsProcessed, linearGates, overflows, bWindowsAnalyzed, sWindowsAnalyzed,
# speChargeDist, bPeaksPTDist, sPeaksPTDist, csiBaselineDist, csiBaseline, peakWidthDist
# peakAmplitudeDist, bigPulseOnset, bigPulseCharge, muonHits, chargeDist2d

def main(args):
    #run = args[1]

    # Create hdf5 file
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed/Stability/'
    
    
# ============================================================================
#                                Run program
# ============================================================================        
if __name__ == '__main__':
    main(sys.argv)

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
