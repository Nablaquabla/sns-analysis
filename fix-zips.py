import os
import time as tm
import sys
           
# Main function handling all internals
# -----------------------------------------------------------------------------
def main():

    # ===============================================================================================================
    #    Fix zip files for Run-16-02-29-11-54-20
    # ===============================================================================================================
    run = 'Run-16-02-29-11-54-20'
    times = {'160304': ['114020','230020'],
             '160308': ['003422','003022','003322','002022'],
             '160305': ['233422','233822','221622','232822','225622','220822','224722','232922','232222',
                        '232422','224422','231222','231322','231522','232122','224322','232722','234022',
                        '232522','221822','222022','221222','231622','224922','231422','220322','223822',
                        '231922','225722','231722','225522','220222','224022','235322','223522','224122'],
             '160306': ['000222','023022','023722','025222','003222','023522','024422','021022','010022','015622'],
             '160307': ['235322','234822']}


    for day in times.keys():         
        for t in times[day]:
            cFolder = '/home/bjs66/csi/sns_data/%s/%s'%(run,day)
            cZipFile = '%s/%s.zip'%(cFolder,t)
            cUnzippedFile = '%s/%s'%(cFolder,t)
            cmdUnzip = 'unzip -j %s -d %s'%(cZipFile,cFolder)
            cmdRmZip = 'rm %s'%cZipFile
            cmdZip = 'zip -j %s %s'%(cZipFile,cUnzippedFile)
            cmdRmUnzipped = 'rm %s'%cUnzippedFile
            os.system(cmdUnzip)
            os.system(cmdRmZip)
            os.system(cmdZip)
            os.system(cmdRmUnzipped)

           
if __name__ == '__main__':
    main()

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
