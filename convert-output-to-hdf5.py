#!/home/bjs66/anaconda2/bin/python2.7

import os
import time as tm

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(run):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/Convert-%s.condor'%run,'w') as f:
        
        # Fixed program location'
        f.write('Executable = _convertData.py\n') #/home/bjs66/anaconda2/bin/python2.7\n')
        
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
        f.write('log = ../../Logs/Test.log\n')
        f.write('Output = ../../Outs/Test.out\n')
        f.write('Error = ../../Errs/Test.err\n')
        
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
#    run = 'Run-15-06-25-12-53-44'
#    run = 'Run-15-06-26-11-23-13'
#    run = 'Run-15-07-31-18-30-14'
#    runDirs = ['Run-15-08-18-14-51-18']
#    run = 'Run-15-08-31-00-23-36'
#    run = 'Run-15-09-21-20-58-01'
#    run = 'Run-15-09-23-21-16-00'
#    run = 'Run-15-10-03-09-26-22'
#    run = 'Run-15-10-13-13-27-09'
#    run = 'Run-15-10-21-13-12-27'
#    run = 'Run-15-10-29-15-56-36'   
#    run = 'Run-15-11-09-11-30-13'
#    run = 'Run-15-11-20-11-34-48'
#    run = 'Run-15-12-14-11-21-45'
#    run = 'Run-15-12-26-08-30-40'
#    run = 'Run-16-01-07-12-16-36'
#    runDirs = ['Run-15-08-31-00-23-36','Run-15-09-23-21-16-00','Run-15-10-03-09-26-22','Run-15-10-13-13-27-09'] 
    runDirs = ['Run-15-06-26-11-23-13','Run-15-07-31-18-30-14'] 
#    runDirs = ['Run-15-10-21-13-12-27','Run-15-10-29-15-56-36','Run-15-11-09-11-30-13','Run-15-11-20-11-34-48','Run-15-11-24-15-35-32','Run-15-12-14-11-21-45']
    for run in runDirs:
        createCondorFile(run)
        cmd = 'condor_submit /home/bjs66/CondorFiles/Convert-%s.condor'%run
        os.system(cmd)
        tm.sleep(1
)
if __name__ == '__main__':
    main()

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
