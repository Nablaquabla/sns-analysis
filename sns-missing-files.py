import os
import time as tm
import sys

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(dataDir,outDir,run,day,time):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/%s-%s-%s.condor'%(run,day,time),'w') as f:
        
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
        f.write('request_memory = 300\n')
     
        # Output, error and log name convention: run-day-time.log/out/err
        f.write('log = ../../Logs/%s-%s-%s.log\n'%(run,day,time))
        f.write('Output = ../../Outs/%s-%s-%s.out\n'%(run,day,time))
        f.write('Error = ../../Errs/%s-%ss-%s.err\n'%(run,day,time))
        
        # Do not write any emails
        f.write('notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')
        
        # Add single job to queue
        f.write('Queue')
        
# Main function handling all internals
# -----------------------------------------------------------------------------
def main(runMissing):       
   # Choose main directory, i.e. ~/csi/beam_on_data/Run-15-06-25-xyz/
    mainRunDir = '/var/phy/project/phil/grayson/COHERENT/CsI/'
    
    # Choose output directory, i.e. ~/output/Run-15-06-25-xyz/
    mainOutDir = '/var/phy/project/phil/grayson/COHERENT/CsI/bjs-analysis/'

    # Choose run to analyze
    runDirs = ['Run-15-06-25-12-53-44']
#    runDirs = ['Run-15-06-26-11-23-13','Run-15-07-31-18-30-14']
#    runDirs = ['Run-15-07-31-18-30-14']
#    runDirs = ['Run-15-06-25-12-53-44','Run-15-08-18-14-51-18','Run-15-08-31-00-23-36','Run-15-09-21-20-58-01','Run-15-09-23-21-16-00']
#    runDirs = ['Run-15-10-13-13-27-09','Run-15-10-21-13-12-27','Run-15-10-29-15-56-36','Run-15-11-09-11-30-13']
#    runDirs = ['Run-15-11-20-11-34-48','Run-15-11-24-15-35-32','Run-15-12-14-11-21-45']
#    runDirs = ['Run-15-08-18-14-51-18']
#    run = 'Run-15-08-31-00-23-36'
#    runDirs = ['Run-15-09-21-20-58-01']
#    runDirs = ['Run-15-09-23-21-16-00']
#    runDirs = ['Run-15-10-03-09-26-22','Run-15-10-13-13-27-09']
#    run = 'Run-15-10-13-13-27-09'
#    run = 'Run-15-10-21-13-12-27'
#    run = 'Run-15-10-29-15-56-36'   
#    run = 'Run-15-11-09-11-30-13'
#    run = 'Run-15-11-20-11-34-48'
#    run = 'Run-15-12-14-11-21-45'
#    run = 'Run-15-12-26-08-30-40'
#    run = 'Run-16-01-07-12-16-36'
#    run = 'Run-16-02-02-16-26-26'
#    run = 'Run-16-02-15-13-46-34'
#    run = 'Run-16-02-29-11-54-20'
#    run = 'Run-16-03-09-13-00-14'
#    run = 'Run-16-03-22-18-09-33' 
    
    subdirs = {}
    days_in = {}
    possibleSubDirs = ['beam_off_data','beam_on_data','sns_data']
    
    for run in runDirs:        
        for psd in possibleSubDirs:
            possibleRuns = os.listdir(mainRunDir + psd)
            if run in possibleRuns:
                subdirs[run] = psd
                days_in[run] = [x for x in os.listdir(mainRunDir + psd + '/' + run) if 'Settings' not in x]
                break
        print subdirs
        print days_in
        
    for run in runDirs:           
        for day in days_in[run]:
            print run,day
            # Prepare paths for further processing
            dataRunDir = mainRunDir + '%s/%s/%s'%(subdirs[run],run,day)
            outDir = mainOutDir + '%s/%s'%(run,day)
            
            # Get all times within the day folder chosen
            inputList = [x.split('.')[0] for x in os.listdir(dataRunDir)]
             
            # Get all times within the day folder chosen
            outputList_B = [x.split('-')[1] for x in os.listdir(outDir) if 'B-' in x]
            outputList_S = [x.split('-')[1] for x in os.listdir(outDir) if 'S-' in x]
            outputList_I = [x.split('-')[1] for x in os.listdir(outDir) if 'I-' in x]
            
            # Check if there is a file missing in the day folder
            if len(inputList) != len(outputList_B) or len(inputList) != len(outputList_S) or len(inputList) != len(outputList_I):
                missing = list(set(inputList) - set(outputList_B))
		print missing
                if len(missing) > 0: 
                    print len(missing)
                if runMissing == '1':
                    for m in missing:
                        createCondorFile(dataRunDir,outDir,run,day,m)
                        cmd = 'condor_submit /home/bjs66/CondorFiles/%s-%s-%s.condor'%(run,day,m)
                        os.system(cmd)
		#for m in missing:
		#	print m
             
if __name__ == '__main__':
    main(sys.argv[1])

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
