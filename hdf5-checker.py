#!/home/bjs66/anaconda2/bin/python

import h5py
import sys
import os
import numpy as np

def main(args):
    run = args[1]
    runDir = '/home/bjs66/csi/bjs-analysis/Processed/' + args[1]
    daysInRun = np.sort([x for x in os.listdir(runDir) if '.h5' in x])

    print '\n------------------------------------'
    print 'Working on', run
    print '------------------------------------'

    print 'Day\t\tPower\tSPEQ\tWindow'
    print '---------\t-----\t----\t------'

    for day in daysInRun:
        f = h5py.File(runDir + '/' + day, 'r')
        power = bool(('/B/beamOn' in f) * ('/S/beamOn' in f))
        speq = ('SPEQ' in f)
        window = bool(('/B/speQindex' in f) * ('/S/speQindex' in f))
        print '%s\t%s\t%s\t%s'%(day,power,speq,window)
        f.close()

if __name__ == "__main__":
    main(sys.argv)
