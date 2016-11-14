import os
import time as tm
import sys

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(dataDir,outDir,time):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/test.condor','w') as f:
        
        # Fixed program location'
        f.write('Executable = /home/bjs66/GitHub/sns-analysis/sns-analysis-v4\n')
        
        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"1 %s %s %s 1\"\n'%(dataDir,time,outDir)) 
        
        # Standard cluster universe
        f.write('universe   = vanilla\n')
        f.write('getenv     = true\n')

        # Program needs at least 250 MB of free memory to hold unzipped data
        f.write('request_memory = 400\n')
     
        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = ../../Logs/test.log\n')
        f.write('Output = ../../Outs/test.out\n')
        f.write('Error = ../../Errs/test.err\n')
        
        # Do not write any emails
        f.write('notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')
        
        # Add single job to queue
        f.write('Queue')
        
# Main function handling all internals
# -----------------------------------------------------------------------------
def main():       
   # Choose main directory, i.e. ~/csi/beam_on_data/Run-15-06-25-xyz/
    mainRunDir = '/var/phy/project/phil/grayson/COHERENT/CsI/'
    
    # Choose output directory, i.e. ~/output/Run-15-06-25-xyz/
    mainOutDir = '/var/phy/project/phil/grayson/COHERENT/CsI/bjs-analysis/test/'
    
    pathToFile = 'beam_on_data/Run-15-09-21-20-58-01/150923/'
    time = '085902'

    dataRunDir = mainRunDir + pathToFile
    createCondorFile(dataRunDir,mainOutDir,time)
    cmd = 'condor_submit /home/bjs66/CondorFiles/test.condor'
    os.system(cmd)
             
if __name__ == '__main__':
    main()

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
