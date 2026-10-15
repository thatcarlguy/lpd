# -*- coding: utf-8 -*-
"""
Created on Tue Feb  3 09:39:37 2026

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
from datetime import datetime

from ni_daq import DAQ


import numpy as np

import logging
logging.basicConfig(format='%(asctime)s %(levelname)s - %(message)s', datefmt='%H:%M:%S', level=logging.INFO)
from logging import info, error

default_filepath = r'C:\Users\carla\Documents'
default_filename_base = r'data_20260427'
default_filename_tail = '_run{run:05d}.csv'
default_runnumber = 1
default_runduration = 1
default_feedduration = 10
default_samplerate = 5          # kHz for PD only

default_ai_channel = 'ai0'
default_do_channels = ['port0/line0','port0/line1','port0/line2','port0/line3']
default_displayupdaterate = 10      # Hz



# instrument calibrations
def volts_to_engineering_units_1(data):
    # photodiode
    return np.asarray(data)*1000.00

class GUI(QtWidgets.QDialog):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # Load the UI from local path
        ui_path = os.path.dirname(os.path.abspath(__file__))
        loadUi(os.path.join(ui_path, "lpd_acq_gui.ui"), self)
        
        self.setWindowTitle('Data Acquisition GUI')
        
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
        self.sb_feedduration.setValue(default_feedduration)
        
        # actions
        self.pb_takedata.clicked.connect(self.takeData)
        self.sb_runduration.valueChanged.connect(self.update_daq)
        self.sb_samplerate.valueChanged.connect(self.update_daq)
        self.pb_particle_low.clicked.connect( lambda : self.setTimedParticleFeed(1))
        self.pb_particle_med.clicked.connect( lambda : self.setTimedParticleFeed(2))
        self.pb_particle_high.clicked.connect(lambda : self.setTimedParticleFeed(3))
        self.pb_particle_off.clicked.connect( self.stopTimedFeeder)
        self.feed_speed = 'Off'
        
        # setup DAQ
        self.daq = DAQ()
        self.update_daq()
        
        # top plot
        self.pw_plot1.setBackground(None)
        self.pw_plot1.setLabel("left", "Photodiode Signal (mV)", color="k", size="12pt")
        self.pw_plot1.setLabel("bottom", "Time (sec)",     color="k", size="12pt")
        self.pw_plot1.showGrid(x=True, y=True)
        self.pw_line1 = self.pw_plot1.plot(self.time, self.data1_eng, pen='b')
        
        # timer to poll DAQ for new measurements
        self.timer = QtCore.QTimer()
        self.timer.setInterval(50)                                                                                  # msec change this
        self.timer.timeout.connect(self.update_gui)
        self.timer.start()
        
        # timer to run feeder for specified run time
        self.ftimer = QtCore.QTimer()
        self.ftimer.setInterval(default_feedduration*1000)      # msec
        self.ftimer.timeout.connect(self.stopTimedFeeder)
        self.ftimer.setSingleShot(True)
        

        
    def __del__(self):
        del(self.timer)
        del(self.ftimer)
        del(self.daq)
        pass
    
    def update_daq(self):
        self.daq.stop_ai()
        
        # update the display vectors to match datacollection settings
        self.time = np.arange(self.sb_runduration.value()*self.sb_samplerate.value()*1000)/(self.sb_samplerate.value()*1000)
        self.data1_eng = np.zeros_like(self.time,dtype=float)
        
        # store this one since it gets called on every plot update
        self.__hsdisplayupdatesamples = int(self.sb_runduration.value()*(self.sb_samplerate.value()*1000)/default_displayupdaterate)
        
        self.daq.start_ai(channels=[default_ai_channel],
                          samplerate=self.sb_samplerate.value()*1000,
                          min_val=-5.0, max_val=5.0)
        
    def update_gui(self):
        data1_raw = self.daq.read_ai(self.__hsdisplayupdatesamples)
        
        # shift data
        self.data1_eng[0:-self.__hsdisplayupdatesamples] = self.data1_eng[self.__hsdisplayupdatesamples::]
        
        # convert to engineering units and put in the end of the plotting vector
        self.data1_eng[-self.__hsdisplayupdatesamples::] = volts_to_engineering_units_1(data1_raw)
        
        self.pw_line1.setData(self.time, self.data1_eng)
        
        
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
                       np.vstack((self.time,self.data1_eng)).T,
                       fmt='%10.4f',
                       delimiter=',',
                       header=(f'{datetime.now().strftime("%Y%m%d %H:%M:%S")}\n'
                               f'Run {self.sb_runnumber.value()}\n'
                               f'Acquisition Duration: {self.sb_runduration.value()} sec\n'
                               f'Sample Rate: {self.sb_samplerate.value()} kHz\n'
                               '\n'
                               f'Particle Feeder Speed: {self.feed_speed}\n'
                               '\n'
                               'Time [s], Photodiode [mV]'),
                       comments='')
            
            # when sucessfull 
            self.sb_runnumber.setValue(run+1)
            info(f'Acquired {duration} sec of data to {filename}')
    
    
    
    
    
    
    
    
    def stopTimedFeeder(self):
        self.daq.write_do_array(channels=default_do_channels,values=[False,False,False,False])
        self.feed_speed = "Off"
        info(f'Setting particle distribution speed to {self.feed_speed}')
        
        if self.ftimer.isActive():
            self.ftimer.stop()
        
        if self.pb_particle_low.isChecked():
            self.pb_particle_low.setChecked(False)
        if self.pb_particle_med.isChecked():
            self.pb_particle_med.setChecked(False)
        if self.pb_particle_high.isChecked():
            self.pb_particle_high.setChecked(False)
        

    def setTimedParticleFeed(self,speed):
        # 1 is slow, 2 is medium, 3 is high
        if speed==1:
            if self.pb_particle_med.isChecked():
                self.pb_particle_med.setChecked(False)
            if self.pb_particle_high.isChecked():
                self.pb_particle_high.setChecked(False)
        elif speed==2:
            if self.pb_particle_low.isChecked():
                self.pb_particle_low.setChecked(False)
            if self.pb_particle_high.isChecked():
                self.pb_particle_high.setChecked(False)
        else: # speed==3
            if self.pb_particle_low.isChecked():
                self.pb_particle_low.setChecked(False)
            if self.pb_particle_med.isChecked():
                self.pb_particle_med.setChecked(False)
        
        if self.pb_particle_low.isChecked():
            self.feed_speed = "Low"
            self.daq.write_do_array(channels=default_do_channels,values=[True,True,False,False])
        elif self.pb_particle_med.isChecked():
            self.feed_speed = "Medium"
            self.daq.write_do_array(channels=default_do_channels,values=[True,False,True,False])
        else: # self.pb_particle_high.isChecked():
            self.feed_speed = "High"
            self.daq.write_do_array(channels=default_do_channels,values=[True,False,False,True])
        
        self.ftimer.setInterval(self.sb_feedduration.value()*1000)      # msec
        info(f'Setting particle distribution speed to {self.feed_speed} for {self.sb_runduration.value()} seconds')
        self.ftimer.start()

















if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    main = GUI()
    main.show()
    sys.exit(app.exec_())
    