#!/home/bjs66/anaconda2/bin/python2.7

import os
import time as tm

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(run):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/GetBaSpectra-%s.condor'%run,'w') as f:

        # Fixed program location'
        f.write('Executable = _getBaSpectra.py\n') #/home/bjs66/anaconda2/bin/python2.7\n')

        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"%s\"\n'%run)

        # Standard cluster universe
        f.write('universe   = vanilla\n')
        f.write('getenv     = true\n')

        # Program needs at least 300 MB of free memory to hold unzipped data
        f.write('request_memory = 1000\n')

        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = /home/bjs66/Logs/GetBaSpectra-%s.log\n'%run)
        f.write('Output = /home/bjs66/Outs/GetBaSpectra-%s.out\n'%run)
        f.write('Error = /home/bjs66/Errs/GetBaSpectra-%s.err\n'%run)

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
    runDirs = ['Run-15-03-30-13-33-05','Run-15-04-08-11-38-28',
               'Run-15-04-17-16-56-59','Run-15-04-29-16-34-44','Run-15-05-05-16-09-12',
               'Run-15-05-11-11-46-30','Run-15-05-19-17-04-44','Run-15-05-27-11-13-46'] 
#
#    runDirs = ['Run-15-03-27-12-42-26','Run-15-03-30-13-33-05','Run-15-04-08-11-38-28',
#               'Run-15-04-17-16-56-59','Run-15-04-29-16-34-44','Run-15-05-05-16-09-12',
#               'Run-15-05-11-11-46-30','Run-15-05-19-17-04-44','Run-15-05-27-11-13-46'] 
#    runDirs = ['Run-15-03-27-12-42-26']
 
    for run in runDirs:
        createCondorFile(run)
        cmd = 'condor_submit /home/bjs66/CondorFiles/GetBaSpectra-%s.condor'%run
        os.system(cmd)
        tm.sleep(1)

if __name__ == '__main__':
    main()



























