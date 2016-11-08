import os
import time as tm

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(dataDir,outDir,run,day,times):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/%s-%s.condor'%(run,day),'w') as f:
        
        # Fixed program location'
        f.write('Executable = /home/bjs66/GitHub/sns-analysis/sns-analysis-multi-submit\n')
        
        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"%s $(Process) %s 0\"\n'%(dataDir,outDir)) 
        
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
def main():       
    # Choose main directory, i.e. ~/csi/beam_on_data/Run-15-06-25-xyz/
    mainRunDir = '/var/phy/project/phil/grayson/COHERENT/CsI/'
    
    # Choose output directory, i.e. ~/output/Run-15-06-25-xyz/
    mainOutDir = '/var/phy/project/phil/grayson/COHERENT/CsI/bjs-analysis/'

    # Choose run to analyze
#    run = 'Run-15-06-25-12-53-44'
#    run = 'Run-15-06-26-11-23-13'
#    run = 'Run-15-07-31-18-30-14'
#    run = 'Run-15-08-18-14-51-18'
#    run = 'Run-15-08-31-00-23-36'
    run = 'Run-15-09-21-20-58-01'
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
     
    subdirs = {}
    days_in = {}
    read_subdirs = False
    read_days = False
    with open('runtimes-data.dat','r') as f:
        for line in f:
            if 'Start sub-dirs' in line:
                read_subdirs = True
                continue
            if 'End sub-dirs' in line:
                read_subdirs = False
                continue
            if 'Start days' in line:
                read_days = True
                continue
            if 'End days' in line:
                read_days = False
                continue
            
            if read_subdirs:
                key,data = line.strip().split(':')
                subdirs[key] = data.strip()
            if read_days:
                key,data = line.strip().split(':')
                days_in[key] = eval(data)

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
        #createCondorFile(dataRunDir,outDir,run,day,len(tList))
        createCondorFile(dataRunDir,outDir,run,day,2)
        cmd = 'condor_submit /home/bjs66/CondorFiles/%s-%s.condor'%(run,day)
        os.system(cmd)
        tm.sleep(1)
 
if __name__ == '__main__':
    main()

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
