# SNS Analysis Code
Code used for the analysis of CsI[Na] data on the *Duke Condor cluster*. The code basis is the same for the Ba-133 calibration, Am-241 calibration as well as the final SNS data set. Only analysis windows are adjusted according to the experiment.

## Workflow
1. Rsync data from ORNL to Condor
2. Adjust **runDirs** in run-sns-analysis.py, sns-missing-files.py, convert-output-to-hdf5.py and calculate-stability.py
3. On Condor invoke **python run-sns-analysis.py** to start the analysis. The speed-up is usually on the order of ~40, i.e. one can analyze 40 minutes of data in 1 minute wall time.
4. On Condor invoke **python sns-missing-files.py 0** to check for any missing files. If any are found run **python sns-missing-files.py 1**. Usually this happens when a job gets held due to other people having a higher priority.
5. On Condor invoke **python convert-output-to-hdf5.py** to convert all data to easy managable HDF5 data sets.
6. Move all HDF5 data to their respective run folder in **/csi/bjs-analysis/Processed/**
7. On Condor invoke **python calculate-stability-data.py**
