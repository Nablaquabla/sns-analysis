#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <string.h>
#include <cmath>
#include <dirent.h>
#include <algorithm>
#include <limits>
#include <zip.h>
#include <string>
#include <queue>
#include <array>
#include <numeric>

// Global constants
const unsigned int cmfWidth = 250;
const double cmfThreshold = 3.0;
const int peakFinderAmplitudeThreshold = 4;
const int peakFinderWidthThreshold = 3;
const bool saveWaveFormsToFile = false;

// Assortment of all distributions that are being tracked throughout the analysis
struct infoData
{
	// Vanilla analysis data
	std::array<int, 300> peakCharges;
	std::array<int, 51> peakAmplitudes;
	std::array<int, 51> peakWidths;
	std::array<int, 51> bPeaksInPretrace;
	std::array<int, 51> sPeaksInPretrace;
	std::array<int, 51> bPeaksInROI;
	std::array<int, 51> sPeaksInROI;
	std::array<int, 51> bPeaksInIW;
	std::array<int, 51> sPeaksInIW;

	// Conditional mean filtered analysis data
	std::array<int, 300> cmf_peakCharges;
	std::array<int, 51> cmf_peakAmplitudes;
	std::array<int, 51> cmf_peakWidths;
	std::array<int, 51> cmf_bPeaksInPretrace;
	std::array<int, 51> cmf_sPeaksInPretrace;
	std::array<int, 51> cmf_bPeaksInROI;
	std::array<int, 51> cmf_sPeaksInROI;
	std::array<int, 51> cmf_bPeaksInIW;
	std::array<int, 51> cmf_sPeaksInIW;

	// Local baseline analysis data
	std::array<int, 300> lbl_peakCharges;
	std::array<int, 51> lbl_peakAmplitudes;

	std::vector<int> muonVetoEvents;
	
	int waveformCounter;
	int linearGateCounter;
	int overflowCounter;
	int muonCounter;
	int summedBaseline;
};

void initializeInfoData(infoData &ID)
{
	ID.peakCharges = {};
	ID.peakAmplitudes = {};
	ID.peakWidths = {};
	ID.bPeaksInPretrace = {};
	ID.sPeaksInPretrace = {};
	ID.bPeaksInROI = {};
	ID.sPeaksInROI = {};
	ID.bPeaksInIW = {};
	ID.sPeaksInIW = {};

	ID.cmf_peakCharges = {};
	ID.cmf_peakAmplitudes = {};
	ID.cmf_peakWidths = {};
	ID.cmf_bPeaksInPretrace = {};
	ID.cmf_sPeaksInPretrace = {};
	ID.cmf_bPeaksInROI = {};
	ID.cmf_sPeaksInROI = {};
	ID.cmf_bPeaksInIW = {};
	ID.cmf_sPeaksInIW = {};

	ID.lbl_peakCharges = {};
	ID.lbl_peakAmplitudes = {};

	ID.muonVetoEvents.clear();

	ID.waveformCounter = 0;
	ID.linearGateCounter = 0;
	ID.overflowCounter = 0;
	ID.muonCounter = 0;
	ID.summedBaseline = 0;
}
// =========================================================================================================================
// Waveform object that does all the heavy lifting
// ===============================================
// Current analysis available:
// ---------------------------
// 1. Vanilla: No changes to old (v4) analysis
// 2. LBL - Local baseline: Peak detection as in vanilla anlysis. Peak integration uses a local baseline estimate.
// 3. CMF - Conditional mean filter: Baseline is estimated using a conditional mean filter. This heavily suppresses noise.
// =========================================================================================================================
class waveform
{
	public:
		// Set timestamp and flags (all determined when waveform is read in main())
		void setTimeStamp(std::string TS){ timeStamp = TS; }
		void setOverflowFlag(bool flag) { overflowFlag = flag; }
		void setLinearGateFlag(bool flag) { linearGateFlag = flag; }

		// Fill data arrays with samples. Bin correction applied in main(). Also fills median histograms
		void setCsIValue(int idx, int value)
		{
			csi[idx] = value;
			if (idx < 20000){ medianBaselineHistCsI[value + 128] += 1; }
		}
		void setMuonVetoValue(int idx, int value)
		{
			muonVeto[idx] = value;
			if (idx < 20000){ medianBaselineHistMuonVeto[value + 128] += 1; }
		}
		void cmf_setCsIValue(int idx, double value)
		{
			cmf_csi[idx] = value;
		}

		// Determine and subtract baseline for both CsI and Muon Veto. Only for Vanilla and LBL analysis
		void applyBaselineCorrection()
		{
			bool csiBaselineFound = false;
			int csiBaselineCounter = 0;

			bool muonVetoBaselineFound = false;
			int muonVetoBaselineCounter = 0;

			for (int i = 0; i < 256; i++)
			{
				if (!csiBaselineFound)
				{
					csiBaselineCounter += medianBaselineHistCsI[i];
					if (csiBaselineCounter >= 10000)
					{
						globalBaselineCsI = i - 128;
						csiBaselineFound = true;
					}
				}

				if (!muonVetoBaselineFound)
				{
					muonVetoBaselineCounter += medianBaselineHistMuonVeto[i];
					if (muonVetoBaselineCounter >= 10000)
					{
						globalBaselineMuonVeto = i - 128;
						muonVetoBaselineFound = true;
					}
				}
			}
			for (int i = 0; i < 35000; i++)
			{
				csi[i] = globalBaselineCsI - csi[i];
				muonVeto[i] = globalBaselineMuonVeto - muonVeto[i];
			}

			if (globalBaselineCsI < 50) { linearGateFlag = true; }
		}

		// Find muon veto events in Muon Veto trace
		void findMuonVetoPeaks(int peakAmplitudeThreshold, int peakWidthThreshold)
		{
			int currentPeakWidth = 0;

			// Cycle through full Muon Veto trace
			for (int i = 0; i < 35000; i++)
			{
				if (muonVeto[i] >= peakAmplitudeThreshold) { currentPeakWidth++; }
				else
				{
					if (currentPeakWidth >= peakWidthThreshold)
					{
						muonEvents.push_back(i - currentPeakWidth);
					}
					currentPeakWidth = 0;
				}
			}
			// Set muon veto flag if a muon has been identified
			if (muonEvents.size() > 0) { muonVetoFlag = true; }
		}

		// Find peaks in CsI trace (Both vanilla and CMF versions - Could be merged by passing an integration window offset and a pointer to the traces)
		void findCsIPeaks(int peakAmplitudeThreshold, int peakWidthThreshold)
		{
			int currentPeakWidth = 0;
			for (int i = 0; i < 35000; i++)
			{
				if (csi[i] >= peakAmplitudeThreshold) { currentPeakWidth++; }
				else
				{
					if (currentPeakWidth >= peakWidthThreshold)
					{
						// Begin of peak integration window is 2 samples before pos. threshold crossing
						peakBegin.push_back( ((i - currentPeakWidth - 2) >= 0 ? (i - currentPeakWidth - 2) : 0) );

						// End of peak integration window is 2 samples after neg. threshold crossing
						peakEnd.push_back( ((i + 1) <= 34999 ? (i + 1) : 34999) );
					}
					currentPeakWidth = 0;
				}
			}
		}
		void cmf_findCsIPeaks(int peakAmplitudeThreshold, int peakWidthThreshold)
		{
			int currentPeakWidth = 0;
			for (int i = 0; i < 35000 - cmfWidth; i++)
			{
				if (cmf_csi[i] >= peakAmplitudeThreshold) { currentPeakWidth++; }
				else
				{
					if (currentPeakWidth >= peakWidthThreshold)
					{
						// Begin of peak integration window is 3 samples before pos. threshold crossing
						cmf_peakBegin.push_back( ((i - currentPeakWidth - 3) >= 0 ? (i - currentPeakWidth - 3) : 0) );

						// End of peak integration window is 3 samples after neg. threshold crossing
						cmf_peakEnd.push_back( ((i + 2) <= 34999 - cmfWidth ? (i + 2) : 34999 - cmfWidth) );
					}
					currentPeakWidth = 0;
				}
			}
		}

		// Integrate each peak found and add charge to peakCharges histogram in infoData - Different for Vanilla, CMF, LBL
		void updateIntegratedCsIPeaks(infoData &cInData)
		{
			// Only analyze if there has been a peak identified in the trace
			if (peakBegin.size() > 0)
			{
				// Go through all peaks
				for (std::vector<int>::size_type idx = 0; idx < peakBegin.size(); idx++)
				{
					int charge = 0;
					int amplitude = -1;
					int width = peakEnd[idx] - peakBegin[idx] + 1;

					// Integrate samples within peak window
					for (int i = peakBegin[idx]; i <= peakEnd[idx]; i++)
					{
						charge += csi[i];
						if (csi[i] > amplitude){ amplitude = csi[i]; }
					}

					// Add peak properties to infoData
					if (charge >= -50 && charge < 250) { cInData.peakCharges[(charge + 50)] += 1; }
					cInData.peakAmplitudes[( (amplitude < 51) ? amplitude : 50 )] += 1;
					cInData.peakWidths[( (width < 51) ? width : 50 )] += 1;
				}
			}
		}
		void lbl_updateIntegratedCsIPeaks(infoData &cInData)
		{
			// Only analyze if there has been a peak identified in the trace
			if (peakBegin.size() > 0)
			{
				// Go through all peaks
				for (std::vector<int>::size_type idx = 0; idx < peakBegin.size(); idx++)
				{
					double _tCharge = 0.0;
					double _tAmplitude = -1;


					// Estimate local baseline
					double localBaseline = 0;
					int numSamples = 0;
					int iMin = ( (peakBegin[idx] - 75 >= 0) ? peakBegin[idx] - 75 : 0 );
					int iMax = ( (peakEnd[idx] + 75 < 35000) ? peakEnd[idx] + 75 : 34999 );
					for (int i = iMin; i < iMax; i++)
					{
						if (fabs(csi[i]) <= 3)
						{
							numSamples += 1;
							localBaseline += double(csi[i]);
						}
					}
					localBaseline /= double(numSamples);

					// Integrate samples within peak window
					for (int i = peakBegin[idx]; i <= peakEnd[idx]; i++)
					{
						_tCharge += csi[i]-localBaseline;
						if (csi[i]-localBaseline > _tAmplitude){ _tAmplitude = csi[i]-localBaseline; }
					}

					// Add peak properties to infoData - As local baseline type is double cast data to int first
					int charge = int(round(_tCharge));
					int amplitude = int(round(_tAmplitude));
					if (charge >= -50 && charge < 250) { cInData.lbl_peakCharges[(charge + 50)] += 1; }
					cInData.lbl_peakAmplitudes[( (amplitude < 51) ? amplitude : 50 )] += 1;
				}
			}
		}
		void cmf_updateIntegratedCsIPeaks(infoData &cInData)
		{
			// Only analyze if there has been a peak identified in the trace
			if (cmf_peakBegin.size() > 0)
			{
				// Go through all peaks
				for (std::vector<int>::size_type idx = 0; idx < cmf_peakBegin.size(); idx++)
				{
					double _tCharge = 0.0;
					double _tAmplitude = -1;
					int width = cmf_peakEnd[idx] - cmf_peakBegin[idx] + 1;

					// Integrate samples within peak window
					for (int i = cmf_peakBegin[idx]; i <= cmf_peakEnd[idx]; i++)
					{
						_tCharge += cmf_csi[i];
						if (cmf_csi[i] > _tAmplitude){ _tAmplitude = cmf_csi[i]; }
					}

					// Add peak properties to infoData - As local baseline type is double cast data to int first
					int charge = int(round(_tCharge));
					int amplitude = int(round(_tAmplitude));
					if (charge >= -50 && charge < 250) { cInData.cmf_peakCharges[(charge + 50)] += 1; }
					cInData.cmf_peakAmplitudes[( (amplitude < 51) ? amplitude : 50 )] += 1;
					cInData.cmf_peakWidths[( (width < 51) ? width : 50 )] += 1;
				}
			}
		}

		// Determine number of peaks in different regions of the trace, i.e. B-PT, B-ROI, S-PT, S-ROI (# of peaks in IW is determined later)
		void countPeaksPerRegion()
		{
			// Check that there is indeed at least one peak present in the trace
			if (peakBegin.size() > 0)
			{
				// For each peak check in which region its onset is actually located
				for (std::vector<int>::size_type idx = 0; idx < peakBegin.size(); idx++)
				{
					// Integration window onset has to be shifted by 2 samples to get pos. threshold crossing
					int onset = peakBegin[idx] + 2;

					if (onset >= bRegionLimits[0] && onset < bRegionLimits[1]) { bPeakCounts[0] += 1; }
					if (onset >= bRegionLimits[2] && onset < bRegionLimits[3]) { bPeakCounts[1] += 1; }

					if (onset >= sRegionLimits[0] && onset < sRegionLimits[1]) { sPeakCounts[0] += 1; }
					if (onset >= sRegionLimits[2] && onset < sRegionLimits[3]) { sPeakCounts[1] += 1; }
				}
			}
		}
		void cmf_countPeaksPerRegion()
		{
			// Check that there is indeed at least one peak present in the trace
			if (cmf_peakBegin.size() > 0)
			{
				// For each peak check in which region its onset is actually located
				for (std::vector<int>::size_type idx = 0; idx < cmf_peakBegin.size(); idx++)
				{
					// Integration window onset has to be shifted by 3 samples to get pos. threshold crossing
					int onset = cmf_peakBegin[idx] + 3;

					if (onset >= cmf_bRegionLimits[0] && onset < cmf_bRegionLimits[1]) { cmf_bPeakCounts[0] += 1; }
					if (onset >= cmf_bRegionLimits[2] && onset < cmf_bRegionLimits[3]) { cmf_bPeakCounts[1] += 1; }

					if (onset >= cmf_sRegionLimits[0] && onset < cmf_sRegionLimits[1]) { cmf_sPeakCounts[0] += 1; }
					if (onset >= cmf_sRegionLimits[2] && onset < cmf_sRegionLimits[3]) { cmf_sPeakCounts[1] += 1; }
				}
			}
		}

		// Analyze region of interest for all different analysis types.
		void analyzeROIWindowVanillaStyle(bool signalRegion)
		{
			// Set correct window parameters based on signal/background region
			int peaksInROI = (signalRegion ? sPeakCounts[1] : bPeakCounts[1]);
			int beginROI = (signalRegion ? sRegionLimits[2] : bRegionLimits[2]);
			int endROI = (signalRegion ? sRegionLimits[3] : bRegionLimits[3]);

			// Only analyze data if there is at least one PE in the ROI
			if (peaksInROI > 0)
			{
				int arrivalIndex = -1;
				int peakIndex = -1;

				// Find the first Peak in the ROI
				for (int i = 0; i < peakBegin.size(); i++)
				{
					if (peakBegin[i] >= beginROI)
					{
						arrivalIndex = peakBegin[i];
						peakIndex = i;
						break;
					}
				}
				(signalRegion ? sArrivalIndex : bArrivalIndex) = arrivalIndex;

				// Check that the full IW fits in ROI (i.e. onset happens in the first 6000 samples of ROI)
				if (arrivalIndex < (endROI - 1500))
				{
					// Determine number of peaks in IW
					for (int i = peakIndex; i < peakBegin.size(); i++)
					{
						// Add a shift of 2 to the integration window beginning to get proper peak onset
						(signalRegion ? sPeakCounts[2] : bPeakCounts[2]) = (signalRegion ? sPeakCounts[2] : bPeakCounts[2]) + int((peakBegin[i] + 2 - arrivalIndex) < 1500);
					}

					// Integrate over all peaks in IW
					int _tPeakIndex = peakIndex;
					double _tCharge = 0;
					int wfIndex = 0;
					for (int i = 0; i < 1500; i++)
					{
						// Get proper index in 'waveform space'
						wfIndex = i + arrivalIndex;

						// Add sample if it is within one of the PE regions identified previously
						if (_tPeakIndex < peakBegin.size() && wfIndex >= peakBegin[_tPeakIndex] && wfIndex <= peakEnd[_tPeakIndex])
						{
							_tCharge += csi[wfIndex];

							// If end of current peak integration window has been reached go to next peak
							if (wfIndex == peakEnd[_tPeakIndex]) { _tPeakIndex++; }
						}
						integratedCharge[i] = _tCharge;
					}

					// Save total charge in IW
					(signalRegion ? sChargeIW : bChargeIW) = int(round(integratedCharge[1499]));

					// Calculate charge thresholds for risetime analysis
					double thresholds[3];
					thresholds[0] = 0.1*integratedCharge[1499];
					thresholds[1] = 0.5*integratedCharge[1499];
					thresholds[2] = 0.9*integratedCharge[1499];

					// Determine risetimes
					double _t1 = 0;
					double _t2 = 0;
					for (int i = 0; i < 1499; i++)
					{
						_t1 = integratedCharge[i];
						_t2 = integratedCharge[i + 1];

						for (int j = 0; j < 3; j++)
						{
							if (_t1 < thresholds[j] && _t2 >= thresholds[j])
							{
								// Linear extrapolation between bins
								(signalRegion ? sRiseTimes[j] : bRiseTimes[j]) = i + (thresholds[j] - _t1) / (_t2 - _t1);
							}
						}
					}
				}
			}
		}
		void analyzeROIWindowCMFStyle(bool signalRegion)
		{
			// Set correct window parameters based on signal/background region
			int peaksInROI = (signalRegion ? cmf_sPeakCounts[1] : cmf_bPeakCounts[1]);
			int beginROI = (signalRegion ? cmf_sRegionLimits[2] : cmf_bRegionLimits[2]);
			int endROI = (signalRegion ? cmf_sRegionLimits[3] : cmf_bRegionLimits[3]);

			// Only analyze data if there is at least one PE in the ROI
			if (peaksInROI > 0)
			{
				int arrivalIndex = -1;
				int peakIndex = -1;

				// Find the first Peak in the ROI
				for (int i = 0; i < cmf_peakBegin.size(); i++)
				{
					if (cmf_peakBegin[i] >= beginROI)
					{
						arrivalIndex = cmf_peakBegin[i];
						peakIndex = i;
						break;
					}
				}
				(signalRegion ? cmf_sArrivalIndex : cmf_bArrivalIndex) = arrivalIndex;

				// Check that the full IW fits in ROI (i.e. onset happens in the first 6000 samples of ROI)
				if (arrivalIndex < (endROI - 1500))
				{
					// Determine number of peaks in IW
					for (int i = peakIndex; i < cmf_peakBegin.size(); i++)
					{
						// Add a shift of 3 to the integration window beginning to get proper peak onset
						(signalRegion ? cmf_sPeakCounts[2] : cmf_bPeakCounts[2]) = (signalRegion ? cmf_sPeakCounts[2] : cmf_bPeakCounts[2]) + int(cmf_peakBegin[i] + 3 - arrivalIndex < 1500);
					}

					// Integrate over all peaks in IW
					int _tPeakIndex = peakIndex;
					double _tCharge = 0;
					int wfIndex = 0;
					for (int i = 0; i < 1500; i++)
					{
						// Get proper index in 'waveform space'
						wfIndex = i + arrivalIndex;

						// Add sample if it is within one of the PE regions identified previously
						if (_tPeakIndex < cmf_peakBegin.size() && wfIndex >= cmf_peakBegin[_tPeakIndex] && wfIndex <= cmf_peakEnd[_tPeakIndex])
						{
							_tCharge += cmf_csi[wfIndex];

							// If end of current peak integration window has been reached go to next peak
							if (wfIndex == cmf_peakEnd[_tPeakIndex]) { _tPeakIndex++; }
						}
						integratedCharge[i] = _tCharge;
					}

					// SAve total charge in IW
					(signalRegion ? cmf_sChargeIW : cmf_bChargeIW) = integratedCharge[1499];

					// Calculate charge thresholds for risetime analysis
					double thresholds[3];
					thresholds[0] = 0.1*integratedCharge[1499];
					thresholds[1] = 0.5*integratedCharge[1499];
					thresholds[2] = 0.9*integratedCharge[1499];

					// Determine threshold crossing times
					double _t1 = 0;
					double _t2 = 0;
					for (int i = 0; i < 1499; i++)
					{
						_t1 = integratedCharge[i];
						_t2 = integratedCharge[i + 1];

						for (int j = 0; j < 3; j++)
						{
							if (_t1 < thresholds[j] && _t2 >= thresholds[j])
							{
								(signalRegion ? cmf_sRiseTimes[j] : cmf_bRiseTimes[j]) = i + (thresholds[j] - _t1) / (_t2 - _t1);
							}
						}
					}
				}
			}
		}
		void analyzeROIWindowLBLStyle(bool signalRegion)
		{
			// Set correct window parameters based on signal/background region
			int peaksInROI = (signalRegion ? sPeakCounts[1] : bPeakCounts[1]);
			int beginROI = (signalRegion ? sRegionLimits[2] : bRegionLimits[2]);
			int endROI = (signalRegion ? sRegionLimits[3] : bRegionLimits[3]);

			// Only analyze data if there is at least one PE in the ROI
			if (peaksInROI > 0)
			{
				int arrivalIndex = -1;
				int peakIndex = -1;

				// Find the first Peak in the ROI
				for (int i = 0; i < peakBegin.size(); i++)
				{
					if (peakBegin[i] >= beginROI)
					{
						arrivalIndex = peakBegin[i];
						peakIndex = i;
						break;
					}
				}

				// Check that the full IW fits in ROI (i.e. onset happens in the first 6000 samples of ROI)
				if (arrivalIndex < (endROI - 1500))
				{
					// Estimate local baseline based on waveform 1 us before the arrival
					double localBaseline = 0;
					int numSamples = 0;
					for (int i = arrivalIndex - 500; i < arrivalIndex; i++)
					{
						if (fabs(csi[i]) <= 3)
						{
							localBaseline += double(csi[i]);
							numSamples++;
						}
					}
					localBaseline /= double(numSamples);

					// Integrate over all peaks in IW
					int _tPeakIndex = peakIndex;
					double _tCharge = 0;
					int wfIndex = 0;
					for (int i = 0; i < 1500; i++)
					{
						// Get proper index in 'waveform space'
						wfIndex = i + arrivalIndex;

						// Add sample if it is within one of the PE regions identified previously
						if (_tPeakIndex < peakBegin.size() && wfIndex >= peakBegin[_tPeakIndex] && wfIndex <= peakEnd[_tPeakIndex])
						{
							_tCharge += (csi[wfIndex] - localBaseline);

							// If end of current peak integration window has been reached go to next peak
							if (wfIndex == peakEnd[_tPeakIndex]) { _tPeakIndex++; }
						}
						integratedCharge[i] = _tCharge;
					}

					// Save total charge in IW
					(signalRegion ? lbl_sChargeIW : lbl_bChargeIW) = integratedCharge[1499];

					// Calculate charge thresholds for risetime analysis
					double thresholds[3];
					thresholds[0] = 0.1*integratedCharge[1499];
					thresholds[1] = 0.5*integratedCharge[1499];
					thresholds[2] = 0.9*integratedCharge[1499];

					// Determine threshold crossing times
					double _t1 = 0;
					double _t2 = 0;
					for (int i = 0; i < 1499; i++)
					{
						_t1 = integratedCharge[i];
						_t2 = integratedCharge[i + 1];

						for (int j = 0; j < 3; j++)
						{
							if (_t1 < thresholds[j] && _t2 >= thresholds[j])
							{
								(signalRegion ? lbl_sRiseTimes[j] : lbl_bRiseTimes[j]) = i + (thresholds[j] - _t1) / (_t2 - _t1);
							}
						}
					}
				}
			}
		}

		// Update info data with internal data
		void updateInfoData(infoData &cInData)
		{
			// Update peaks in PT, ROI and IW histograms
			cInData.bPeaksInPretrace[((bPeakCounts[0] < 50) ? bPeakCounts[0] : 50)] += 1;
			cInData.bPeaksInROI[((bPeakCounts[1] < 50) ? bPeakCounts[1] : 50)] += 1;
			cInData.bPeaksInIW[((bPeakCounts[2] < 50) ? bPeakCounts[2] : 50)] += 1;
			cInData.sPeaksInPretrace[((sPeakCounts[0] < 50) ? sPeakCounts[0] : 50)] += 1;
			cInData.sPeaksInROI[((sPeakCounts[1] < 50) ? sPeakCounts[1] : 50)] += 1;
			cInData.sPeaksInIW[((sPeakCounts[2] < 50) ? sPeakCounts[2] : 50)] += 1;

			cInData.cmf_bPeaksInPretrace[((cmf_bPeakCounts[0] < 50) ? cmf_bPeakCounts[0] : 50)] += 1;
			cInData.cmf_bPeaksInROI[((cmf_bPeakCounts[1] < 50) ? cmf_bPeakCounts[1] : 50)] += 1;
			cInData.cmf_bPeaksInIW[((cmf_bPeakCounts[2] < 50) ? cmf_bPeakCounts[2] : 50)] += 1;
			cInData.cmf_sPeaksInPretrace[((cmf_sPeakCounts[0] < 50) ? cmf_sPeakCounts[0] : 50)] += 1;
			cInData.cmf_sPeaksInROI[((cmf_sPeakCounts[1] < 50) ? cmf_sPeakCounts[1] : 50)] += 1;
			cInData.cmf_sPeaksInIW[((cmf_sPeakCounts[2] < 50) ? cmf_sPeakCounts[2] : 50)] += 1;

			// Update counters
			cInData.linearGateCounter += ((linearGateFlag) ? 1 : 0);
			cInData.overflowCounter += ((overflowFlag) ? 1 : 0);
			cInData.muonCounter += ((muonVetoFlag) ? 1 : 0);

			// Update muon veto vector in infoData
			if (muonEvents.size() > 0 && !linearGateFlag)
			{
				for (std::vector<int>::size_type i = 0; i != muonEvents.size(); i++) { cInData.muonVetoEvents.push_back(muonEvents[i]); }
			}

			// Add global baseline
			cInData.summedBaseline += globalBaselineCsI;
		}

		// Write internal data to file
		void writeEventData(std::ofstream &bOutput, std::ofstream &sOutput)
		{
			// Write background data
			int muonLocation = ((muonEvents.size() > 0) ? muonEvents[0] : -1);

			bOutput << timeStamp << " " << int(overflowFlag) << " " << int(muonVetoFlag) << " " << int(linearGateFlag) << " " << globalBaselineCsI << " ";
			bOutput << bPeakCounts[0] << " " << bPeakCounts[1] << " " << bPeakCounts[2] << " " << bArrivalIndex << " " << bChargeIW << " " << bRiseTimes[0] << " " << bRiseTimes[1] << " " << bRiseTimes[2] << " ";
			bOutput << cmf_bPeakCounts[0] << " " << cmf_bPeakCounts[1] << " " << cmf_bPeakCounts[2] << " " << cmf_bArrivalIndex << " " << cmf_bChargeIW << " " << cmf_bRiseTimes[0] << " " << cmf_bRiseTimes[1] << " " << cmf_bRiseTimes[2] << " ";
			bOutput << lbl_bChargeIW << " " << lbl_bRiseTimes[0] << " " << lbl_bRiseTimes[1] << " " << lbl_bRiseTimes[2] << " ";
			bOutput << muonLocation << std::endl;

			// Write signal data
			sOutput << timeStamp << " " << int(overflowFlag) << " " << int(muonVetoFlag) << " " << int(linearGateFlag) << " " << globalBaselineCsI << " ";
			sOutput << sPeakCounts[0] << " " << sPeakCounts[1] << " " << sPeakCounts[2] << " " << sArrivalIndex << " " << sChargeIW << " " << sRiseTimes[0] << " " << sRiseTimes[1] << " " << sRiseTimes[2] << " ";
			sOutput << cmf_sPeakCounts[0] << " " << cmf_sPeakCounts[1] << " " << cmf_sPeakCounts[2] << " " << cmf_sArrivalIndex << " " << cmf_sChargeIW << " " << cmf_sRiseTimes[0] << " " << cmf_sRiseTimes[1] << " " << cmf_sRiseTimes[2] << " ";
			sOutput << lbl_sChargeIW << " " << lbl_sRiseTimes[0] << " " << lbl_sRiseTimes[1] << " " << lbl_sRiseTimes[2] << " ";
			sOutput << muonLocation << std::endl;
		}

		// If needed write waveforms and peak locations to file
		void writeWaveformToFile(std::ofstream &wOutput)
		{
			for (int i = 0; i < 35000; i++)
			{
				wOutput << csi[i] << " ";
			}
			wOutput << std::endl;

			for (int i = 0; i < 35000; i++)
			{
				wOutput << cmf_csi[i] << " ";
			}
			wOutput << std::endl;
			wOutput << globalBaselineCsI << std::endl;
			if (peakBegin.size() > 0)
			{
				for (int i = 0; i < peakBegin.size(); i++)
				{
					wOutput << peakBegin[i] << " ";
				}
			}
			else
			{
				wOutput << -1;
			}
			wOutput << std::endl;

			if (cmf_peakBegin.size() > 0)
			{
				for (int i = 0; i < cmf_peakBegin.size(); i++)
				{
					wOutput << cmf_peakBegin[i] << " ";
				}
			}
			else
			{
				wOutput << -1;
			}
			wOutput << std::endl;
		}

		bool getLinearGateFlag()
		{
			return linearGateFlag;
		}
		// Constructor - Destructor
		waveform(std::array<unsigned int, 4> bRegions, std::array<unsigned int, 4> sRegions)
		{
			for (int i = 0; i < 4; i++)
			{
				bRegionLimits[i] = bRegions[i];
				sRegionLimits[i] = sRegions[i];
				cmf_bRegionLimits[i] = bRegions[i];
				cmf_sRegionLimits[i] = sRegions[i];
			}

			globalBaselineCsI = 0;
			globalBaselineMuonVeto = 0;

			bChargeIW = 0;
			sChargeIW = 0;
			bArrivalIndex = -1;
			sArrivalIndex = -1;

			cmf_bChargeIW = 0.0;
			cmf_sChargeIW = 0.0;
			cmf_bArrivalIndex = -1;
			cmf_sArrivalIndex = -1;

			lbl_bChargeIW = 0.0;
			lbl_sChargeIW = 0.0;

			overflowFlag = false;
			muonVetoFlag = false;
			linearGateFlag = false;

			csi = {};
			muonVeto = {};
			cmf_csi = {};
			medianBaselineHistCsI = {};
			medianBaselineHistMuonVeto = {};

			peakBegin.clear();
			peakEnd.clear();
			cmf_peakBegin.clear();
			cmf_peakEnd.clear();
			muonEvents.clear();

			bPeakCounts = {};
			sPeakCounts = {};
			bRiseTimes = {};
			sRiseTimes = {};

			lbl_bRiseTimes = {};
			lbl_sRiseTimes = {};

			cmf_bPeakCounts = {};
			cmf_sPeakCounts = {};
			cmf_bRiseTimes = {};
			cmf_sRiseTimes = {};

			integratedCharge = {};
		}
		~waveform(){};

	private:
		std::string timeStamp;
		int globalBaselineCsI;
		int globalBaselineMuonVeto;

		int bChargeIW;
		int sChargeIW;
		int bArrivalIndex;
		int sArrivalIndex;

		double lbl_bChargeIW;
		double lbl_sChargeIW;

		double cmf_bChargeIW;
		double cmf_sChargeIW;
		int cmf_bArrivalIndex;
		int cmf_sArrivalIndex;

		bool overflowFlag;
		bool muonVetoFlag;
		bool linearGateFlag;

		std::array<int, 35000> csi;
		std::array<int, 35000> muonVeto;
		std::array<double, 35000> cmf_csi;
		std::array<unsigned int, 257> medianBaselineHistCsI;
		std::array<unsigned int, 257> medianBaselineHistMuonVeto;

		std::vector<int> peakBegin;
		std::vector<int> peakEnd;
		std::vector<int> cmf_peakBegin;
		std::vector<int> cmf_peakEnd;
		std::vector<int> muonEvents;

		std::array<unsigned int, 4> bRegionLimits;
		std::array<unsigned int, 4> sRegionLimits;
		std::array<unsigned int, 3> bPeakCounts;
		std::array<unsigned int, 3> sPeakCounts;
		std::array<double, 3> bRiseTimes;
		std::array<double, 3> sRiseTimes;

		std::array<double, 3> lbl_bRiseTimes;
		std::array<double, 3> lbl_sRiseTimes;

		std::array<unsigned int, 4> cmf_bRegionLimits;
		std::array<unsigned int, 4> cmf_sRegionLimits;
		std::array<unsigned int, 3> cmf_bPeakCounts;
		std::array<unsigned int, 3> cmf_sPeakCounts;
		std::array<double, 3> cmf_bRiseTimes;
		std::array<double, 3> cmf_sRiseTimes;

		std::array<double, 1500> integratedCharge;
};

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

	// Waveform counter to keep track of how many triggers have been analyzed
	unsigned long waveformCtr = 0;

	// Output streams
	std::ofstream bg_out_file;
	std::ofstream s_out_file;
	std::ofstream w_out_file;
	std::ofstream infoOut;

	// Setup distribution histories
	infoData cInfoData;
	initializeInfoData(cInfoData);

	// LabView headers
	int no_samples = 0;
	int no_channels = 0;

	// Timestamp of current trigger
	std::string timestamp;

	// Conditional mean filter setup
	double _cmfC = 0;
	double cmfBL = 0;

	// Buffer to store bit shifted samples
	int _tmpC = 0;

	// Rising and falling threshold crossings used for linear gate detection
	int _previous_c = 0;
	int gate_down = 0;
	int gate_up = 0;

	// Determine pretrace regions and ROIs based on data set chosen
	unsigned int BG_PT[2] = {};
	unsigned int BG_ROI[2] = {};
	unsigned int S_PT[2] = {};
	unsigned int S_ROI[2] = {};

	// Prepare zip read
	std::string main_dir;
	int current_time;
	int single_time;
	std::string out_dir;
	std::string current_zip_file;
	std::string time_name_in_zip;

	// Set main run directory, e.g. Run-15-10-02-27-32-23/151002
	// Set current time to be analzyed as index of sorted number of total files in folder, e.g. 0-1439 for a full day
	// Set output directory, eg Output/ Run-15-10-02-27-32-23/151002
	int data_set = 0;
	int PE_max_PT = 100000;

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

	switch (data_set)
	{
	case 1: BG_PT[0] = cmfWidth;
		BG_PT[1] = 19950;
		BG_ROI[0] = 19950;
		BG_ROI[1] = 27475;
		S_PT[0] = 7525 + cmfWidth;
		S_PT[1] = 27475;
		S_ROI[0] = 27475;
		S_ROI[1] = 35000;
		PE_max_PT = 100000;
		break;
	case 2: BG_PT[0] = 0;
		BG_PT[1] = 20000;
		BG_ROI[0] = 20000;
		BG_ROI[1] = 27400;
		S_PT[0] = 7400;
		S_PT[1] = 27400;
		S_ROI[0] = 27400;
		S_ROI[1] = 35000;
		PE_max_PT = 20;
		break;
	case 3: BG_PT[0] = 0;
		BG_PT[1] = 17500;
		BG_ROI[0] = 17500;
		BG_ROI[1] = 25000;
		S_PT[0] = 7500;
		S_PT[1] = 25000;
		S_ROI[0] = 25000;
		S_ROI[1] = 32500;
		PE_max_PT = 30;
		break;
	default: std::cout << "Arguments not matching! Aborting now!" << std::endl;
		return 1;
	}

	// Prepare array to be passed to waveform objects
	std::array<unsigned int, 4> bRegions = { BG_PT[0], BG_PT[1], BG_ROI[0], BG_ROI[1] };
	std::array<unsigned int, 4> sRegions = { S_PT[0], S_PT[1], S_ROI[0], S_ROI[1] };

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

	// Unzip the compressed file into memory
	zip_file *f = zip_fopen(z, time_name_in_zip.c_str(), 0);
	fileSize = st.size;
	zip_fread(f, contents, fileSize);
	zip_fclose(f);

	//And close the archive
	zip_close(z);

	// Create signal, background and info output files
	bg_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "B-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	s_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "S-")).c_str(), std::ofstream::out | std::ofstream::trunc);
	if (saveWaveFormsToFile) { w_out_file.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "W-")).c_str(), std::ofstream::out | std::ofstream::trunc); }
	infoOut.open((out_dir + "/" + fileName(atoi(time_name_in_zip.c_str()), "I-")).c_str(), std::ofstream::out | std::ofstream::trunc);

	// Begin data processing if file has been properly opened
	if(err == 0)
	{
		// Reset waveform counter
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

			//  Process consecutive waveforms without encountering another LabView header inbetween
			for (int wf=0; wf < (int) (no_samples/35007); wf++)
			{
				// A new waveform begins, update counter and create waveform object
			 	cInfoData.waveformCounter += 1;
				waveform currentWaveForm(bRegions, sRegions);

				// Update timestamp
				timestamp.clear();
				for(int i=0; i<7; i++)
				{
					c = contents[zidx++];
					c = contents[zidx++];
					timestamp += zeroPad((int) c, 2);
				}
				currentWaveForm.setTimeStamp(timestamp);

				// Bit corrected waveform value
				_tmpC = 0;

				// Reset conditional mean filter
				_cmfC = 0;
				std::queue<double> cmfQ;
				double _fTmpC = 0.0;
				cmfBL = 0;
				
				// Reset linear gate detection
				_previous_c = 128;
				gate_down = 0;
				gate_up = 0;

				// Read CsI and MV data from binary stream
				for(int i=0; i<35000; i++)
				{
					// Read CsI value and apply bit correction
					c = contents[zidx++];
					_tmpC = (int) c - (int) floor(((double) c + 5.0)/11.0);
					_fTmpC = double(_tmpC);

					// Preload CMF filter
					if (i < cmfWidth)
					{
						cmfQ.push(_fTmpC);
						cmfBL += _fTmpC;
					}

					// CMF filter ready
					else
					{
						// Get average of current prewindow
						double m = cmfBL / double(cmfWidth);

						// No dominant feature present
						if (fabs(_fTmpC - m) < cmfThreshold)
						{
							_cmfC = 0;
							cmfQ.push(_fTmpC);
							cmfBL += _fTmpC;
						}

						// Feature present
						else
						{
							_cmfC = m - _fTmpC;
							cmfQ.push(m);
							cmfBL += m;
						}
						cmfBL -= cmfQ.front();
						cmfQ.pop();
					}

					// Fill waveform object with bin corrected and filtered CsI data
					currentWaveForm.setCsIValue(i, _tmpC);
					if (i >= cmfWidth) { currentWaveForm.cmf_setCsIValue(i, _cmfC); }
					
					// Preload linear gate detection algorithm
					if (i == 0) { _previous_c = _tmpC; }

					// Gate check
					if (_tmpC <= 18 && _previous_c > 18) { gate_down++; }
					if (_previous_c <= 18 && _tmpC > 18) { gate_up++; }
					_previous_c = _tmpC;

					// Overflow check
					if (c >= 127 || c == -128) { currentWaveForm.setOverflowFlag(true); }

					// Read muon veto data and apply correction
					c = contents[zidx++];
					_tmpC = (int) c + (int) ((std::signbit((int) c) ? -1 : 1 ) * floor((4.0 - abs((double) c))/11.0));
					currentWaveForm.setMuonVetoValue(i, _tmpC);
				}

				// Set linear gate flag if gated
				if (gate_down != gate_up) { currentWaveForm.setLinearGateFlag(true); }

				// Apply global baseline correction 
				currentWaveForm.applyBaselineCorrection();

				
				// Find all peaks in CsI and Muon Veto data (peak width and threshold for CsI trace is defined at the beginning of this file; the ones for the muon veto are fixed)
				currentWaveForm.findCsIPeaks(peakFinderAmplitudeThreshold, peakFinderWidthThreshold);
				currentWaveForm.cmf_findCsIPeaks(peakFinderAmplitudeThreshold, peakFinderWidthThreshold);
				currentWaveForm.findMuonVetoPeaks(10, 3);

				// Determine peaks per PT/ROI region
				currentWaveForm.countPeaksPerRegion();
				currentWaveForm.cmf_countPeaksPerRegion();

				// Get SPE charge distributions
				currentWaveForm.updateIntegratedCsIPeaks(cInfoData);
				currentWaveForm.cmf_updateIntegratedCsIPeaks(cInfoData);
				currentWaveForm.lbl_updateIntegratedCsIPeaks(cInfoData);

				// Analyze ROIs Vanilla Style
				currentWaveForm.analyzeROIWindowVanillaStyle(false); // Background region
				currentWaveForm.analyzeROIWindowVanillaStyle(true); // Signal region

				// Analyze ROIs CMF Style
				currentWaveForm.analyzeROIWindowCMFStyle(false); // Background region
				currentWaveForm.analyzeROIWindowCMFStyle(true); // Signal region

				// Analyze ROIs LBL Style
				currentWaveForm.analyzeROIWindowLBLStyle(false); // Background region
				currentWaveForm.analyzeROIWindowLBLStyle(true); // Signal region

				// Update info data
				currentWaveForm.updateInfoData(cInfoData);

				// Write analysis results to file
				currentWaveForm.writeEventData(bg_out_file, s_out_file);

				// Write first 10 waveforms to file if desired
				// if (saveWaveFormsToFile){ currentWaveForm.writeWaveformToFile(w_out_file); }
				// if (saveWaveFormsToFile && cInfoData.waveformCounter <= 10){ currentWaveForm.writeWaveformToFile(w_out_file); }
				if (saveWaveFormsToFile && currentWaveForm.getLinearGateFlag()){ currentWaveForm.writeWaveformToFile(w_out_file); }
			}
		}
	}

	// Before exiting, make sure that both output files are properly closed to prevent data loss.
	if (bg_out_file.is_open()) { bg_out_file.close(); }
	if (s_out_file.is_open()) { s_out_file.close(); }
	if (saveWaveFormsToFile && w_out_file.is_open()) { w_out_file.close(); }

	// Write run info
	if (infoOut.is_open())
	{
		// Counters and average baseline
		infoOut << cInfoData.waveformCounter << " " << cInfoData.linearGateCounter << " " << cInfoData.overflowCounter << " " << cInfoData.muonCounter << " " << double(cInfoData.summedBaseline) / double(cInfoData.waveformCounter) << std::endl;

		// Plain vanilla analysis
		infoOut << "Peak charge histogram" << std::endl;
		for (int idx = 0; idx < 300; idx++)
		{
			infoOut << cInfoData.peakCharges[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peak amplitude histogram" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.peakAmplitudes[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peak width histogram" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.peakWidths[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in B PT" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.bPeaksInPretrace[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in B ROI" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.bPeaksInROI[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in B IW" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.bPeaksInIW[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in S PT" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.sPeaksInPretrace[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in S ROI" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.sPeaksInROI[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in S IW" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.sPeaksInIW[idx] << " ";
		}
		infoOut << std::endl;

		// Local baseline estimation analysis
		infoOut << "Peak charge histogram" << std::endl;
		for (int idx = 0; idx < 300; idx++)
		{
			infoOut << cInfoData.lbl_peakCharges[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peak amplitude histogram" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.lbl_peakAmplitudes[idx] << " ";
		}
		infoOut << std::endl;

		// Conditional mean filtered analysis
		infoOut << "Peak charge histogram" << std::endl;
		for (int idx = 0; idx < 300; idx++)
		{
			infoOut << cInfoData.cmf_peakCharges[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peak amplitude histogram" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_peakAmplitudes[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peak width histogram" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_peakWidths[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in B PT" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_bPeaksInPretrace[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in B ROI" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_bPeaksInROI[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in B IW" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_bPeaksInIW[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in S PT" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_sPeaksInPretrace[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in S ROI" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_sPeaksInROI[idx] << " ";
		}
		infoOut << std::endl;

		infoOut << "Peaks in S IW" << std::endl;
		for (int idx = 0; idx < 51; idx++)
		{
			infoOut << cInfoData.cmf_sPeaksInIW[idx] << " ";
		}
		infoOut << std::endl;

		// Muon event onsets
		infoOut << "All muon onsets - If linear gate present only the first is recorded" << std::endl;
		for (std::vector<int>::size_type idx = 0; idx < cInfoData.muonVetoEvents.size(); idx++)
		{
			infoOut << cInfoData.muonVetoEvents[idx] << " ";
		}
		infoOut << std::endl;
	}
	return 0;
}
