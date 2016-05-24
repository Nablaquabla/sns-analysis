#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <algorithm>
#include <limits>
#include <zip.h>
#include <string>

std::string zeroPad(int num, int size)
{
   std::ostringstream ss;
   ss << std::setw(size) << std::setfill('0') << num;
   return ss.str();
}

std::string fileName(int num, std::string prefix)
{
   std::ostringstream ss;
   ss << prefix << std::setw(6) << std::setfill('0') << num;
   return ss.str();
}

bool timeSort(std::string i, std::string j) 
{ 
    return atoi(i.c_str())<atoi(j.c_str()); 
}

int main(int argc, char* argv[])
{
    // Single byte to be read from the data files
    char c;
    
	// Counters to keep track of cuts
	unsigned long waveformCtr = 0;
	unsigned long linearGateCtr = 0;
	unsigned long muonVetoCtr = 0;
	unsigned long overflowCtr = 0;
	unsigned long bgAnalysisCtr = 0;
	unsigned long sAnalysisCtr = 0;

    // File counter
    unsigned int files_processed = 0;
    unsigned int max_no_files_processed = std::numeric_limits<unsigned int>::max();
    
    // Variables governing the background output files
    int bg_counter = 0;
    int bg_file_number = 0;
    std::ofstream bg_out_file;

    // Variables governing the signal output files
    int s_counter = 0;
    int s_file_number = 0;
    std::ofstream s_out_file;
    
	// Run Info output
	std::ofstream infoOut;

    // LabView headers
    int no_samples = 0;
    int no_channels = 0;

    // Timestamp of current trigger
    std::string timestamp;
    
    // Waveform buffers
    int csi [35000] = {} ;
    int mv [35000]  = {} ;

    // Buffer to store bit shifted samples
    int _tmpC = 0;
    
    // Medians of CsI and muon veto
    int med_csi = 0;
    int med_mv = 0;
    
    // Buffers used for median calculation as well as overflow detection
    unsigned int med_csi_sum = 0;
    unsigned int med_mv_sum = 0;
    unsigned int med_csi_arr [256] = {} ;
    unsigned int med_mv_arr [256] = {};   
    bool med_csi_found = false;
    bool med_mv_found = false;
    bool overflow = false;
      
	// Buffers for SPE integration
	int spe_charge_dist[300] = {};
	int spe_integration_ctr = 0;
	int spe_integration_charge = 0;

    // Rising and falling threshold crossings used for linear gate detection
    int _previous_c = 0;
    int gate_down = 0;
    int gate_up = 0;

    // Waveform contains a gate or muon veto fired more than three times
    bool linear_gate = false;
    bool muon_veto_flag = false;
      
    // Photoelectron counter for all interesting regions
    // pt = pretrace, roi = region of interest, iw = integration window
    unsigned int s_pt_ct = 0;
    unsigned int bg_pt_ct = 0;
    unsigned int s_roi_ct = 0;
    unsigned int bg_roi_ct = 0;
    unsigned int s_iw_ct = 0;
    unsigned int bg_iw_ct = 0;
	unsigned int PE_max_PT = 100000;

    // Buffers to store current peak width in CsI and muon veto waveform
	int above_pe_threshold = 0;
	int below_pe_threshold = 0;
	int current_peak_width = 0;
	int current_pe_width = 0;
    int m_peak_width = 0;

    // PE location in CsI, Muon locations in muon veto
    std::vector<int> peaks;
	std::vector<int> pe_beginnings;
	std::vector<int> pe_endings;
    std::vector<int> muon_peaks; 

    // Charge of PE in PT
    int current_spe_q = 0;
    
    // Integration window buffers
    int s_q_arr [1500] = {};
    int bg_q_arr [1500] = {};
    
    // Threshold buffer and measured rise times
    float thresholds [3] = {};
    float bg_rt [3] = {};
    float s_rt [3] = {};    

    // Buffers used during window analysis
    int idx_0 = 0;
	int idx_w_offset = 0;
    int i_peak = 0;
	int i_pe = 0;
    int q_int = 0;   
    float _t1 = 0.0;
    float _t2 = 0.0;
    
    std::string main_dir;
	int current_time;
	int single_time;
    std::string out_dir;
	std::string current_zip_file;
	std::string time_name_in_zip;

    // Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	// Set current time to be analzyed as index of sorted number of total files in folder, e.g. 0-1439 for a full day
	// Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
	int data_set = 1;
    if (argc == 6) 
    {
		data_set = atoi(argv[1]);
        main_dir = std::string(argv[2]); 
		current_time = atoi(argv[3]);
        out_dir = std::string(argv[4]);
		single_time = atoi(argv[5]);
    }
    else
    {
        std::cout << "Arguments not matching! Aborting now!" << std::endl;
        return 1;
    }

	unsigned int BG_PT[2] = {};
	unsigned int BG_ROI[2] = {};
	unsigned int S_PT[2] = {};
	unsigned int S_ROI[2] = {};

	switch (data_set)
	{
	case 1: BG_PT[0] = 0;
			BG_PT[1] = 20000;
			BG_ROI[0] = 20000;
			BG_ROI[1] = 26000;
			S_PT[0] = 7500;
			S_PT[1] = 27500;
			S_ROI[0] = 27500;
			S_ROI[1] = 33500;
			break;
	case 2: BG_PT[0] = 0;
			BG_PT[1] = 20000;
			BG_ROI[0] = 20000;
			BG_ROI[1] = 26000;
			S_PT[0] = 7450;
			S_PT[1] = 27450;
			S_ROI[0] = 27450;
			S_ROI[1] = 33500;
			break;
	default: std::cout << "Arguments not matching! Aborting now!" << std::endl;
			 return 1;
	}

	// Full analysis -> Converts $(Process) from condor submit to the current time file
	if (single_time == 0)
	{
		// Buffers used to process data files and to step through directories
		std::vector<std::string> time_files;
		DIR *dpdf;
		struct dirent *epdf;

		// Find all time files and sort them
		time_files.clear();
		dpdf = opendir(main_dir.c_str());
		if (dpdf != NULL)
		{
			std::string _tmp;
			while (epdf = readdir(dpdf))
			{
				_tmp = epdf->d_name;
				if (_tmp != "." && _tmp != ".." && _tmp.substr(7) == "zip")
				{
					time_files.push_back(_tmp.substr(0, 6));
				}
			}
		}

		// Sort time files by time in ascending order to convert current_time index to proper file
		std::sort(time_files.begin(), time_files.end(), timeSort);

		// Prepare paths to zipped and unzipped files
		current_zip_file = main_dir + "/" + time_files[current_time] + ".zip";
		time_name_in_zip = time_files[current_time];
	}

	// Single file analysis -> Directly convert input to current time file
	else
	{
		current_zip_file = main_dir + "/" + zeroPad(current_time,6) +".zip";
		time_name_in_zip = zeroPad(current_time,6);
	}

	//Open the ZIP archive
	int err = 0;
	int zidx = 0;
	int fileSize = 0;
	zip *z = zip_open(current_zip_file.c_str(), 0, &err);

	//Search for the file of given name
	const char *name = time_name_in_zip.c_str();
	struct zip_stat st;
	zip_stat_init(&st);
	zip_stat(z, name, 0, &st);

	//Alloc memory for its uncompressed contents
	char *contents = new char[st.size];

	//Read the compressed file
	zip_file *f = zip_fopen(z, time_name_in_zip.c_str(), 0);
	fileSize = st.size;
	zip_fread(f, contents, fileSize);
	zip_fclose(f);

	//And close the archive
	zip_close(z);

	// Create signal, background and info output files
	bg_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "B-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	s_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "S-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	infoOut.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "I-")).c_str(), std::ofstream::out | std::ofstream::trunc);
            
	// Begin data processing if file has been properly opened
	if(err == 0)
	{
		waveformCtr = 0;
		zidx = 0;

		// Begin reading byte-stream
		while (zidx < fileSize)
		{   

			// Read LabView header and get the total number of samples written for each channel in the next chunk of data
			for (int i=0;i<4;i++)
			{
				c = contents[zidx++];
				no_samples = no_samples << 8 | (unsigned char) c;
			}
			// Read LabView header and get the total number of channels written in next chunk of data (always 2 in our case)
			for (int i=0;i<4;i++)
			{
				c = contents[zidx++];
				no_channels = no_channels << 8 | (unsigned char) c;
			}

			// Takes care of LabViews closing bit...
			if (no_samples > 350070)
			{
				break;
			}

			// ----------------------------------------------------------------
			//  Process XYZ consecutive waveforms without encountering another
			// LabView header inbetween
			// ----------------------------------------------------------------
			for (int wf=0; wf < (int) (no_samples/35007); wf++)
			{
				// A new waveform begins
				waveformCtr += 1;

				// -------------------------------------------------------------
				//    Reset all major waveform specific variables
				// -------------------------------------------------------------
				timestamp.clear();
				overflow = false;
				linear_gate = false;
				muon_veto_flag = false;
				_previous_c = 128;
				gate_down = 0;
				gate_up = 0;
				memset(med_csi_arr,0,sizeof med_csi_arr);
				memset(med_mv_arr,0,sizeof med_mv_arr );      
				med_csi_found = false;
				med_mv_found = false;
				med_csi_sum = 0;
				med_mv_sum = 0;     
				above_pe_threshold = 0;
				below_pe_threshold = 0;
				current_peak_width = 0;
				current_pe_width = 0;
				current_spe_q = 0;
				m_peak_width = 0;
				peaks.clear();
				pe_beginnings.clear();
				pe_endings.clear();
				muon_peaks.clear();
				spe_integration_ctr = 0;
				spe_integration_charge = 0;

				// -------------------------------------------------------------
				//  Read current timestamp
				// -------------------------------------------------------------
				for(int i=0; i<7; i++)
				{
					c = contents[zidx++];
					c = contents[zidx++];
					timestamp += zeroPad((int) c, 2);  
				}
				std::cout << "Waveform timestamp: " << timestamp << std::endl;
				// ---------------------------------------------------------------
				// Read the full CsI and muon veto waveforms from the zip-stream
				// + Apply bit transformation
				// + Determine if a linear gate is present
				// ---------------------------------------------------------------
				for(int i=0; i<35000; i++)
				{
					// CsI
					c = contents[zidx++];
					_tmpC = (int) c - (int) floor(((float) c + 5.0)/11.0);
					if (i<20000){ med_csi_arr[_tmpC + 128] += 1; }
					csi[i] = _tmpC;
					std::cout << c << " " << csi[i] << std::endl;

					// Gate check
					if (_tmpC <= 18 && _previous_c > 18) { gate_down++; }
					if (_previous_c <= 18 && _tmpC > 18) { gate_up++; }
					_previous_c = _tmpC;

					// Muon veto
					c = contents[zidx++];
					_tmpC = (int) c + (int) ((signbit((int) c) ? -1 : 1 ) * floor((4.0 - abs((float) c))/11.0));
					med_mv_arr[_tmpC + 128] += 1;
					mv[i] = _tmpC;
				}
                        
				// ----------------------------------------------
				//  Adjust linear gate counter if wf is gated
				// ----------------------------------------------
				if (gate_down != gate_up) 
				{ 
					linear_gate = true;
					linearGateCtr += 1;
				}
         
				// -------------------------------------------------
				//  Adjust overflow counter if wf shows an overflow
				// -------------------------------------------------
				if (med_csi_arr[12] > 0 || med_csi_arr[243] > 0)
				{
					overflow = true;
					overflowCtr += 1;
				} 

				// ---------------------------------------
				//  Calculate the median of both channels
				// ---------------------------------------
				
				for(int i=0; i<256; i++)
				{
					if (!med_csi_found)
					{
						med_csi_sum += med_csi_arr[i];
						if (med_csi_sum >= 17499)
						{
							med_csi = i-128;
							med_csi_found=true;
						}
					}

					if (!med_mv_found)
					{
						med_mv_sum += med_mv_arr[i];
						if (med_mv_sum >= 17499)
						{
							med_mv = i-128;
							med_mv_found=true;	
						}
					}
				} 
				std::cout << "Found Median: " << med_csi << std::endl;
				std::cout << "Offset Waveform: " << std::endl;
				// -----------------------------------------------
				//     Find peaks and photoelectrons in waveforms
				// -----------------------------------------------
				for(int i=0;i<35000; i++)
				{
					// -------------------------------------------
					//          Analyze CsI[Na] waveform
					// -------------------------------------------
					// Additional offset of 1 necessary?!
					csi[i] = med_csi - csi[i];
					std::cout << csi[i] << std::endl;
					// Peak finder
					if (csi[i] >= 3) { current_peak_width++; }
					else
					{
						if (current_peak_width >= 3) { peaks.push_back(i - current_peak_width); }
						current_peak_width = 0;
					}

					// Determine integration windows for possible photoelectrons
					if (csi[i] >= 2)
					{
						above_pe_threshold += 1;
						below_pe_threshold = 0;
					}
					else
					{
						below_pe_threshold += 1;

						// Less than three consecutive samples have been found to be above threshold before the 'drop'-> No PE & continue streaming
						if (above_pe_threshold < 3) { above_pe_threshold = 0; }
						else
						{
							// Ignore single dips below threshold, but 'close' the PE window if there are two consecutive samples below threshold.
							if (below_pe_threshold >= 2)
							{
								above_pe_threshold = 0;
								pe_beginnings.push_back(i - current_pe_width - 2);
								pe_endings.push_back(i - 2);
								current_pe_width = 0;
							}
						}
					}
					current_pe_width += (above_pe_threshold >= 3) ? 1 : 0;
					
					// -------------------------------------------
					//        Analyze muon veto waveform
					// -------------------------------------------
					mv[i] = med_mv - mv[i];

					// Peak finder
					if (mv[i] >= 10) { m_peak_width++; }
					else
					{
						if (m_peak_width >= 3) { muon_peaks.push_back(i-m_peak_width); }
						m_peak_width = 0;
					}
				}
				return 0;
				// Raise muon veto flag if more than three muons have been found
				// If less than 3 have been found fill the vector with -1 for postprocessing
				if (muon_peaks.size() > 3)
				{
					muon_veto_flag = true;
					muonVetoCtr += 1;
				}
				else { muon_peaks.resize(3,-1); }

				// ========================================================================
				// Check that there is at least one PE in the trace, there is no overflow, 
				// no linear gate and no muon veto flag, otherwise continue to next
				// waveform without bothering to analyze this one
				// ========================================================================
				if (peaks.size() > 0 && !overflow && !linear_gate && !muon_veto_flag)
				{
					// -------------------------------------------------------------
					// Integrate all SPE found in the PT and histogram their charge
					// -------------------------------------------------------------
					for (int idx = 0; idx <= pe_beginnings.size(); idx++)
					{
						if (pe_beginnings[idx] < BG_PT[1])
						{
							current_spe_q = 0;
							for (int i = pe_beginnings[idx] - 2; i <= pe_endings[idx] + 2; i++)
							{
								if (i >= 0){ current_spe_q += csi[i]; }
							}
							if (current_spe_q >= -50 && current_spe_q < 250) { spe_charge_dist[current_spe_q+50] += 1; }
						}
						else { break; }
					}

					// -------------------------------------------------------------
					//       Determine number of PE in different regions
					// -------------------------------------------------------------
					bg_pt_ct = 0;
					bg_roi_ct = 0;
					bg_iw_ct = 0;
					s_pt_ct = 0;
					s_roi_ct = 0;
					s_iw_ct = 0;
					for (int idx = 0; idx < peaks.size(); idx++)
					{
						if (peaks[idx] >= BG_PT[0]  && peaks[idx] < BG_PT[1]) { bg_pt_ct += 1; }
						if (peaks[idx] >= S_PT[0]   && peaks[idx] < S_PT[1]) { s_pt_ct += 1; }
						if (peaks[idx] >= BG_ROI[0] && peaks[idx] < BG_ROI[1]) { bg_roi_ct += 1; }
						if (peaks[idx] >= S_ROI[0]  && peaks[idx] < S_ROI[1]) { s_roi_ct += 1; }
					}
					// -------------------------------------------------------------
					//    Only analzye BG region if there are a maximum of PE_max_PT
					//    & at least one PE in ROI
					// -------------------------------------------------------------
					if (bg_pt_ct <= PE_max_PT && bg_roi_ct > 0)
					{
						bgAnalysisCtr += 1;

						// Reset window parameters
						idx_0 = 0;
						i_peak = 0;
						i_pe = 0;
						q_int = 0;
						_t1 = 0.0;
						_t2 = 0.0;

						// -------------------------------------------------------------
						//  Determine onset of integration window by finding the first
						//  PE in the region of interest
						// -------------------------------------------------------------      
						for (int i = 0; i < peaks.size(); i++)
						{
							if (peaks[i] >= BG_ROI[0])
							{
								idx_0 = peaks[i];
								i_peak = i;
								break;
							}
						}
						idx_0 -= 10;

						// -------------------------------------------------------------
						// Only analyze if the full integration window is within the ROI
						// -------------------------------------------------------------
						if (idx_0 < (BG_ROI[1] - 1500))
						{

							// --------------------------------------------------------------
							// Determine number of peaks in integration window (magic bullet)
							// --------------------------------------------------------------
							for (int i = i_peak; i < peaks.size(); i++)
							{
								bg_iw_ct += (peaks[i] - idx_0 < 1500) ? 1 : 0;
							}

							// -------------------------------------------------------------
							// Find first PE within the integration window
							// -------------------------------------------------------------
							for (int i = 0; i < pe_beginnings.size(); i++)
							{
								if (pe_beginnings[i] >= idx_0) { i_pe = i; break; }
							}

							// -------------------------------------------------------------
							//    Integrate over all PE found in the integration window
							// -------------------------------------------------------------
							for (int i = 0; i < 1500; i++)
							{
								// Get proper 'real' index that includes the onset offset
								idx_w_offset = i + idx_0;

								// Add sample if it is within one of the PE regions identified previously
								if (idx_w_offset >= pe_beginnings[i_pe] && idx_w_offset <= pe_endings[i_pe])
								{
									q_int += csi[idx_w_offset];
									if (idx_w_offset == pe_endings[i_pe]) { i_pe += ((i_pe + 1) < pe_beginnings.size()) ? 1 : 0; }
								}

								// Keep track of charge integration to determine rise times later
								bg_q_arr[i] = q_int;
							}

							// -------------------------------------------------------------
							//    Determine rise times
							// -------------------------------------------------------------

							// Calculate charge thresholds
							thresholds[0] = 0.1*bg_q_arr[1499];
							thresholds[1] = 0.5*bg_q_arr[1499];
							thresholds[2] = 0.9*bg_q_arr[1499];

							// Determine threshold crossing times
							for (int i = 0; i < 1499; i++)
							{
								_t1 = (float)bg_q_arr[i];
								_t2 = (float)bg_q_arr[i + 1];

								for (int j = 0; j < 3; j++)
								{
									if (_t1 < thresholds[j] && _t2 >= thresholds[j]){ bg_rt[j] = i + (thresholds[j] - _t1) / (_t2 - _t1); }
								}
							}

							// -------------------------------------------------------------
							//    Write analysis results to file
							// -------------------------------------------------------------
							bg_out_file << timestamp << " " << med_csi << " " << med_mv << " ";
							bg_out_file << bg_pt_ct << " " << bg_roi_ct << " ";
							bg_out_file << bg_iw_ct << " " << idx_0 << " " << bg_q_arr[1499] << " ";
							bg_out_file << bg_rt[0] << " " << bg_rt[1] << " " << bg_rt[2] << " ";
							bg_out_file << muon_peaks[0] << " " << muon_peaks[1] << " " << muon_peaks[2] << std::endl;

							// Keeps track of how many BG waveforms have actually been analyzed
							bg_counter++;
						}
					}

					// -------------------------------------------------------------
					//    Only analzye S region if there are a maximum of PE_max_PT
					//    & at least one PE in ROI
					// -------------------------------------------------------------
					if (s_pt_ct <= PE_max_PT && s_roi_ct > 0)
					{
						sAnalysisCtr += 1;

						// Reset window parameters
						idx_0 = 0;
						i_peak = 0;
						i_pe = 0;
						q_int = 0;
						_t1 = 0.0;
						_t2 = 0.0;

						// -------------------------------------------------------------
						//  Determine onset of integration window by finding the first
						//  PE in the region of interest
						// -------------------------------------------------------------      
						for (int i = 0; i < peaks.size(); i++)
						{
							if (peaks[i] >= S_ROI[0])
							{
								idx_0 = peaks[i];
								i_peak = i;
								break;
							}
						}
						idx_0 -= 10;

						// -------------------------------------------------------------
						// Only analyze if the full integration window is within the ROI
						// -------------------------------------------------------------
						if (idx_0 < (S_ROI[1] - 1500))
						{

							// --------------------------------------------------------------
							// Determine number of peaks in integration window (magic bullet)
							// --------------------------------------------------------------
							for (int i = i_peak; i < peaks.size(); i++)
							{
								s_iw_ct += (peaks[i] - idx_0 < 1500) ? 1 : 0;
							}

							// -------------------------------------------------------------
							// Find first PE within the integration window
							// -------------------------------------------------------------
							for (int i = 0; i < pe_beginnings.size(); i++)
							{
								if (pe_beginnings[i] >= idx_0) { i_pe = i; break; }
							}

							// -------------------------------------------------------------
							//    Integrate over all PE found in the integration window
							// -------------------------------------------------------------
							for (int i = 0; i < 1500; i++)
							{
								// Get proper 'real' index that includes the onset offset
								idx_w_offset = i + idx_0;

								// Add sample if it is within one of the PE regions identified previously
								if (idx_w_offset >= pe_beginnings[i_pe] && idx_w_offset <= pe_endings[i_pe])
								{
									q_int += csi[idx_w_offset];
									if (idx_w_offset == pe_endings[i_pe]) { i_pe += ((i_pe + 1) < pe_beginnings.size()) ? 1 : 0; }
								}

								// Keep track of charge integration to determine rise times later
								s_q_arr[i] = q_int;
							}

							// -------------------------------------------------------------
							//    Determine rise times
							// -------------------------------------------------------------

							// Calculate charge thresholds
							thresholds[0] = 0.1*s_q_arr[1499];
							thresholds[1] = 0.5*s_q_arr[1499];
							thresholds[2] = 0.9*s_q_arr[1499];

							// Determine threshold crossing times
							for (int i = 0; i < 1499; i++)
							{
								_t1 = (float)s_q_arr[i];
								_t2 = (float)s_q_arr[i + 1];

								for (int j = 0; j < 3; j++)
								{
									if (_t1 < thresholds[j] && _t2 >= thresholds[j]){ bg_rt[j] = i + (thresholds[j] - _t1) / (_t2 - _t1); }
								}
							}

							// -------------------------------------------------------------
							//    Write analysis results to file
							// -------------------------------------------------------------
							s_out_file << timestamp << " " << med_csi << " " << med_mv << " ";
							s_out_file << s_pt_ct << " " << s_roi_ct << " ";
							s_out_file << s_iw_ct << " " << idx_0 << " " << s_q_arr[1499] << " ";
							s_out_file << s_rt[0] << " " << s_rt[1] << " " << s_rt[2] << " ";
							s_out_file << muon_peaks[0] << " " << muon_peaks[1] << " " << muon_peaks[2] << std::endl;

							// Keeps track of how many S waveforms have actually been analyzed
							s_counter++;
						}
					}
					
				}
			}
		}            
	}
            
    // Before exiting, make sure that both output files are properly closed to prevent data loss.
    if (bg_out_file.is_open()) { bg_out_file.close(); }
    if (s_out_file.is_open()) { s_out_file.close(); }

	// Write run info
	if (infoOut.is_open())
	{
		// Description of output
		infoOut << "0:  Total number of Waveforms processed" << std::endl;
		infoOut << "1:  Number of triggers with linear gates in CsI channel" << std::endl;
		infoOut << "2:  Number of triggers with overflows in either channel" << std::endl;
		infoOut << "3:  Number of triggers with more than 3 muons in muon veto channel" << std::endl;
		infoOut << "4:  Number of triggers that have actually been analyzed in the background window (less than x PE/peaks in PT + at least one PE/peak in ROI)" << std::endl;
		infoOut << "5:  Number of triggers that have actually been analyzed in the signal window (less than x PE/peaks in PT + at least one PE/peak in ROI)" << std::endl;
		infoOut << "6:  Maximum number of PE/peaks in the pretrace" << std::endl;
		infoOut << "7:  Integration threshold" << std::endl;
		infoOut << "8:  Background pretrace start" << std::endl;
		infoOut << "9:  Background pretrace stop" << std::endl;
		infoOut << "10: Background ROI start" << std::endl;
		infoOut << "11: Background ROI stop" << std::endl;
		infoOut << "12: Signal pretrace start" << std::endl;
		infoOut << "13: Signal pretrace stop" << std::endl;
		infoOut << "14: Signal ROI start" << std::endl;
		infoOut << "15: Signal ROI stop" << std::endl;
		infoOut << "16: Unzipped file size" << std::endl;
		infoOut << "17: SPE histogram" << std::endl;

		// Actual output
		infoOut << "0\t" << waveformCtr << std::endl;
		infoOut << "1\t" << linearGateCtr << std::endl;
		infoOut << "2\t" << overflowCtr << std::endl;
		infoOut << "3\t" << muonVetoCtr << std::endl;
		infoOut << "4\t" << bgAnalysisCtr << std::endl;
		infoOut << "5\t" << sAnalysisCtr << std::endl;
		infoOut << "6\t" << PE_max_PT << std::endl;
		infoOut << "7\t" << "Not In Use" << std::endl;
		infoOut << "8\t" << BG_PT[0] << std::endl;
		infoOut << "9\t" << BG_PT[1] << std::endl;
		infoOut << "10\t" << BG_ROI[0] << std::endl;
		infoOut << "11\t" << BG_ROI[1] << std::endl;
		infoOut << "12\t" << S_PT[0] << std::endl;
		infoOut << "13\t" << S_PT[1] << std::endl;
		infoOut << "14\t" << S_ROI[0] << std::endl;
		infoOut << "15\t" << S_ROI[1] << std::endl;
		infoOut << "16\t" << fileSize << std::endl;
		infoOut << "17\t";
		for (int idx = 0; idx < 300; idx++)
		{
			infoOut << spe_charge_dist[idx] << " ";
		}
		infoOut.close();
	}
    return 0;
}