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

//==========================================================
//  Additional functions - Only string operations for now
//==========================================================
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

//==========================================================
//		Main
//==========================================================
int main(int argc, char* argv[])
{
	//==========================================================
	//  Variable declarations
	//==========================================================
    // Single byte to be read from the data files
    char c;
    
	// Counters to keep track of cuts
	unsigned long waveformCtr = 0;
	unsigned long linearGateCtr = 0;
	unsigned long muonVetoCtr = 0;
	unsigned long overflowCtr = 0;

	// Output file
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
	unsigned int med_csi_sum = 0;
	unsigned int med_mv_sum = 0;
	unsigned int med_csi_arr [256] = {} ;
	unsigned int med_mv_arr [256] = {};   
	bool med_csi_found = false;
	bool med_mv_found = false;

	// Linear gate detection
	int _previous_c = 0;
	int gate_down = 0;
	int gate_up = 0;

	// Cuts
	bool linear_gate = false;
	bool muon_veto_flag = false;
	bool overflow = false;

	// Muon locations in muon veto
	std::vector<int> muon_peaks; 
	int m_peak_width = 0;

	// Paths, etc.
	std::string main_dir;
	int current_time;
	int single_time;
	std::string out_dir;
	std::string current_zip_file;
	std::string time_name_in_zip;

	//==========================================================
	//		Pass arguments
	//==========================================================
	// Main run directory, e.g. Run-15-10-02-27-32-23/151002
	// Current time to be analzyed eg 145921 (i.e. 14:59:21)
	// Set output directory, eg ./Output/Run-15-10-02-27-32-23/151002
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

	//==========================================================
	//		Reading zipped data into memory
	//==========================================================
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
		current_zip_file = main_dir + "/" + zeroPad(current_time, 6) + ".zip";
		time_name_in_zip = zeroPad(current_time, 6);
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

	// Create output file
	std::ofstream out_file;
	out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "Data-")).c_str(), std::ofstream::out | std::ofstream::trunc);

	//==========================================================
	//		Begin processing data file
	//==========================================================
	// Begin data processing if zip file has been properly opened/unzipped into memory
	if (err == 0)
	{
		// Reset waveform counter
		waveformCtr = 0;

		// Reset current byte location
		zidx = 0;

		// Begin reading byte-stream
		while (zidx < fileSize)
		{
			// Read LabView header and get the total number of samples written for each channel in the next chunk of data
			for (int i = 0; i < 4; i++)
			{
				c = contents[zidx++];
				no_samples = no_samples << 8 | (unsigned char)c;
			}

			// Read LabView header and get the total number of channels written in next chunk of data (always 2 in our case)
			for (int i = 0; i < 4; i++)
			{
				c = contents[zidx++];
				no_channels = no_channels << 8 | (unsigned char)c;
			}

			// Takes care of LabViews closing bit issue.
			if (no_samples > 350070) { break; }

			//==========================================================
			//		Split current chunk into single waveoforms such
			//		that they can be properly analyzed
			//==========================================================
			for (int wf = 0; wf < (int)(no_samples / 35007); wf++)
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
				memset(med_csi_arr, 0, sizeof med_csi_arr);
				memset(med_mv_arr, 0, sizeof med_mv_arr);
				med_csi_found = false;
				med_mv_found = false;
				med_csi_sum = 0;
				med_mv_sum = 0;
				m_peak_width = 0;
				muon_peaks.clear();

				// -------------------------------------------------------------
				//  Read current timestamp
				// -------------------------------------------------------------
				for (int i = 0; i < 7; i++)
				{
					c = contents[zidx++];
					c = contents[zidx++];
					timestamp += zeroPad((int)c, 2);
				}

				// ---------------------------------------------------------------
				// Read the full CsI and muon veto waveforms from the zip-stream
				// + Apply bit transformation to get rid of zero bins
				// + Determine if a linear gate is present
				// ---------------------------------------------------------------
				for (int i = 0; i < 35000; i++)
				{
					// CsI
					c = contents[zidx++];
					_tmpC = (int)c - (int)floor(((double)c + 5.0) / 11.0);
					if (i < 20000){ med_csi_arr[_tmpC + 128] += 1; }
					csi[i] = _tmpC;
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
					_tmpC = (int)c + (int)((signbit((int)c) ? -1 : 1) * floor((4.0 - abs((double)c)) / 11.0));
					med_mv_arr[_tmpC + 128] += 1;
					mv[i] = _tmpC;
				}

				// ---------------------------------------
				//  Calculate median of both channels
				// ---------------------------------------
				for (int i = 0; i < 256; i++)
				{
					if (!med_csi_found)
					{
						med_csi_sum += med_csi_arr[i];
						if (med_csi_sum >= 10000)
						{
							med_csi = i - 128;
							med_csi_found = true;
						}
					}

					if (!med_mv_found)
					{
						med_mv_sum += med_mv_arr[i];
						if (med_mv_sum >= 10000)
						{
							med_mv = i - 128;
							med_mv_found = true;
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
					// Subtract baseline and invert muon veto waveform
					// Technically not necessary, but it's easier for me to wrap my head around certain algorithms
					mv[i] = med_mv - mv[i];

					// Peak finder - Threshold is 10 ADC counts above baseline and a minimum width of 3 samples
					if (mv[i] >= 10) { m_peak_width++; }
					else
					{
						if (m_peak_width >= 3) { muon_peaks.push_back(i - m_peak_width); }
						m_peak_width = 0;
					}
				}
				
				if (muon_peaks.size() > 0)
				{
					for (int i = 0; i < muon_peaks.size(); i++)
					{
						out_file << muon_peaks[i] << " " << (int) linear_gate << std::endl;
					}
				}
			}
		}
	}

	// Before exiting, make sure that the output file is properly closed to prevent data loss.
	if (out_file.is_open()) { out_file.close(); }
	return 0;
}