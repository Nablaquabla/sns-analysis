#!/home/bjs66/anaconda2/bin/python
"""
Created on Mon Feb 01 15:03:56 2016

@author: Nablaquabla
"""
import h5py
import numpy as np
import easyfit as ef
import sys

#=============================================================================#
#                           Define fit functions
#=============================================================================#
# Single Polya and flat offset
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
    _tn = p[5] * np.exp(-x/p[7]) + p[8]
    return _t1 + _t2 + _t3 + _tn

def pFit3(x,p):
    _t1 = p[2] * polya(x,p[0],p[1],1)
    _t2 = p[3] * polya(x,p[0],p[1],2)
    _t3 = p[4] * polya(x,p[0],p[1],3)
    _tn = p[5] * np.exp(-x/p[7]) + p[8]
    return _t1 + _t2 + _t3 + _tn

# ============================================================================
#                                Run program
# ============================================================================
def main(args):
    run = args[1]
    h5In = h5py.File('/home/bjs66/csi/bjs-analysis/Processed/Stability/%s.h5'%run,'r+')
    speQDists = h5In['/speChargeDist'][...].T

    xQ = np.arange(-50,249)
    gaussFits = {'Best': [], 'Error': [], 'Width': []}
    polyaFits = {'Best': [], 'Error': [], 'Width': []}
    lims = [[2,1,0,0,0],[3,1,0,0,0],[4,1,0,0,0],[5,1,0,0,0]]

    for i,speQ in enumerate(speQDists):
        xMin = np.argmax(np.diff(speQ)) - 50 + 10

        yQ = speQ
        c = xQ >= xMin

        x2,pars,xfit,yfit = ef.arbFit(gFit3,xQ[c],yQ[c],'Poisson',[60.0,5.0,0.85*np.max(yQ),0.1*np.max(yQ),0.05*np.max(yQ),0.1*np.max(yQ),0,5.0,0],lims)
        gaussFits['Best'].append(pars[0][0])
        gaussFits['Error'].append(pars[2][0])
        gaussFits['Width'].append(pars[0][1])

        x2,pars,xfit,yfit = ef.arbFit(pFit3,xQ[c],yQ[c],'Poisson',[60.0,5.0,0.85*np.max(yQ),0.1*np.max(yQ),0.05*np.max(yQ),0.1*np.max(yQ),0,5.0,0],lims)
        polyaFits['Best'].append(pars[0][0])
        polyaFits['Error'].append(pars[2][0])
        polyaFits['Width'].append(pars[0][1])

    
    for _type in ['Best','Error','Width']:
        if '/fittedSPECharges/gauss/%s'%_type in h5In:
            del h5In['/fittedSPECharges/gauss/%s'%_type]
        if '/fittedSPECharges/polya/%s'%_type in h5In:
            del h5In['/fittedSPECharges/polya/%s'%_type]

    for _type in ['Best','Error','Width']:
        h5In.create_dataset('/fittedSPECharges/gauss/%s'%_type,data=gaussFits[_type])
        h5In.create_dataset('/fittedSPECharges/polya/%s'%_type,data=polyaFits[_type])
    h5In.close()

# ============================================================================
#                                Run program
# ============================================================================
if __name__ == '__main__':
    main(sys.argv)
















