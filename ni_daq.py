# -*- coding: utf-8 -*-
"""

Reading on AI channels without anything connected gets real strange. Seems to go away
if you short diff channel 0 to ground?

Install notes
 - python -m pip install nidaqmx
 - python -m nidaqmx installdriver
 - pip install -U pyvisa-py
 - pip install -U zeroconf

Main support website for the driver/interface with detailed examples is https://github.com/ni/nidaqmx-python

To troubleshoot and look at the different channel names/configuration options, use NI MAX or the configuration explorer


"""
from nidaqmx.system import System
from nidaqmx import Task, constants
from nidaqmx.stream_writers import CounterWriter
from random import random
#import numpy as np

import logging
logging.basicConfig(format='%(asctime)s %(levelname)s - %(message)s', datefmt='%H:%M:%S', level=logging.INFO)
from logging import info, error


class DAQ():
    def __init__(self,daq_type='USB-6361'):
        self.device = None
        
        # AI 
        self.channels_ai = None
        self.task_ai = None
        
        # DO
        self.channel_do = None
        self.task_do = None
        
        # not done yet
        self.channels_di = None
        self.channel_counter = None
        self.channels_ao = None
        
        # look for daq
        system = System.local()
        for _device in system.devices:
            if daq_type in _device.product_type:
                info(f'Found {_device.product_type} as "{_device.name}"')
                self.device = _device.name
                return
        error('USB DAQ not found, entering debug mode')
        
    def __del__(self):
        try:    self.task_ai.stop()
        except: pass
        try:    self.task_pwm.stop()
        except: pass
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Analog Input Methods
    ###
    ###########################################################################
    ###########################################################################
    def start_ai(self,task=0,channels=['ai0' ,'ai1'],samplerate=1000,min_val=-5.0,max_val=5.0):
        # Start data acquisition on the supplied analog channels, with the same samplerate and voltage range for all channels
        #  samplerate - Hz
        # min_val/max_val - Volts
        
        self.channels_ai = channels
        
        if self.device is not None:
            # turn off if currently running
            if self.task_ai is not None:
                try:
                    self.task_ai.stop()
                except:
                    pass
                finally:
                    self.task_ai = None
            
            self.task_ai = Task()
            for channel in channels:
                self.task_ai.ai_channels.add_ai_voltage_chan(f'{self.device}/{channel}',
                                                                terminal_config=constants.TerminalConfiguration.DIFF,
                                                                min_val=min_val, max_val=max_val)
            
            self.task_ai.timing.cfg_samp_clk_timing(float(samplerate), 
                                                       sample_mode=constants.AcquisitionType.CONTINUOUS)
            self.task_ai.start()
    
    def read_ai(self,number_of_samples_per_channel=100):
        # return is a list of lists, [N channels][N samples]
        if self.task_ai is None:
            return len(self.channels_ai)*[[random()+0.5 for i in range(number_of_samples_per_channel)]]
        return self.task_ai.read(number_of_samples_per_channel=number_of_samples_per_channel)
    
    def stop_ai(self):
        if self.task_ai is not None:
            try:
                self.task_ai.stop()
            except:
                pass
            finally:
                self.task_ai = None
    
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Digital Output PWM Methods
    ###
    ###########################################################################
    ###########################################################################
    
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Analog Output Methods
    ###
    ###########################################################################
    ###########################################################################
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Digital Input Methods
    ###
    ###########################################################################
    ###########################################################################
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Digital Output Methods
    ###
    ###########################################################################
    ###########################################################################
    def write_do_single(self,channel='port0/line0',value=True):
        if self.device is not None:
            
            if self.task_do is not None:                    # not really implemented or checked
                self.task_do.stop()
            
            self.channels_do = f'{self.device}/{channel}'
            self.task_do = Task()
            self.task_do.do_channels.add_do_chan(self.channels_do,
                                                     line_grouping=constants.LineGrouping.CHAN_PER_LINE)
            self.task_do.start()
            self.task_do.write(value)
            self.task_do.stop()
    
    def write_do_array(self,channels=['port0/line0','port0/line1','port0/line2','port0/line3'],values=[False,False,False,False]):
        if self.device is not None:
            
            if self.task_do is not None:                    # not really implemented or checked
                self.task_do.stop()
            
            self.channels_do = [f'{self.device}/{channel}' for channel in channels]
            self.task_do = Task()
            for channel in self.channels_do:
                self.task_do.do_channels.add_do_chan(channel,
                                                     line_grouping=constants.LineGrouping.CHAN_PER_LINE)
            self.task_do.start()
            self.task_do.write(values)
            self.task_do.stop()
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Counter Input Methods
    ###
    ###########################################################################
    ###########################################################################
    

if __name__=='__main__':
    # print all devices found under system
    
    system = System.local()
    device = None
    for ni_device in system.devices:
        print(f'Found {ni_device.product_type} as "{ni_device.name}"')

