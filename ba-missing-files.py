import os
import time as tm
import sys

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(dataDir,outDir,run,day,time):
    # Condor submission file name convention: run-day-time.condor
    with open('/home/bjs66/CondorFiles/%s-%s-%s.condor'%(run,day,time),'w') as f:
        
        # Fixed program location'
        f.write('Executable = /home/bjs66/GitHub/sns-analysis/sns-analysis-v3\n')
        
        # Arguments passed to the exe:
        # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	    # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
	    # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
        f.write('Arguments = \"3 %s %s %s 1\"\n'%(dataDir,time,outDir)) 
        
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
#    run = 'Run-15-03-27-12-42-26'
#    run = 'Run-15-03-30-13-33-05'
#    run = 'Run-15-04-08-11-38-28'
#    run = 'Run-15-04-17-16-56-59'
#    run = 'Run-15-04-29-16-34-44'
#    run = 'Run-15-05-05-16-09-12'
#    run = 'Run-15-05-11-11-46-30'
#    run = 'Run-15-05-19-17-04-44'
#    run = 'Run-15-05-27-11-13-46'
 #   runDirs = ['Run-15-05-05-16-09-12','Run-15-05-11-11-46-30','Run-15-05-19-17-04-44','Run-15-05-27-11-13-46']
    runDirs = ['Run-15-03-27-12-42-26','Run-15-03-30-13-33-05','Run-15-04-08-11-38-28','Run-15-04-17-16-56-59','Run-15-04-29-16-34-44',
               'Run-15-05-05-16-09-12','Run-15-05-11-11-46-30','Run-15-05-19-17-04-44','Run-15-05-27-11-13-46']
    
    subdirs = {}
    
    days_in = {'Run-15-03-27-12-42-26': ['150327','150328','150329','150330'],
               'Run-15-03-30-13-33-05': ['150330','150331','150401','150402','150403','150404','150405','150406','150407','150408'],
               'Run-15-04-08-11-38-28': ['150408','150409','150410','150411','150412','150413','150414','150415','150416'],
               'Run-15-04-17-16-56-59': ['150417','150418','150419','150420','150421','150422','150423','150424','150425','150426','150427','150428','150429'],
               'Run-15-04-29-16-34-44': ['150429','150430','150501','150502','150503','150504','150505'],
               'Run-15-05-05-16-09-12': ['150505','150506','150507','150508','150509','150510','150511'],
               'Run-15-05-11-11-46-30': ['150512','150513','150514','150515','150516','150517','150518','150519'],
               'Run-15-05-19-17-04-44': ['150519','150520','150521','150522','150523','150524','150525','150526','150527'],
               'Run-15-05-27-11-13-46': ['150527','150528','150529','150530','150531','150601','150602','150603','150604','150605','150606','150607','150608','150609']}
               
    for run in runDirs:           
        for day in days_in[run]:
            subdirs[run] = 'brillance_data'    
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
                if len(missing) > 0: 
                    print len(missing)
                if runMissing == '1':
                    for m in missing:
                        createCondorFile(dataRunDir,outDir,run,day,m)
                        cmd = 'condor_submit /home/bjs66/CondorFiles/%s-%s-%s.condor'%(run,day,m)
                        os.system(cmd)

             
if __name__ == '__main__':
    main(sys.argv[1])

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
