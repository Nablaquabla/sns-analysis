import os
import time as tm

# Handles the creation of condor files for a given set of directories
# -----------------------------------------------------------------------------
def createCondorFile(RUNTIME):
    with open('/home/bjs66/CondorFiles/copy-%s.condor'%(RUNTIME),'w') as f:
        f.write('Executable = /home/bjs66/GitHub/sns-analysis/copy-data-from-sns.sh\n')
        f.write('Arguments = %s\n'%RUNTIME)
        f.write('Universe = vanilla\n')
        f.write('Getenv = true\n')
        f.write('Log = ../../Logs/copy.log\n')
        f.write('Output = ../../Outs/copy.out\n')
        f.write('Error = ../../Errs/copy.err\n')
        f.write('Notification = never\n')
        f.write('+Department  = Physics\n')
        f.write('should_transfer_files = NO\n')
        f.write('Queue')
        
# Main function handling all internals
# -----------------------------------------------------------------------------
def main():     
    #runArray = ['Run-16-02-15-13-46-34']
    runArray = ['Run-16-02-29-11-54-20','Run-16-03-09-13-00-14','Run-16-03-22-18-09-33' ]
    
    for run in runArray:
        print run              
        createCondorFile(run)
        cmd = 'condor_submit /home/bjs66/CondorFiles/copy-%s.condor'%(run)
        os.system(cmd)
 
if __name__ == '__main__':
    main()

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
