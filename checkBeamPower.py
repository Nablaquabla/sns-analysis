#!/home/bjs66/anaconda2/bin/python2.7

import os
import time as tm

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(run,day):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/CheckBP-%s-%s.condor'%(run,day),'w') as f:

        # Fixed program location'
        f.write('Executable = _checkBeamPower.py\n') #/home/bjs66/anaconda2/bin/python2.7\n')

        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"%s %s\"\n'%(run,day))

        # Standard cluster universe
        f.write('universe   = vanilla\n')
        f.write('getenv     = true\n')

        # Program needs at least 300 MB of free memory to hold unzipped data
        f.write('request_memory = 600\n')

        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = ../../Logs/CheckBP-%s-%s.log\n'%(run,day))
        f.write('Output = ../../Outs/CheckBP-%s-%s.out\n'%(run,day))
        f.write('Error = ../../Errs/CheckBP-%s-%s.err\n'%(run,day))

        # Do not write any emails
        f.write('notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')

        # Add single job to queue
        f.write('Queue')

# Main function handling all internals
# -----------------------------------------------------------------------------
def main():
    # Choose run to analyze
    runDirs = ['Run-15-06-25-12-53-44']
#    runDirs = ['Run-15-08-18-14-51-18','Run-15-08-31-00-23-36','Run-15-09-21-20-58-01','Run-15-09-23-21-16-00','Run-15-10-03-09-26-22','Run-15-10-13-13-27-09','Run-15-10-21-13-12-27','Run-15-10-29-15-56-36','Run-15-11-09-11-30-13']
#    runDirs = ['Run-15-06-25-12-53-44','Run-15-06-26-11-23-13','Run-15-07-31-18-30-14','Run-15-08-18-14-51-18',
#               'Run-15-08-31-00-23-36','Run-15-09-21-20-58-01','Run-15-09-23-21-16-00','Run-15-10-03-09-26-22',
#               'Run-15-10-13-13-27-09','Run-15-10-21-13-12-27','Run-15-10-29-15-56-36','Run-15-11-09-11-30-13']
#    runDirs = ['Run-15-11-20-11-34-48','Run-15-11-24-15-35-32','Run-15-12-14-11-21-45']
#    runDirs = ['Run-16-02-15-13-46-34']
#    runDirs = ['Run-16-02-29-11-54-20']
#    runDirs = ['Run-16-03-09-13-00-14']
#    runDirs = ['Run-16-03-22-18-09-33','Run-16-03-30-12-44-57']
#    runDirs = ['Run-16-04-12-11-54-27','Run-16-04-20-11-22-48','Run-16-05-05-14-08-52','Run-16-05-17-14-40-34','Run-16-06-02-12-35-56','Run-16-06-17-12-09-12']

    for run in runDirs:
        days = [x.split('.')[0] for x in os.listdir('/home/bjs66/csi/bjs-analysis/Processed/%s/'%run) if '.h5' in x]
        for d in days:
            if True:
                createCondorFile(run,d)
                cmd = 'condor_submit /home/bjs66/CondorFiles/CheckBP-%s-%s.condor'%(run,d)
                os.system(cmd)
                tm.sleep(1)

if __name__ == '__main__':
    main()



























