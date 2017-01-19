#!/home/bjs66/anaconda2/bin/python

import h5py
import numpy as np
import os
import sys
import datetime

def main(argv):

    run = argv[1]
    d = argv[2]

    # Get beam power file and prepare keys for data readout
    bPF = h5py.File('/home/bjs66/csi/bjs-analysis/BeamPowerHistory/beamPowerHistory.h5','r')
#    bPF = h5py.File('F:/Work-Data-Storage/BeamPower/BeamPowerHistory.h5','r')
    dayTS = datetime.datetime(2000 + int(d[:2]), int(d[2:4]), int(d[4:6]))
    dayTSArray = [dayTS + datetime.timedelta(days=-1), dayTS, dayTS + datetime.timedelta(days=1)]
    dayKeys = [x.strftime('%Y%m%d') for x in dayTSArray]

    # Read power data for current day +/- one day
    timeData = np.array([0])
    powerData = np.array([0])
    for dk in dayKeys:
        timeData = np.concatenate((timeData,bPF['/%s/time'%dk][...]))
        powerData = np.concatenate((powerData,bPF['/%s/power'%dk][...]))

    # Remove timestamps with zero and almost zero power
    powerCut = (powerData >= 5e-5)
    timeData = timeData[powerCut]

    # Create hdf5 file
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed/'
#    mainDir = 'F:/Work-Data-Storage/CsI/Condor/SNS/Testdata/'

    # Open HDF5 file
    f = h5py.File(mainDir + run + '/' + d + '.h5', 'r+')

    # For both signal and background window go through all events and tag those that happend during a beam on period
    for wd in ['S','B']:
        # Array to store beam-on flags
        beamOnArray = []

        # Get all timestamps and truncate them to the second
        evtTS = f['/%s/timestamp'%wd][...].astype(np.uint64)
        timeCut = (timeData>=evtTS[0]) * (timeData<=evtTS[-1])
        timesWithBeam = timeData[timeCut]

        # Get timestamp for each event and see if there was beam
        idx = 0
        skip = False
        for et in evtTS:
                while True:
                    if skip:
                        beamOnArray.append(False)
                        break
                    elif et == timesWithBeam[idx]:
                        beamOnArray.append(True)
                        break
                    elif et > timesWithBeam[idx]:
                        idx += 1
                        if idx >= len(timesWithBeam):
                            skip = True
                        continue
                    elif et < timesWithBeam[idx]:
                        beamOnArray.append(False)
                        break

        # Write beam on flag to HDF5 file
        f.create_dataset('/%s/beamOn'%wd,data=beamOnArray,dtype=bool)
    f.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
#    main(['','Run-15-06-25-12-53-44','150626'])




