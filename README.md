# SNS Analysis Code
Code used for the analysis of CsI[Na] data on the *Duke Condor cluster*. The code basis is the same for the Ba-133 calibration, Am-241 calibration as well as the final SNS data set. Only analysis windows are adjusted according to the experiment.

## Workflow
1. Rsync data from ORNL to Condor
2. Adjust **runDirs** in 
  * run-sns-analysis.py
  * sns-missing-files.py
  * convert-output-to-hdf5.py
  * calculate-stability.py
3. ``python run-sns-analysis.py``: Start analysis of the runs specified in runDirs. About 40 minutes of data can be analyzed in one minute of wall time.
4. ``python sns-missing-files.py 0``: Check for any missing files. Usually happens if jobs are held due to other people having higher priority on that node.
5. ``python sns-missing-files.py 1``: Reanalyze all missing data.
5. ``python convert-output-to-hdf5.py``: Convert all data to easy managable HDF5 data sets.
6. Move all HDF5 data to their respective run folder in **/csi/bjs-analysis/Processed/**
7. ``python calculate-stability-data.py``: Reads the info part of the HDF5 data and creates easy to handle HDF5 tables containing info about the detector stability
