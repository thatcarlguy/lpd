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
from PyQt5.uic import loadUi
import sys
import os

from ni_daq import DAQ


import logging
logging.basicConfig(format='%(asctime)s %(levelname)s - %(message)s', datefmt='%H:%M:%S', level=logging.INFO)
from logging import info

default_feedduration = 10

default_do_channels = ['port0/line0','port0/line1','port0/line2','port0/line3']



class GUI(QtWidgets.QDialog):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # Load the UI from local path
        ui_path = os.path.dirname(os.path.abspath(__file__))
        loadUi(os.path.join(ui_path, "feeder_gui.ui"), self)
        
        self.setWindowTitle('Particulate Feeder GUI')
        
        # run number, duration, acq speed
        self.sb_feedduration.setValue(default_feedduration)
        
        # actions
        self.pb_particle_low.clicked.connect( lambda : self.setTimedParticleFeed(1))
        self.pb_particle_med.clicked.connect( lambda : self.setTimedParticleFeed(2))
        self.pb_particle_high.clicked.connect(lambda : self.setTimedParticleFeed(3))
        self.pb_particle_off.clicked.connect( self.stopTimedFeeder)
        self.feed_speed = 'Off'
        
        # setup DAQ
        self.daq = DAQ()
        
        # timer to run feeder for specified run time
        self.ftimer = QtCore.QTimer()
        self.ftimer.setInterval(default_feedduration*1000)      # msec
        self.ftimer.timeout.connect(self.stopTimedFeeder)
        self.ftimer.setSingleShot(True)
        

    def __del__(self):
        del(self.ftimer)
        del(self.daq)
        pass

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
    