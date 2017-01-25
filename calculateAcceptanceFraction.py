#!/home/bjs66/anaconda2/bin/python2.7

import os
import time as tm

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(condorKey,cherenkov,minPEinPT,maxPEinPT,minRT050,maxRT050,minRT1090,maxRT1090):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/CalculateAcceptanceFraction-%s.condor'%condorKey,'w') as f:

        # Fixed program location'
        f.write('Executable = _calculateAcceptanceFraction.py\n') #/home/bjs66/anaconda2/bin/python2.7\n')

        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"%d %d %d %d %d %d %d\"\n'%(cherenkov,minPEinPT,maxPEinPT,minRT050,maxRT050,minRT1090,maxRT1090))

        # Standard cluster universe
        f.write('universe   = vanilla\n')
        f.write('getenv     = true\n')

        # Program needs at least 300 MB of free memory to hold unzipped data
        f.write('request_memory = 1000\n')

        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = /home/bjs66/Logs/CalculateAcceptanceFraction-%s.log\n'%condorKey)
        f.write('Output = /home/bjs66/Outs/CalculateAcceptanceFraction-%s.out\n'%condorKey)
        f.write('Error = /home/bjs66/Errs/CalculateAcceptanceFraction-%s.err\n'%condorKey)

        # Do not write any emails
        f.write('notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')

        # Add single job to queue
        f.write('Queue')

# Main function handling all internals
# -----------------------------------------------------------------------------
def main():
    # Ba runs to analyze.
        # Ba analysis
#    runDirs = ['Run-15-03-27-12-42-26','Run-15-03-30-13-33-05','Run-15-04-08-11-38-28',
#               'Run-15-04-17-16-56-59','Run-15-04-29-16-34-44','Run-15-05-05-16-09-12',
#               'Run-15-05-11-11-46-30','Run-15-05-19-17-04-44','Run-15-05-27-11-13-46'] 

    for cherenkov in [4,5,6]:
        for minPEinPT in [5]:
            for maxPEinPT in [15,20,30]:
                for minRT050 in [0,50]:
                    for maxRT050 in [1250,1500]:
                        for minRT1090 in [0,125]:
                            for maxRT1090 in [1375,1500]:
                                condorKey = '%d-%d-%d-%d-%d-%d-%d'%(cherenkov,minPEinPT,maxPEinPT,minRT050,maxRT050,minRT1090,maxRT1090) 
                                createCondorFile(condorKey,cherenkov,minPEinPT,maxPEinPT,minRT050,maxRT050,minRT1090,maxRT1090)
                                cmd = 'condor_submit /home/bjs66/CondorFiles/CalculateAcceptanceFraction-%s.condor'%condorKey
                                os.system(cmd)
                                tm.sleep(1)

if __name__ == '__main__':
    main()



























