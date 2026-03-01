# -*- coding: utf-8 -*-
"""

Reading on AI channels without anything connected gets real strange. Seems to go away
if you short diff channel 0 to ground?

Not always though. need to trouble shoot tomorrow




Install notes
 - python -m pip install nidaqmx
 - python -m nidaqmx installdriver
 - pip install -U pyvisa-py
 - pip install -U zeroconf

Main support website for the driver/interface with detailed examples is https://github.com/ni/nidaqmx-python

To troubleshoot and look at the different channel names/configuration options, use NI MAX or the configuration explorer

pyqtgraph stuff
    https://www.pythonguis.com/tutorials/pyside6-embed-pyqtgraph-custom-widgets/

Install prereqs:
    conda install conda-forge::pyqtgraph

For QT interface:
    from conda terminal, run "designer"

@author: carla
"""
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWidgets import QFileDialog, QErrorMessage
from PyQt5.uic import loadUi
import sys
import os
#import socket
#from numpy import array
from scipy.ndimage import uniform_filter1d

from ni_daq import DAQ
import numpy as np

import logging
logging.basicConfig(format='%(asctime)s %(levelname)s - %(message)s', datefmt='%H:%M:%S', level=logging.INFO)
from logging import info, error

from datetime import datetime




default_filepath = r'C:\Users\carla\Documents'
default_filename_base = r'data_20260203'
default_filename_tail = '_run{run:05d}.csv'
default_runnumber = 1
default_runduration = 4
default_samplerate = 1000
default_aichannels = ['ai4' ,'ai5']

default_displayupdaterate = 20      # Hz

default_smoothinginterval = 0.5  # sec

# instrument calibrations
def volts_to_engineering_units_1(data):
    # pressure transducer
    return np.asarray(data)*10.000
def volts_to_engineering_units_2(data):
    # tc reader
    return np.asarray(data)*10.00

def calc_fps(dp_torr, T_C, ptot_psi):
    # scalar inputs only
    dp_Pa   = dp_torr  * 133.322368
    ptot_Pa = ptot_psi * 6894.75729317
    T_K     = T_C      + 273.15
    
    if dp_torr<0:
        return 0.
    rho_kgm3 = ptot_Pa/(287.0500676*T_K)
    vel_ms = np.sqrt(2*dp_Pa/rho_kgm3)
    vel_fps = vel_ms*3.2808399
    return vel_fps







class GUI(QtWidgets.QDialog):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # Load the UI from local path
        ui_path = os.path.dirname(os.path.abspath(__file__))
        loadUi(os.path.join(ui_path, "tunnel_acq_gui.ui"), self)
        
        self.setWindowTitle('Data Acquisition GUI')
        
        # file path and naming
        self.te_filepath.setPlainText(default_filepath)
        self.pb_filepath.clicked.connect(self.selectFilePath)
        
        self.te_filename.setPlainText(f'{default_filename_base}{default_filename_tail}')
        self.pb_filename.clicked.connect(self.selectFileName)
        
        # run number, duration, acq speed
        self.sb_runnumber.setValue(default_runnumber)
        self.sb_runduration.setValue(default_runduration)
        self.sb_samplerate.setValue(default_samplerate)
        
        # actions
        self.pb_takedata.clicked.connect(self.takeData)
        self.sb_runduration.valueChanged.connect(self.update_daq)
        self.sb_samplerate.valueChanged.connect(self.update_daq)
        
        # setup DAQ
        self.daq = DAQ()
        self.update_daq()
        
        # top plot
        self.pw_plot1.setBackground(None)
        self.pw_plot1.setLabel("left", "Delta Pressure (torr)", color="k", size="12pt")
        self.pw_plot1.setLabel("bottom", "Time (sec)",     color="k", size="12pt")
        self.pw_plot1.showGrid(x=True, y=True)
        self.pw_line1 = self.pw_plot1.plot(self.time, self.data1_eng, pen='b')
        
        # bottom plot
        self.pw_plot2.setBackground(None)
        self.pw_plot2.setLabel("left", "Temperature (°C)", color="k", size="12pt")
        self.pw_plot2.setLabel("bottom", "Time (sec)",     color="k", size="12pt")
        self.pw_plot2.showGrid(x=True, y=True)
        self.pw_line2 = self.pw_plot2.plot(self.time, self.data2_eng, pen='b')
        
        # timer to poll DAQ for new measurements
        self.timer = QtCore.QTimer()
        self.timer.setInterval(50)                                                                                  # msec change this
        self.timer.timeout.connect(self.update_gui)
        self.timer.start()

        
    def __del__(self):
        del(self.timer)
        del(self.daq)
        pass
    
    def update_daq(self):
        self.daq.stop_ai()
        
        # update the display vectors to match datacollection settings
        self.time = np.arange(self.sb_runduration.value()*self.sb_samplerate.value())/self.sb_samplerate.value()
        self.data1_raw = np.zeros_like(self.time,dtype=float)
        self.data2_raw = np.zeros_like(self.time,dtype=float)
        self.data1_eng = np.zeros_like(self.time,dtype=float)
        self.data2_eng = np.zeros_like(self.time,dtype=float)
        
        # store this one since it gets called on every plot update
        self.__displayupdatesamples = int(self.sb_runduration.value()*self.sb_samplerate.value()/default_displayupdaterate)
        self.__smoothingfiltersize = int(default_smoothinginterval*self.sb_samplerate.value())
        
        self.daq.start_ai(channels=default_aichannels,
                          samplerate=self.sb_samplerate.value(),
                          min_val=-5.0, max_val=5.0)
    
    def update_gui(self):
        data1_raw, data2_raw = self.daq.read_ai(self.__displayupdatesamples)
        
        # shift data
        self.data1_raw[0:-self.__displayupdatesamples] = self.data1_raw[self.__displayupdatesamples::]
        self.data2_raw[0:-self.__displayupdatesamples] = self.data2_raw[self.__displayupdatesamples::]
        
        # put in the end of the plotting vector
        self.data1_raw[-self.__displayupdatesamples::] = np.asarray(data1_raw,dtype=float)
        self.data2_raw[-self.__displayupdatesamples::] = np.asarray(data2_raw,dtype=float)
        
        # process entire vector
        self.data1_eng[:] = uniform_filter1d(volts_to_engineering_units_1(self.data1_raw), size=self.__smoothingfiltersize)
        self.data2_eng[:] = uniform_filter1d(volts_to_engineering_units_2(self.data2_raw), size=self.__smoothingfiltersize)
        
        
        '''# shift data
        self.data1_eng[0:-self.__displayupdatesamples] = self.data1_eng[self.__displayupdatesamples::]
        self.data2_eng[0:-self.__displayupdatesamples] = self.data2_eng[self.__displayupdatesamples::]
        
        # convert to engineering units and put in the end of the plotting vector
        self.data1_eng[-self.__displayupdatesamples::] = volts_to_engineering_units_1(data1_raw)
        self.data2_eng[-self.__displayupdatesamples::] = volts_to_engineering_units_2(data2_raw)
        '''
        self.pw_line1.setData(self.time, self.data1_eng)
        self.pw_line2.setData(self.time, self.data2_eng)
        
        # update conditions
        dp_torr = self.data1_eng.mean()
        T_C = self.data2_eng.mean()
        T_F = T_C*9/5 + 32.2
        
        self.lcd_windspeed.display(f'{calc_fps(dp_torr,T_C,self.sb_ptot.value()):.1f}')
        self.lcd_temp.display(f'{T_F:.1f}')
        
        if(self.pb_printchannelaverages.isChecked()):
            info(f'1: {self.data1_eng.mean():6.4f} torr          2: {self.data2_eng.mean():6.4f} deg C')
    
    def selectFilePath(self):
        filepath = QFileDialog.getExistingDirectory(self,
                                                    'Open Directory',
                                                    self.te_filepath.toPlainText(),
                                                    QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        self.te_filepath.setPlainText(filepath)
    
    def selectFileName(self):
        filepath = QFileDialog.getSaveFileName(self,
                                               "Select Filepattern to Save",
                                               self.te_filepath.toPlainText(),
                                               "Comma Separated Values (*.csv)")
        filename = os.path.split(filepath[0])[1]    # take out the path
        filename = os.path.splitext(filename)[0]    # drop the extension
        self.te_filename.setPlainText(f'{filename}{default_filename_tail}')

    def takeData(self):
        run = self.sb_runnumber.value()
        duration = self.sb_runduration.value()
        
        filename = os.path.join(self.te_filepath.toPlainText(),
                                self.te_filename.toPlainText().format(run=run))
        
        # check if file exists, never overwrite
        if os.path.exists(filename):
            error(f'File {filename} exists. Data acquisition aborted.')
            error_dialog = QErrorMessage(self)
            error_dialog.showMessage(f'File {filename} exists. Data acquisition aborted.')
        else:
            np.savetxt(filename,
                       np.vstack((self.time,self.data1_eng,self.data2_eng)).T,
                       fmt='%10.4f',
                       delimiter=',',
                       header=(f'{datetime.now().strftime("%Y%m%d %H:%M:%S")}\n'
                               f'Run {self.sb_runnumber.value()}\n'
                               f'Acquisition Duration: {self.sb_runduration.value()} sec\n'
                               f'Sample Rate: {self.sb_samplerate.value()} Hz\n'
                               '\n'
                               f'Ambient Pressure: {self.sb_ptot.value()} psi\n'
                               f'Ambient Temperature: {self.sb_ttot.value()} C\n'
                               f'VFD Frequency: {self.sb_vfdfreq.value()} Hz\n'
                               '\n'
                               'Time [s], Pitot Tube Delta Pressure [torr], Tunnel Air Temperature [C]'),
                       comments='')
            
            # when sucessfull 
            self.sb_runnumber.setValue(run+1)
            info(f'Acquired {duration} sec of data to {filename}')




if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    main = GUI()
    main.show()
    
    app.exec_()
    
    del(main)  # after exit, stop all the DAQ tasks
    
    
    
    
    