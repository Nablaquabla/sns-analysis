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
import datetime
import pytz

# Define timezones used in analysis
eastern = pytz.timezone('US/Eastern')
utc = pytz.utc
epochBeginning = utc.localize(datetime.datetime(1970,1,1))


#=============================================================================#
#                           Define fit functions
#=============================================================================#
def polya(x,q,t,m):
    return (1.0*x/q)**(m*(t+1.0)-1.0) * np.exp(-(t+1.0)*x/q)

def gauss(x,q,s,m):
    return np.exp(-0.5*((x-m*q)/s)**2/m)

def gFit(x,p):
    _t1 = p[2] * gauss(x,p[0],p[1],1)
    _tbg = p[3] * np.exp(-x/p[4])
    return _t1 + _tbg

def pFit(x,p):
    _t1 = p[2] * polya(x,p[0],p[1],1)
    _tbg = p[3] * np.exp(-x/p[4])
    return _t1 + _tbg

def gFit3(x,p):
    _t1 = p[2] * gauss(x,p[0],p[1],1)
    _t2 = p[3] * gauss(x,p[0],p[1],2)
    _t3 = p[4] * gauss(x,p[0],p[1],3)
    _tn = p[5] * np.exp(-x/p[6]) + p[7]
    return _t1 + _t2 + _t3 + _tn

def pFit3(x,p):
    _t1 = p[2] * polya(x,p[0],p[1],1)
    _t2 = p[3] * polya(x,p[0],p[1],2)
    _t3 = p[4] * polya(x,p[0],p[1],3)
    _tn = p[5] * np.exp(-x/p[6]) + p[7]
    return _t1 + _t2 + _t3 + _tn

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

    # Create dummy pars variables
    parsG = np.zeros((3,8))
    parsP = np.zeros((3,8))

    # For each day in the run folder read the HDF5 file and fit SPEQ spectra
    for day in daysInRun:
        h5In = h5py.File(runDir + '/' + day + '.h5', 'r+')
        
        # Prepare/reset output array
        speQArr = {'Times': [],
                   'GaussBest': [],
                   'GaussErr': [],
                   'PolyaBest': [],
                   'PolyaErr': []}

        # Perform analysis for each hour in day file
        for time in np.sort(h5In['/I/'].keys()):

            # Calculate seconds in epoch based on day and hour
            eTS = eastern.localize(datetime.datetime.strptime(day+time,'%y%m%d%H%M%S'))
            uTS = eTS.astimezone(utc)
            sSE = (uTS - epochBeginning).total_seconds()

            speQArr['Times'].append(sSE)

            # Prepare fit data
            xQ = np.arange(-50,249)
            yQ = h5In['/I/%s/speChargeDist'%time][...]

            # In 1 hour of data there are approximately 10^6 spe events. So if there are less than 10^5 use previous data point
            if np.sum(yQ) > 1e5:
                p0 = [60.0, 5.0, 0.85*np.max(yQ), 0.1*np.max(yQ), 0.05*np.max(yQ), 0.1*np.max(yQ), 10.0, 0]
                lims = [[2,1,0,0,0],[3,1,0,0,0],[4,1,0,0,0],[5,1,0,0,0]]

                # Find plateau before the SPE peak
                xMin = np.argmax(np.diff(yQ)) - 50 + 10
                c = xQ >= xMin

                # Fit gaussian SPE charge distribution
                x2G,parsG,xfitG,yfitG = ef.arbFit(gFit3,xQ[c],yQ[c],'Poisson',p0,lims)
                speQArr['GaussBest'].append(parsG[0][:])
                speQArr['GaussErr'].append(parsG[2][:])

                # Fit polya SPE charge distribution
                x2P,parsP,xfitP,yfitP = ef.arbFit(pFit3,xQ[c],yQ[c],'Poisson',p0,lims)
                speQArr['PolyaBest'].append(parsP[0][:])
                speQArr['PolyaErr'].append(parsP[2][:])

            else:
                speQArr['GaussBest'].append(parsG[0][:])
                speQArr['GaussErr'].append(parsG[2][:])
                speQArr['PolyaBest'].append(parsP[0][:])
                speQArr['PolyaErr'].append(parsP[2][:])

        # Write data to HDF5 file replacing already existing data
        for key in speQArr:
            if '/SPEQ/%s'%key in h5In:
                del h5In['/SPEQ/%s'%key]
            h5In.create_dataset('/SPEQ/%s'%key,data=speQArr[key])

        h5In.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
















