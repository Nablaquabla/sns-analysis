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
    bPF = h5py.File('/home/bjs66/csi/bjs-analysis/BeamPowerHistory/BeamPowerHistory.h5','r')
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
    beam_was_on = (len(timeData) > 0)

    # Create hdf5 file
    mainDir = '/home/bjs66/csi/bjs-analysis/Processed/'

    # Open HDF5 file
    f = h5py.File(mainDir + run + '/' + d + '.h5', 'r+')

    # For both signal and background window go through all events and tag those that happend during a beam on period
    for wd in ['S','B']:

        # If there has already been a beam power check delete the previous data set and replace it with the new one
        if '/%s/beamOn'%wd in f:
            del f['/%s/beamOn'%wd]

        # Array to store beam-on flags
        beamOnArray = []

        # Get all timestamps and truncate them to the second
        evtTS = f['/%s/timestamp'%wd][...].astype(np.uint64)
        timeCut = (timeData>=evtTS[0]) * (timeData<=evtTS[-1])
        timesWithBeam = timeData[timeCut]

        # Get timestamp for each event and see if there was beam
        idx = 0
        skip = False
        if len(timesWithBeam) == 0:
            skip = True
        for et in evtTS:
                if beam_was_on:
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
                else:
                    beamOnArray.append(False)

        # Write beam on flag to HDF5 file
        f.create_dataset('/%s/beamOn'%wd,data=beamOnArray,dtype=bool)
    f.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
