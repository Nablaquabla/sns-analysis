import os
import time as tm
import sys

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(dataDir,outDir,run,day,times):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/%s-%s.condor'%(run,day),'w') as f:
        
        # Fixed program location'
        f.write('Executable = /home/bjs66/GitHub/sns-analysis/sns-analysis-v3\n')
        
        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"2 %s $(Process) %s 0\"\n'%(dataDir,outDir)) 
        
        # Standard cluster universe
        f.write('universe   = vanilla\n')
        f.write('getenv     = true\n')

        # Program needs at least 300 MB of free memory to hold unzipped data
        f.write('request_memory = 300\n')
     
        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = ../../Logs/%s-%s-$(Process).log\n'%(run,day))
        f.write('Output = ../../Outs/%s-%s-$(Process).out\n'%(run,day))
        f.write('Error = ../../Errs/%s-%s-$(Process).err\n'%(run,day))
        
        # Do not write any emails
        f.write('notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')
        
        # Add single job to queue
        f.write('Queue %i'%times)

# Main function handling all internals
# -----------------------------------------------------------------------------
def main(r):       
    # Choose main directory, i.e. ~/csi/beam_on_data/Run-15-06-25-xyz/
    mainRunDir = '/var/phy/project/phil/grayson/COHERENT/CsI/'
    
    # Choose output directory, i.e. ~/output/Run-15-06-25-xyz/
    mainOutDir = '/var/phy/project/phil/grayson/COHERENT/CsI/bjs-analysis/'

    # Choose run to analyze
    run = 'Position-%s'%r
    
    subdirs = {}
    subdirs[run] = 'am_calibration_1350v'
    days_in = {}
    days_in[run] = ['150617']
    # Iterate through all days in a given run folder, create a condor file and run it.                
    for day in days_in[run]:

        # Prepare paths for further processing
        dataRunDir = mainRunDir + '%s/%s/%s'%(subdirs[run],run,day)
        outDir = mainOutDir + '%s/%s'%(run,day)
    
        # Create output directory if it does not exist
        if not os.path.exists(outDir):
            os.makedirs(outDir)    

        # Get all times within the day folder chosen and prepare condor submit files
        tList = [x.split('.')[0] for x in os.listdir(dataRunDir)]
        createCondorFile(dataRunDir,outDir,run,day,len(tList))
#        createCondorFile(dataRunDir,outDir,run,day,2)
        cmd = 'condor_submit /home/bjs66/CondorFiles/%s-%s.condor'%(run,day)
        os.system(cmd)
        tm.sleep(1)
 
if __name__ == '__main__':
    main(sys.argv[1])

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
