#!/home/bjs66/anaconda2/bin/python

import sys
import h5py
import datetime
import os

def main(argv):
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed'
    run = argv[1]
    dayToAnalyze = argv[2]
    # ============================================================================
    #                       Full timestamp conversion
    # ============================================================================
    t0 = datetime.datetime(2015,06,25,0,0,0)
    t1 = datetime.datetime.strptime(dayToAnalyze, '%y%m%d')
    dt = (t1 - t0).total_seconds()

    days = [x for x in os.listdir('%s/%s'%(mainDir,run)) if '.h5' in x]
    
    print '\nStarting conversion now ...'
    
    for d in days:
        f = h5py.File('%s/%s/%s'%(mainDir,run,d))
        for wd in ['B','S']:
            tS = f['%s/timestamp'%wd][...] + dt
            del f['%s/timestamp'%wd]
            f.create_dataset('%s/timestamp'%wd,data=tS)
                
if __name__ == '__main__':
    main(sys.argv)
