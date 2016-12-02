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
    # Choose run dir to process
#    runDirs = ['Run-15-06-25-12-53-44','Run-15-09-21-20-58-01']
    runDirs = ['Run-15-11-20-11-34-48','Run-15-11-24-15-35-32','Run-15-12-14-11-21-45']           
    progDict = {'convert': '_convertData',
                'stability': '_calculateStabilityData',
                'reduce': '_reduceDataSet'}
    

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

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
