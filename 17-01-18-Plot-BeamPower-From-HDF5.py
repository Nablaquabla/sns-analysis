import numpy as np
import h5py
import datetime
import pytz
import calendar
import copy
import matplotlib as mpl
import easyfy as ez
import colormaps as cmaps
import matplotlib.pylab as plt
plt.ioff()
#=============================================================================#
#                           Setup  Plot Settings
#=============================================================================#
mpl.rc('font', **{'serif' : 'Times New Roman','family' : 'serif'})
mpl.rcParams['legend.fancybox'] = True
mpl.rcParams['legend.shadow'] = False
mpl.rcParams['figure.dpi'] = 100
mpl.rcParams['ytick.major.pad'] = 5
mpl.rcParams['xtick.major.pad'] = 5
mpl.rcParams['axes.formatter.limits'] = (-4,4)
plt.rc('font', **{'size':12})
colors = ez.colors()
colMap = cmaps.viridis
colMap.set_under('w')
plotDir = 'C:/Users/Nablaquabla/Desktop/COHERENT-Plots/'

# Set print formats for timestamps
fmt = '%Y-%m-%d %H:%M:%S %Z%z'
fmt_Plot = '%Y-%m-%d'

# Define timezones used in analysis
eastern = pytz.timezone('US/Eastern')
utc = pytz.utc
epochBeginning = utc.localize(datetime.datetime(1970,1,1))

# Set data paths
beamPowerFile = 'F:/Work-Data-Storage/BeamPower/BeamPowerHistory.h5'
#runHistoryFile = './start-stop-times-of-runs.txt'
runHistoryFile = './start-stop-times-of-runs-excl-outage.txt'

# Create timezone aware timestamps for start and stop dates
start,stop = np.loadtxt(runHistoryFile,unpack=True,dtype=str)
startTS = [eastern.localize(datetime.datetime.strptime(x[4:],'%y-%m-%d-%H-%M-%S')) for x in start]
stopTS = [eastern.localize(datetime.datetime.strptime(x,'%y-%m-%d-%H-%M-%S')) for x in stop]

# Convert timestamps to seconds in epoch for UTC
start_sse = [((x.astimezone(utc) - epochBeginning).total_seconds()) for x in startTS]
stop_sse = [((x.astimezone(utc) - epochBeginning).total_seconds()) for x in stopTS]

# Open beam power data file
h5In = h5py.File(beamPowerFile,'r')
days = np.sort(h5In.keys())

# Read times and power from data file
time = np.array([])
power = np.array([])
for d in days:
    print d
    time = np.append(time,h5In['%s/time'%d][...])
    power = np.append(power,h5In['%s/power'%d][...])

# Prepare figure output
plt.figure(figsize=(6.5,4),edgecolor='k',facecolor='w')
ax = plt.subplot(111)

# Plot shaded areas for times we have recorded data
for start,stop in zip(start_sse,stop_sse):
    plt.fill_between([start,stop],0,1.8,color=colors.green,alpha=0.2)

# Plot beam power data
plt.plot(time[::10],power[::10],c=colors.blue)

# Get total beam power delivered and total beam power available
totalPowerMeasured = 0
totalPowerDelivered = 0
totalPowerDelivered = np.sum(power[(time >= start_sse[0])*(time <= stop_sse[-1])])/3600.0
for start,stop in zip(start_sse,stop_sse):
    c = (time >= start) * (time <= stop)
    totalPowerMeasured += np.sum(power[c])/3600.0

print totalPowerMeasured
print totalPowerDelivered

# Set plot limits to something reasonable
plt.xlim(start_sse[0],stop_sse[-1])

# Update ticks to show in human readible dates
t0 = eastern.localize(datetime.datetime(2015,3,1))
tickLabels = []
for j in np.arange(7):
    for j in range(3):
        _tmpTS = t0 + datetime.timedelta(days=calendar.monthrange(t0.year,t0.month)[1])
        t0 = _tmpTS
    tickLabels.append(copy.copy(_tmpTS))
tickNumbers = [((x.astimezone(utc) - epochBeginning).total_seconds()) for x in tickLabels]
tickLabels = [x.strftime(fmt_Plot) for x in tickLabels]
plt.xticks(tickNumbers,tickLabels)

# Set proper labels
plt.ylabel("Average Beam Power Per Second [MW]")
plt.xlabel("Time")
plt.ylim(0,1.8)

# Add power delivered and measured to plot
plt.text(0.03,0.95,'Total energy delivered between %s and %s: %.2f MWHr'%(startTS[0].strftime(fmt_Plot),stopTS[-1].strftime(fmt_Plot),totalPowerDelivered),transform=ax.transAxes,ha='left',va='center',color=colors.black)
plt.text(0.03,0.88,'Data available for times with a total energy of %.2f MWHr'%totalPowerMeasured,transform=ax.transAxes,ha='left',va='center',color=colors.black)

# Make things pretty
plt.tight_layout(pad=0.25)
plt.savefig(plotDir + 'COHERENT-Beampower.png',dpi=500)
plt.show()