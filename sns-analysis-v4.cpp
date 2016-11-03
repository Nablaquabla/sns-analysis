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
#include <queue>

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

	// Saved waveforms
	std::ofstream waveformOut;

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

	// Buffers for peaks in pretrace
	int bg_pe_pt[52] = {};
	int s_pe_pt[52] = {};

	// Buffer for baseline
	int baseline_hist[32] = {};

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
	unsigned int PE_max_PT = 10;

    // Buffers to store current peak width in CsI and muon veto waveform
	int above_pe_threshold = 0;
	int current_peak_width = 0;
	int current_pe_width = 0;
    int m_peak_width = 0;

    // PE location in CsI, Muon locations in muon veto
    std::vector<int> peaks;
	std::vector<int> peak_heights;
	int peak_height_dist[100] = {};
	int peak_amplitude = 0;
	int max_peak_charge = -1;
	int max_peak_charge_dist[5000] = {};
	int max_peak_charge_dist_mv[5000] = {};
	std::vector<int> pe_beginnings;
	std::vector<int> pe_endings;
    std::vector<int> muon_peaks; 

	std::vector<int> muon_onset_arr;
	// std::vector<int> tmp_waveform;

	// Keep track of all peak/pe locations in the full minute
	int peak_distribution[350][350] = {};
	int charge_distribution[350][350] = {};
	int charge_distribution_mat[350][350] = {};
	for (int i1 = 0; i1 < 350; i1++)
	{
		for (int i2 = 0; i2 < 350; i2++)
		{
			peak_distribution[i1][i2] = 0;
			charge_distribution[i1][i2] = 0;
			charge_distribution[i1][i2] = 0;
		}
	}
	// Get peak width distribution
	int peak_width_distribution[51] = {};

    // Charge of PE in PT
    int current_spe_q = 0;
    
    // Integration window buffers
    int s_q_arr [1500] = {};
    int bg_q_arr [1500] = {};
	int running_charge = 0;

	// LogLikelihood prefactors & estimators
	double lnL_pf_real[1500] = {};
	double lnL_pf_flat[1500] = {};
	double lnL_real = 0;
	double lnL_flat = 0;

	// Big pulse detection
	bool bP_detected = false;
	int _t_bP_idx = 0;
	std::vector<int> bP_onset_arr;
	std::vector<int> bP_charge_arr;

	// Calculate prefactors for loglikelihood analysis
	// timing and fraction from NIMA paper
	double r = 041;
	double tf = 527.0;
	double ts = 5600.0;

	// total integration window is 3 us long -> tMax = 1500
	double _tFast = 1.0 / ((1.0 + r) * tf * (1 - exp(-1500.0 / tf)));
	double _tSlow = r / ((1.0 + r) * ts * (1 - exp(-1500.0 / ts)));
		
	// Calculate prefactor for each time step
	for (int i = 0; i < 1500; i++)
	{
		lnL_pf_real[i] = log(_tFast * exp(-(double)i / tf) + _tSlow * exp(-(double)i / ts));
		lnL_pf_flat[i] = log(1.0 / 1500.0);
	}

    // Threshold buffer and measured rise times
    double thresholds [3] = {};
    double bg_rt [3] = {};
    double s_rt [3] = {};    

    // Buffers used during window analysis
    int idx_0 = 0;
	int idx_w_offset = 0;
    int i_peak = 0;
	int i_pe = 0;
    int q_int = 0;   
    double _t1 = 0.0;
    double _t2 = 0.0;
    
    std::string main_dir;
	int current_time;
	int single_time;
    std::string out_dir;
	std::string current_zip_file;
	std::string time_name_in_zip;
	
	// Save waveforms as ascii - Counter,cuts etc
	int save_wf_ctr = 0;
	int no_total_peaks[2] = { 100, 200 };
	int minimum_no_peaks_iw = 6;
	int rt1090_bottom_left[2] = { 750, 1150 };
	int rt050_bottom_left[2] = { 235, 345 };
	int rt1090_upper_right[2] = { 1080, 1280 };
	int rt050_upper_right[2] = { 520, 760 };
	bool save_waveforms = false;
	bool passed_cuts_bg = false;
	bool passed_cuts_s = false;
	bool passed_cut = false;

	// Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	// Set current time to be analzyed as index of sorted number of total files in folder, e.g. 0-1439 for a full day
	// Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
	int data_set = 0;
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
	case 1: BG_PT[0]  = 0;
			BG_PT[1]  = 19975;
			BG_ROI[0] = 19975;
			BG_ROI[1] = 27475;
			S_PT[0]   = 7475;
			S_PT[1]   = 27475;
			S_ROI[0]  = 27475;
			S_ROI[1]  = 34975;
			PE_max_PT = 10;
			break;
	case 2: BG_PT[0]  = 0;
			BG_PT[1]  = 20000;
			BG_ROI[0] = 20000;
			BG_ROI[1] = 27400;
			S_PT[0]   = 7400;
			S_PT[1]   = 27400;
			S_ROI[0]  = 27400;
			S_ROI[1]  = 35000;
			PE_max_PT = 20;
			break;
	case 3: BG_PT[0]  = 0;
			BG_PT[1]  = 20000;
			BG_ROI[0] = 20000;
			BG_ROI[1] = 25000;
			S_PT[0]   = 5000;
			S_PT[1]   = 25000;
			S_ROI[0]  = 25000;
			S_ROI[1]  = 30000;
			PE_max_PT = 20;
			break;
	/*case 3: BG_PT[0]  = 0;
		    BG_PT[1]  = 20000;
			BG_ROI[0] = 20000;
			BG_ROI[1] = 25125;
			S_PT[0]   = 5125;
			S_PT[1]   = 25125;
			S_ROI[0]  = 25125;
			S_ROI[1]  = 30250;
			PE_max_PT = 20;
			break;*/
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

	//std::cout << time_name_in_zip << std::endl;

	//Open the ZIP archive
	int err = 0;
	int zidx = 0;
	int fileSize = 0;
	zip *z = zip_open(current_zip_file.c_str(), 0, &err);
	//std::cout << "Opened zip" << std::endl;

	//Search for the file of given name
	const char *name = time_name_in_zip.c_str();
	struct zip_stat st;
	zip_stat_init(&st);
	zip_stat(z, name, 0, &st);

	//Alloc memory for its uncompressed contents
	char *contents = new char[st.size];

	//Read the compressed file
	zip_file *f = zip_fopen(z, time_name_in_zip.c_str(), 0);
	//std::cout << "Reading file" << std::endl;
	fileSize = st.size;
	//std::cout << fileSize << std::endl;
	zip_fread(f, contents, fileSize);
	//std::cout << "Read chunk" << std::endl;
	zip_fclose(f);

	//And close the archive
	zip_close(z);
	//std::cout << "Zip closed" << std::endl;

	// Create signal, background and info output files
	bg_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "B-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	s_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "S-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	infoOut.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "I-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	if (save_waveforms)
	{
		waveformOut.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "W-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	}
	//std::cout << "Created output files" << std::endl;

	int csi_raw[35000] = {};
	//std::cout << err << std::endl;
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
				current_peak_width = 0;
				current_pe_width = 0;
				current_spe_q = 0;
				m_peak_width = 0;
				peaks.clear();
				peak_heights.clear();
				pe_beginnings.clear();
				pe_endings.clear();
				muon_peaks.clear();
				spe_integration_ctr = 0;
				spe_integration_charge = 0;
				max_peak_charge = -1;
				passed_cuts_s = false;
				passed_cuts_bg = false;
				passed_cut = false;
				peak_amplitude = 0;
				bP_detected = false;
				_t_bP_idx = 0;
				// tmp_waveform.clear();

				// -------------------------------------------------------------
				//  Read current timestamp
				// -------------------------------------------------------------
				for(int i=0; i<7; i++)
				{
					c = contents[zidx++];
					c = contents[zidx++];
					timestamp += zeroPad((int) c, 2);  
				}

				// ---------------------------------------------------------------
				// Read the full CsI and muon veto waveforms from the zip-stream
				// + Apply bit transformation
				// + Determine if a linear gate is present
				// ---------------------------------------------------------------
				for(int i=0; i<35000; i++)
				{
					// CsI
					c = contents[zidx++];
					_tmpC = (int) c - (int) floor(((double) c + 5.0)/11.0);
					if (i<20000){ med_csi_arr[_tmpC + 128] += 1; }
					csi[i] = _tmpC;
					csi_raw[i] = _tmpC;

					if (i == 0) { _previous_c = _tmpC; }

					// Gate check
					if (_tmpC <= 18 && _previous_c > 18) { gate_down++; }
					if (_previous_c <= 18 && _tmpC > 18) { gate_up++; }
					_previous_c = _tmpC;

					// Overflow check
					if (!overflow && (c >= 127 || c == -128))
					{
						overflow = true;
						overflowCtr += 1;
					}
					
					// Muon veto
					c = contents[zidx++];
					_tmpC = (int) c + (int) ((signbit((int) c) ? -1 : 1 ) * floor((4.0 - abs((double) c))/11.0));
					med_mv_arr[_tmpC + 128] += 1;
					mv[i] = _tmpC;
				}

				// ---------------------------------------
				//  Calculate the median of both channels
				// ---------------------------------------
				for(int i=0; i<256; i++)
				{
					if (!med_csi_found)
					{
						med_csi_sum += med_csi_arr[i];
						if (med_csi_sum >= 10000)
						{
							med_csi = i-128;
							med_csi_found=true;
						}
					}

					if (!med_mv_found)
					{
						med_mv_sum += med_mv_arr[i];
						if (med_mv_sum >= 10000)
						{
							med_mv = i-128;
							med_mv_found=true;	
						}
					}
				} 

				// ----------------------------------------------
				//  Adjust linear gate counter if wf is gated
				// ----------------------------------------------
				if (gate_down != gate_up || med_csi < 50)
				{
					linear_gate = true;
					linearGateCtr += 1;
				}
				// -----------------------------------------------
				//     Find peaks and photoelectrons in waveforms
				// -----------------------------------------------
				for (int i = 0; i < 35000; i++)
				{
					// -------------------------------------------
					//          Analyze CsI[Na] waveform
					// -------------------------------------------
					csi[i] = med_csi - csi[i];

					// Simple peak finder using threshold crossing with history
					if (csi[i] >= 3) { current_peak_width++; }
					else
					{
						if (current_peak_width >= 3)
						{
							peaks.push_back(i - current_peak_width);
							if (!bP_detected && current_peak_width >= 35)
							{
								bP_detected = true;
								bP_onset_arr.push_back(i - current_peak_width);
							}
						}
						current_peak_width = 0;
					}

					// Determine integration windows for possible photoelectrons (Threshold = 3, Width = 3)
					if (csi[i] >= 3)
					{
						above_pe_threshold += 1;
						peak_amplitude = csi[i] > peak_amplitude ? csi[i] : peak_amplitude;
					}
					else
					{
						if (above_pe_threshold >= 3) 
						{ 
							peak_width_distribution[(above_pe_threshold < 50) ? above_pe_threshold : 50] += 1;
							pe_beginnings.push_back(i - above_pe_threshold - 2);
							pe_endings.push_back(i + 1);
							peak_heights.push_back(peak_amplitude);
						}
						peak_amplitude = 0;
						above_pe_threshold = 0;
					}
					
					// -------------------------------------------
					//        Analyze muon veto waveform
					// -------------------------------------------
					mv[i] = med_mv - mv[i];

					// Peak finder
					if (mv[i] >= 10) { m_peak_width++; }
					else
					{
						if (m_peak_width >= 3)
						{
							muon_peaks.push_back(i-m_peak_width);
						}
						m_peak_width = 0;
					}
				}

				// If there was a big pulse in the trace integrate it
				// But only if there was no linear gate or overflow
				if (bP_detected && !linear_gate && !overflow)
				{
					int _t_charge = 0;
					for (int i = bP_onset_arr.back(); i < 1499 + bP_onset_arr.back(); i++)
					{
						_t_charge += csi[i] >= 3 ? csi[i] : 0;
					}
					bP_charge_arr.push_back(_t_charge);
				}

				// Raise muon veto flag if more than three muons have been found
				// If less than 3 have been found fill the vector with -1 for postprocessing
				int muons_found = muon_peaks.size();

				if (muons_found > 0)
				{
					if (linear_gate)
					{
						muon_onset_arr.push_back(muon_peaks[0]);
					}
					else
					{
						for (int idx = 0; idx < muons_found; idx++)
						{
							muon_onset_arr.push_back(muon_peaks[idx]);
						}
					}
				}
				if (muon_peaks.size() > 3)
				{
					muon_veto_flag = true;
					muonVetoCtr += 1;
				}
				else { muon_peaks.resize(3, -1); }

				// Add PE peaks to peak height distribution
				if (peak_heights.size() > 0 && !overflow && !linear_gate)
				{
					for (int idx = 0; idx < peak_heights.size(); idx++)
					{
						peak_height_dist[peak_heights[idx] < 100 ? peak_heights[idx] : 99]++;
					}
				}

				// ========================================================================
				// Check that there is at least one PE in the trace, there is no overflow, 
				// no linear gate and no muon veto flag, otherwise continue to next
				// waveform without bothering to analyze this one
				// ========================================================================
				bool analyze = false;
				if (data_set == 1 && !overflow && !linear_gate && !muon_veto_flag) { analyze = true; }
				if (data_set == 2 && !overflow && !linear_gate) { analyze = true; }
				if (data_set == 3 && !overflow && !linear_gate) { analyze = true; }

				if (analyze)
				{
					// -------------------------------------------------------------
					// Integrate all SPE found in the PT and histogram their charge
					// -------------------------------------------------------------
					for (int idx = 0; idx <= pe_beginnings.size(); idx++)
					{
						if (pe_beginnings[idx] < BG_PT[1])
						{
							current_spe_q = 0;
							for (int i = pe_beginnings[idx] - 2; i < pe_endings[idx] + 2; i++)
							{
								if (i >= 0){ current_spe_q += csi[i]; }
							}
							if (current_spe_q >= -50 && current_spe_q < 250) { spe_charge_dist[(current_spe_q+50)] += 1; }
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
					if (peaks.size() > 0)
					{
						for (std::vector<int>::size_type idx = 0; idx != peaks.size(); idx++)
						{
							if (peaks[idx] >= BG_PT[0] && peaks[idx] < BG_PT[1]) { bg_pt_ct += 1; }
							if (peaks[idx] >= S_PT[0] && peaks[idx] < S_PT[1]) { s_pt_ct += 1; }
							if (peaks[idx] >= BG_ROI[0] && peaks[idx] < BG_ROI[1]) { bg_roi_ct += 1; }
							if (peaks[idx] >= S_ROI[0] && peaks[idx] < S_ROI[1]) { s_roi_ct += 1; }
						}

						// Distribution of peak onsets
						if (true)
						{
							int sz = peaks.size() / 2;
							if (sz < 350)
							{
								for (std::vector<int>::size_type idx = 0; idx != peaks.size(); idx++)
								{
									peak_distribution[sz][peaks[idx] / 100] += 1;
								}
							}
						}

						// Distribution of charge (only add >= 3)
						if (true)
						{
							int sz = peaks.size() / 2;
							if (sz < 350)
							{
								//for (int idx = 0; idx < 35000; idx++)
								//{
								//	charge_distribution[sz][idx / 100] += (csi[idx] >= 3) ? csi[idx] : 0;
								//}
								int cpi = 0;
								for (int idx = 0; idx < 35000; idx++)
								{
									// Add sample if it is within one of the PE regions identified previously
									if (idx >= pe_beginnings[cpi] && idx <= pe_endings[cpi])
									{
										charge_distribution[sz][idx / 100] += csi[idx];
										if (idx >= pe_endings[cpi]) { cpi += ((cpi + 1) < pe_beginnings.size()) ? 1 : 0; }
									}
								}
							}
						}
					}

					// Histogram baseline and peaks in pretrace
					if (!overflow && !linear_gate && !muon_veto_flag)
					{
						if (med_csi >= 85 && med_csi <= 115) { baseline_hist[med_csi - 85]++; }
						else { baseline_hist[31]++; }

						if (bg_pt_ct <= 50) { bg_pe_pt[bg_pt_ct]++; }
						else { bg_pe_pt[51]++; }

						if (s_pt_ct <= 50) { s_pe_pt[s_pt_ct]++; }
						else { s_pe_pt[51]++; }
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
						lnL_real = 0.0;
						lnL_flat = 0.0;
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
						idx_0 -= 5;

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

								// Add sample if it is within one of the PE regions identified previously as well as update loglikelihood estimators
								if (idx_w_offset >= pe_beginnings[i_pe] && idx_w_offset <= pe_endings[i_pe])
								{
									q_int += csi[idx_w_offset];
									lnL_real += lnL_pf_real[i] * csi[idx_w_offset];
									lnL_flat += lnL_pf_flat[i] * csi[idx_w_offset];
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
								_t1 = (double)bg_q_arr[i];
								_t2 = (double)bg_q_arr[i + 1];

								for (int j = 0; j < 3; j++)
								{
									if (_t1 < thresholds[j] && _t2 >= thresholds[j]){ bg_rt[j] = i + (thresholds[j] - _t1) / (_t2 - _t1); }
								}
							}

							// -------------------------------------------------------------
							//  If waveforms are being saved check whether cuts are passed
							// -------------------------------------------------------------
							if (save_waveforms)
							{
								int rt1090 = (bg_rt[2] - bg_rt[0]);
								int rt050 = bg_rt[1];

								bool bottom_left_cut = (rt1090 >= rt1090_bottom_left[0]) && (rt1090 <= rt1090_bottom_left[1]) && (rt050 >= rt050_bottom_left[0]) && (rt050 <= rt050_bottom_left[1]);
								bool upper_right_cut = (rt1090 >= rt1090_upper_right[0]) && (rt1090 <= rt1090_upper_right[1]) && (rt050 >= rt050_upper_right[0]) && (rt050 <= rt050_upper_right[1]);
								bool min_peak_cut = (bg_iw_ct >= minimum_no_peaks_iw);
								bool total_peak_cut = (peaks.size() >= no_total_peaks[0]) && (peaks.size() <= no_total_peaks[1]);

								passed_cuts_bg = (bottom_left_cut || upper_right_cut) && min_peak_cut && total_peak_cut;
							}
							// -------------------------------------------------------------
							//    Write analysis results to file
							// -------------------------------------------------------------
							bg_out_file << timestamp << " " << med_csi << " " << med_mv << " ";
							bg_out_file << bg_pt_ct << " " << bg_roi_ct << " ";
							bg_out_file << bg_iw_ct << " " << idx_0 << " " << bg_q_arr[1499] << " " << lnL_real << " " << lnL_flat << " ";
							bg_out_file << bg_rt[0] << " " << bg_rt[1] << " " << bg_rt[2] << " ";
							bg_out_file << muon_peaks[0] << " " << muon_peaks[1] << " " << muon_peaks[2] << " " << peaks.size() << std::endl;

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
						lnL_real = 0.0;
						lnL_flat = 0.0;
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
						idx_0 -= 5;

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

								// Add sample if it is within one of the PE regions identified previously & update loglikelihood estimators
								if (idx_w_offset >= pe_beginnings[i_pe] && idx_w_offset <= pe_endings[i_pe])
								{
									q_int += csi[idx_w_offset];
									lnL_real += lnL_pf_real[i] * csi[idx_w_offset];
									lnL_flat += lnL_pf_flat[i] * csi[idx_w_offset];
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
								_t1 = (double)s_q_arr[i];
								_t2 = (double)s_q_arr[i + 1];

								for (int j = 0; j < 3; j++)
								{
									if (_t1 < thresholds[j] && _t2 >= thresholds[j]){ s_rt[j] = i + (thresholds[j] - _t1) / (_t2 - _t1); }
								}
							}

							// -------------------------------------------------------------
							//  If waveforms are being saved check whether cuts are passed
							// -------------------------------------------------------------
							if (save_waveforms)
							{
								int rt1090 = (s_rt[2] - s_rt[0]);
								int rt050 = s_rt[1];

								bool bottom_left_cut = (rt1090 >= rt1090_bottom_left[0]) && (rt1090 <= rt1090_bottom_left[1]) && (rt050 >= rt050_bottom_left[0]) && (rt050 <= rt050_bottom_left[1]);
								bool upper_right_cut = (rt1090 >= rt1090_upper_right[0]) && (rt1090 <= rt1090_upper_right[1]) && (rt050 >= rt050_upper_right[0]) && (rt050 <= rt050_upper_right[1]);
								bool min_peak_cut = (s_iw_ct >= minimum_no_peaks_iw);
								bool total_peak_cut = (peaks.size() >= no_total_peaks[0]) && (peaks.size() <= no_total_peaks[1]);

								passed_cuts_s = (bottom_left_cut || upper_right_cut) && min_peak_cut && total_peak_cut;
							}
							// -------------------------------------------------------------
							//    Write analysis results to file
							// -------------------------------------------------------------
							s_out_file << timestamp << " " << med_csi << " " << med_mv << " ";
							s_out_file << s_pt_ct << " " << s_roi_ct << " ";
							s_out_file << s_iw_ct << " " << idx_0 << " " << s_q_arr[1499] << " " << lnL_real << " " << lnL_flat << " ";
							s_out_file << s_rt[0] << " " << s_rt[1] << " " << s_rt[2] << " ";
							s_out_file << muon_peaks[0] << " " << muon_peaks[1] << " " << muon_peaks[2] << " " << peaks.size() << std::endl;

							// Keeps track of how many BG waveforms have actually been analyzed
							s_counter++;
						}
					}
				}
				// -------------------------------------------------------------
				//  Save waveform if cuts have been passed for either ROI
				// -------------------------------------------------------------
				// if (save_waveforms && (passed_cuts_bg || passed_cuts_s))
				if (save_waveforms && passed_cut)
				{
					for (int idx = 0; idx < 35000; idx++)
					{
						waveformOut << csi_raw[idx] << " ";
					}
					/*for (int idx = 0; idx < 35000; idx++)
					{
						waveformOut << tmp_waveform[idx] << " ";
					}*/
					/*
					for (int idx = 0; idx < peaks.size(); idx++)
					{
						waveformOut << peaks[idx] << " ";
					}
					waveformOut << std::endl;*/
					waveformOut << gate_up << " " << gate_down << " " << med_csi << " ";
					for (int idx = 0; idx < pe_beginnings.size(); idx++)
					{
					waveformOut << pe_beginnings[idx] << " " << pe_endings[idx] << " ";
					}
					waveformOut << std::endl;
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
		infoOut << "Total number of Waveforms processed" << std::endl;
		infoOut << waveformCtr << std::endl;
		infoOut << "Number of triggers with linear gates in CsI channel" << std::endl;
		infoOut << linearGateCtr << std::endl;
		infoOut << "Number of triggers with overflows in either channel" << std::endl;
		infoOut << overflowCtr << std::endl;
		infoOut << "Number of triggers with more than 3 muons in muon veto channel" << std::endl;
		infoOut << muonVetoCtr << std::endl;
		infoOut << "Number of triggers that have actually been analyzed in the background window (less than x PE/peaks in PT + at least one PE/peak in ROI)" << std::endl;
		infoOut << bgAnalysisCtr << std::endl;
		infoOut << "Number of triggers that have actually been analyzed in the signal window (less than x PE/peaks in PT + at least one PE/peak in ROI)" << std::endl;
		infoOut << sAnalysisCtr << std::endl;
		infoOut << "Maximum number of PE/peaks in the pretrace" << std::endl;
		infoOut << PE_max_PT << std::endl;
		infoOut << "Background pretrace start" << std::endl;
		infoOut << BG_PT[0] << std::endl;
		infoOut << "Background pretrace stop" << std::endl;
		infoOut << BG_PT[1] << std::endl;
		infoOut << "Background ROI start" << std::endl;
		infoOut << BG_ROI[0] << std::endl;
		infoOut << "Background ROI stop" << std::endl;
		infoOut << BG_ROI[1] << std::endl;
		infoOut << "Signal pretrace start" << std::endl;
		infoOut << S_PT[0] << std::endl;
		infoOut << "Signal pretrace stop" << std::endl;
		infoOut << S_PT[1] << std::endl;
		infoOut << "Signal ROI start" << std::endl;
		infoOut << S_ROI[0] << std::endl;
		infoOut << "Signal ROI stop" << std::endl;
		infoOut << S_ROI[1] << std::endl;
		infoOut << "Unzipped file size" << std::endl;
		infoOut << fileSize << std::endl;
		infoOut << "SPE charge histogram" << std::endl;
		for (int idx = 0; idx < 300; idx++)
		{
			infoOut << spe_charge_dist[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "Background - Peaks in pretrace histogram" << std::endl;
		for (int idx = 0; idx < 52; idx++)
		{
			infoOut << bg_pe_pt[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "Signal - Peaks in pretrace histogram" << std::endl;
		for (int idx = 0; idx < 52; idx++)
		{
			infoOut << s_pe_pt[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "CsI baseline histogram" << std::endl;
		for (int idx = 0; idx < 32; idx++)
		{
			infoOut << baseline_hist[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "Peak width distribution" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << peak_width_distribution[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "PE amplitudes" << std::endl;
		for (int idx = 0; idx < 100; idx++)
		{
			infoOut << peak_height_dist[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "Big pulse onsets" << std::endl;
		for (int idx = 0; idx < bP_onset_arr.size(); idx++)
		{
			infoOut << bP_onset_arr[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "Big pulse charges" << std::endl;
		for (int idx = 0; idx < bP_charge_arr.size(); idx++)
		{
			infoOut << bP_charge_arr[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "All muon onsets - If linear gate present only the first is recorded" << std::endl;
		for (int idx = 0; idx < muon_onset_arr.size(); idx++)
		{
			infoOut << muon_onset_arr[idx] << " ";
		}
		infoOut << std::endl;
		infoOut << "Peak distribution in full waveform" << std::endl;
		for (int idx_1 = 0; idx_1 < 350; idx_1++)
		{
			bool printLine = false;
			for (int idx_2 = 0; idx_2 < 350; idx_2++)
			{
				if (peak_distribution[idx_1][idx_2] > 0)
				{
					printLine = true;
					break;
				}
			}
			if (printLine)
			{
				infoOut << idx_1 << " ";
				for (int idx_2 = 0; idx_2 < 350; idx_2++)
				{
					if (peak_distribution[idx_1][idx_2] > 0) { infoOut << idx_2 << " " << peak_distribution[idx_1][idx_2] << " "; }
				}
				infoOut << std::endl;
			}
		}
		infoOut << "Charge distribution in full waveform" << std::endl;
		for (int idx_1 = 0; idx_1 < 350; idx_1++)
		{
			bool printLine = false;
			for (int idx_2 = 0; idx_2 < 350; idx_2++)
			{
				if (charge_distribution[idx_1][idx_2] > 0)
				{
					printLine = true;
					break;
				}
			}
			if (printLine)
			{
				infoOut << idx_1 << " ";
				for (int idx_2 = 0; idx_2 < 350; idx_2++)
				{
					if (charge_distribution[idx_1][idx_2] > 0) { infoOut << idx_2 << " " << charge_distribution[idx_1][idx_2] << " "; }
				}
				infoOut << std::endl;
			}
		}
		infoOut.close();
	}
    return 0;
}