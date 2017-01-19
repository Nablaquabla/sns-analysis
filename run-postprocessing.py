#!/home/bjs66/anaconda2/bin/python2.7

import os
import time as tm
import sys

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(program,outLabel,run):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/%s-%s.condor'%(outLabel,run),'w') as f:

        # Fixed program location'
        f.write('Executable = %s.py\n'%program)

        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"%s\"\n'%run)

        # Standard cluster universe
        f.write('universe   = vanilla\n')
        f.write('getenv     = true\n')

        # Program needs at least 300 MB of free memory to hold unzipped data
        f.write('request_memory = 300\n')

        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = /home/bjs66/Logs/%s-%s.log\n'%(outLabel,run))
        f.write('Output = /home/bjs66/Outs/%s-%s.out\n'%(outLabel,run))
        f.write('Error = /home/bjs66/Errs/%s-%s.err\n'%(outLabel,run))

        # Do not write any emails
        f.write('notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')

        # Add single job to queue
        f.write('Queue')

# Main function handling all internals
# -----------------------------------------------------------------------------
def main(prog):
    # Choose run to analyze
#    runDirs = ['Run-15-06-25-12-53-44']
#    runDirs = ['Run-15-06-25-12-53-44','Run-15-06-26-11-23-13','Run-15-07-31-18-30-14']
#    runDirs = ['Run-15-08-18-14-51-18','Run-15-08-31-00-23-36','Run-15-09-21-20-58-01']
#    runDirs = ['Run-15-09-23-21-16-00','Run-15-10-03-09-26-22','Run-15-10-13-13-27-09']
#    runDirs = ['Run-15-10-21-13-12-27','Run-15-10-29-15-56-36','Run-15-11-09-11-30-13']
#    runDirs = ['Run-15-11-20-11-34-48','Run-15-11-24-15-35-32','Run-15-12-14-11-21-45']
#    runDirs = ['Run-15-12-26-08-30-40','Run-16-01-07-12-16-36','Run-16-02-02-16-26-26']
#    runDirs = ['Run-16-02-15-13-46-34','Run-16-02-29-11-54-20','Run-16-03-09-13-00-14']
#    runDirs = ['Run-16-03-22-18-09-33','Run-16-03-30-12-44-57','Run-16-04-12-11-54-27']
#    runDirs = ['Run-16-04-20-11-22-48','Run-16-05-05-14-08-52','Run-16-05-12-14-07-59']
#    runDirs = ['Run-16-05-17-14-40-34','Run-16-06-02-12-35-56','Run-16-06-17-12-09-12']
#    runDirs = ['Run-16-06-27-17-50-08','Run-16-07-06-18-25-19','Run-16-07-12-11-44-55']
#    runDirs = ['Run-16-07-18-11-50-24','Run-16-07-21-11-59-39','Run-16-07-28-12-49-17']

    runDirs = [x for x in os.listdir('/home/bjs66/csi/bjs-analysis/Processed/') if 'Run' in x]
    progDict = {'convert': '_convertData',
                'stability': '_calculateStabilityData',
                'speCharge': '_calculateSPEQ',
                'reduce': '_reduceDataSet'}
    if prog not in progDict.keys() and prog != 'move':
        return

    if prog != 'move':
        pyProg = progDict[prog]
        for run in runDirs:
            createCondorFile(pyProg,prog,run)
            cmd = 'condor_submit /home/bjs66/CondorFiles/%s-%s.condor'%(prog,run)
#            print cmd
            os.system(cmd)
            tm.sleep(1)
    else:
        for run in runDirs:
            cmd = 'mkdir /home/bjs66/csi/bjs-analysis/Processed/%s'%run
            os.system(cmd)
#            print cmd
            tm.sleep(1)

            cmd = 'mv /home/bjs66/csi/bjs-analysis/%s/*.h5 /home/bjs66/csi/bjs-analysis/Processed/%s/'%(run,run)
            os.system(cmd)
#            print cmd
            tm.sleep(1)


if __name__ == '__main__':
    main(sys.argv[1])



























