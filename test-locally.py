import os
import time as tm

# Main function handling all internals
# -----------------------------------------------------------------------------
def main():       
    # Choose main directory, i.e. ~/csi/beam_on_data/Run-15-06-25-xyz/
    mainRunDir = 'F:/Work-Data-Storage/CsI/Testing-Folder/'
    
    # Choose output directory, i.e. ~/output/Run-15-06-25-xyz/
    mainOutDir = 'F:/Work-Data-Storage/CsI/Testing-Folder/Output'

    # Choose run to analyze
    run = '0kb-Files'

    # Fixed program location'
    f.write('Executable = /home/bjs66/GitHub/sns-analysis/sns-analysis-v4\n')

    # Arguments passed to the exe:
    # Set main run directory, e.g. Run-15-10-02-27-32-23/151002
        # Set current time to be analzyed (w/o .zip extension!), e.g. 184502
        # Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
    f.write('Arguments = \"1 %s %s %s 1\"\n'%(dataDir,time,outDir))

 
if __name__ == '__main__':
    main()

    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
