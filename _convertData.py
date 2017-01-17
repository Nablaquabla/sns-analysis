#!/home/bjs66/anaconda2/bin/python

import sys
import pytz
import h5py
import numpy as np
import datetime
import os

# ============================================================================
#                       Full timestamp conversion
# ============================================================================
# Prepare timezones and beginning of epcoch for later use
utc = pytz.utc
eastern = pytz.timezone('US/Eastern')
epochBeginning = utc.localize(datetime.datetime(1970,1,1))

# Convert string to eastern timestamp, convert to UTC timestamp, convert to seconds in epoch
def convertTimestamp(x):
    eastern_ts = eastern.localize(datetime.datetime.strptime(str(np.uint64(x)), '%y%m%d%H%M%S%f'))
    utc_ts = eastern_ts.astimezone(utc)
    sSE = (utc_ts - epochBeginning).total_seconds()
    return sSE

# Vectorize timestamp conversion for faster processing
ct = np.vectorize(convertTimestamp)


def main(argv):
    mainDir = '/home/bjs66/csi/bjs-analysis'
    run = argv[1]
    d = argv[2]

    # Check that day analyzed is a normal day, i.e. no DST changes - Not necessary right now,
    # as there was no data being recorded during the fall change. And this is the only time
    # with ambiguous timestamps

#    tNow = datetime.datetime(2000 + int(d[:2]), int(d[2:4]), int(d[4:6]))
#    easternNow = eastern.localize(tNow)
#    easternTomorrow = eastern.localize(tNow + datetime.timedelta(days=1))
#    elapsedSeconds = easternTomorrow - easternNow
#    noDSTChange = (elapsedSeconds == 86400)


    # Add dataset / group to the hdf5 file
    keys = ['timestamp','bl-csi','bl-mv','pe-pt','pe-roi','pe-iw','arrival','charge','ll-real','ll-flat','rt10','rt50','rt90','muon1','muon2','muon3','pe-total']

    # What data type is necessary to save the information provided in the csv files. Mainly used to save some storage space
    datatypes = {'timestamp': np.dtype(np.float),
                 'bl-csi': np.dtype(np.int16),
                 'bl-mv': np.dtype(np.int16),
                 'pe-pt': np.dtype(np.uint16),
                 'pe-roi': np.dtype(np.uint16),
                 'pe-iw': np.dtype(np.uint16),
                 'arrival': np.dtype(np.uint32),
                 'charge': np.dtype(np.uint32),
                 'll-real': np.dtype(np.float),
                 'll-flat': np.dtype(np.float),
                 'rt10': np.dtype(np.float),
                 'rt50': np.dtype(np.float),
                 'rt90': np.dtype(np.float),
                 'muon1': np.dtype(np.int32),
                 'muon2': np.dtype(np.int32),
                 'muon3': np.dtype(np.int32),
                 'pe-total': np.dtype(np.int32),}

#    days = [x for x in os.listdir('%s/%s'%(mainDir,run)) if '.h5' not in x]

#    print '\nStarting conversion now ...'

    f = h5py.File('%s/%s/%s.h5'%(mainDir,run,d),'w')

    times = [x.split('-')[1] for x in os.listdir('%s/%s/%s'%(mainDir,run,d)) if 'B-' in x]
    times = np.sort(np.asarray(times))
#    print '%i files found for day %s'%(len(times),day)

    # ==== Process info files ====
    # Determine hour long sub-times for each day. If there are less than
    # half an hour at the end of the day add it to the previous bin
    i = 0
    subTimeArray = []
    while True:
        if (len(times) - (i+1)*60) >= 30:
            _subT = times[i*60:(i+1)*60]
            subTimeArray.append(_subT)
            if (i+1)*60 > len(times): break
            i += 1
        else:
            _subT = times[i*60:]
            subTimeArray.append(_subT)
            break
    subTimeArray = np.asarray(subTimeArray)

    # For each sub-time merge all distribution data and concatenate the
    # important numbers
    for sTA in subTimeArray:
        wfProcessed = []
        linGates = []
        overflows = []
        muonVetos = []
        bWinAnalyzed = []
        sWinAnalyzed = []
        csiBaseline = []
        muon_hits = np.array([])
        bP_charge = np.array([])
        bP_onset = np.array([])
        yspe = np.zeros(300)
        ppt_bg = np.zeros(52)
        ppt_s = np.zeros(52)
        csi_bl = np.zeros(32)
        charge_dist = np.zeros((350,350))
        peak_width_distribution = np.zeros(51)
        peak_amplitude_distribution = np.zeros(100)

        # Read data from file...
        for t in sTA:
            if os.stat('%s/%s/%s/%s-%s'%(mainDir,run,d,'I',t)) > 0 and os.path.getsize('%s/%s/%s/%s-%s'%(mainDir,run,d,'S',t)) > 0:
                with open('%s/%s/%s/%s-%s'%(mainDir,run,d,'I',t),'r') as fIn:
                    current_charge = 0
                    read_charge_dist = False
                    for idx,line in enumerate(fIn):
                        if idx == 1:
                            wfProcessed.append(int(line))
                        elif idx == 3:
                            linGates.append(int(line))
                        elif idx == 5:
                            overflows.append(int(line))
                        elif idx == 7:
                            muonVetos.append(int(line))
                        elif idx == 9:
                            bWinAnalyzed.append(int(line))
                        elif idx == 11:
                            sWinAnalyzed.append(int(line))
                        elif idx == 33:
                            yspe = yspe + np.asarray(line.split(),dtype=int)
                        elif idx == 35:
                            ppt_bg = ppt_bg + np.asarray(line.split(),dtype=int)
                        elif idx == 37:
                            ppt_s = ppt_s + np.asarray(line.split(),dtype=int)
                        elif idx == 39:
                            csi_bl = csi_bl + np.asarray(line.split(),dtype=int)
                            try:
                                csiBaseline.append(np.average(np.arange(85,117),weights=(np.asarray(line.split(),dtype=int))))
                            except:
                                print t
                        elif idx == 41:
                            peak_width_distribution = peak_width_distribution + np.asarray(line.split(),dtype=int)
                        elif idx == 43:
                            peak_amplitude_distribution = peak_amplitude_distribution + np.asarray(line.split(),dtype=int)
                        elif idx == 45:
                            bP_onset = np.concatenate((bP_onset,np.asarray(line.split(),dtype=int)))
                        elif idx == 47:
                            bP_charge = np.concatenate((bP_charge,np.asarray(line.split(),dtype=int)))
                        elif idx == 49:
                            muon_hits = np.concatenate((muon_hits,np.asarray(line.split(),dtype=int)))
                        elif idx >= 50:
                            if 'Charge distribution in full waveform' in line:
                                read_charge_dist = True
                            elif read_charge_dist:
                                _td = np.asarray(line.split(),dtype=int)
                                current_charge =  _td[0]
                                non_zero_locations = _td[1::2]
                                non_zero_value = _td[2::2]
                                charge_dist[current_charge][non_zero_locations] = charge_dist[current_charge][non_zero_locations] + non_zero_value
        if len(wfProcessed) > 0:
            f.create_dataset('/I/%s/waveformsProcessed'%sTA[0],data=wfProcessed)
            f.create_dataset('/I/%s/linearGates'%sTA[0],data=linGates)
            f.create_dataset('/I/%s/overflows'%sTA[0],data=overflows)
            f.create_dataset('/I/%s/bWindowsAnalyzed'%sTA[0],data=bWinAnalyzed)
            f.create_dataset('/I/%s/sWindowsAnalyzed'%sTA[0],data=sWinAnalyzed)
            f.create_dataset('/I/%s/speChargeDist'%sTA[0],data=yspe)
            f.create_dataset('/I/%s/bPeaksPTDist'%sTA[0],data=ppt_bg)
            f.create_dataset('/I/%s/sPeaksPTDist'%sTA[0],data=ppt_s)
            f.create_dataset('/I/%s/csiBaselineDist'%sTA[0],data=csi_bl)
            f.create_dataset('/I/%s/csiBaseline'%sTA[0],data=csiBaseline)
            f.create_dataset('/I/%s/peakWidthDist'%sTA[0],data=peak_width_distribution)
            f.create_dataset('/I/%s/peakAmplitudeDist'%sTA[0],data=peak_amplitude_distribution)
            f.create_dataset('/I/%s/bigPulseOnset'%sTA[0],data=bP_onset)
            f.create_dataset('/I/%s/bigPulseCharge'%sTA[0],data=bP_charge)
            f.create_dataset('/I/%s/muonHits'%sTA[0],data=muon_hits)
            f.create_dataset('/I/%s/chargeDist2d'%sTA[0],data=charge_dist)


    for w in ['B','S']:
        create_Dataset = True
        i0 = 0
        dset = {}
        f.create_group(w)
        for idx,t in enumerate(times):
            try:
                if os.path.getsize('%s/%s/%s/%s-%s'%(mainDir,run,d,w,t)) > 0:
                    _tdata = np.loadtxt('%s/%s/%s/%s-%s'%(mainDir,run,d,w,t)).T
                    if not np.isscalar(_tdata[0]):
                        # Convert timestamps to seconds in epoch in UTC!
                        _tdata[0] = ct(_tdata[0])
                        if create_Dataset:
                            create_Dataset = False
                            for i,k in enumerate(keys):
                                dset[k] = f.create_dataset('/%s/%s'%(w,k),data=_tdata[i],dtype=datatypes[k],maxshape=(None,))
                        else:
                            for i,k in enumerate(keys):
                                dset[k].resize((i0 + len(_tdata[i]),))
                                dset[k][i0:] = _tdata[i]
                        i0 += len(_tdata[0])
                        del _tdata
                else:
                    continue
            except OSError:
                 continue
    f.close()
#    print 'Done!'

if __name__ == '__main__':
    main(sys.argv)
