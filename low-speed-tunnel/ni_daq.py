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
from random import random

import logging
logging.basicConfig(format='%(asctime)s %(levelname)s - %(message)s', datefmt='%H:%M:%S', level=logging.INFO)
from logging import info, error


class DAQ():
    def __init__(self,daq_type='USB-6221'):
        self.device = None
        
        self.channels_ai = None
        self.task_ai = None
        
        self.channel_pwm = None
        self.task_pwm = None
        self.counter_pwm = None
        
        # not done yet
        self.channels_di = None
        self.channels_do = None
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
    def start_ai(self,channels=['ai0' ,'ai1'],samplerate=1000,min_val=-5.0,max_val=5.0):
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
    def start_pwm(self,speed=50,channel='PFI8',pwm_min=1.5/20.,pwm_max=1.2/20.,):
        #
        # speed - int or float, percent max speed (range 0 to 100)
        #
        if self.device is not None:
            # turn off pwm if currently running
            if self.task_pwm is not None:
                try:
                    self.task_pwm.stop()
                except:
                    pass
                finally:
                    self.task_pwm = None
            
            # initialize and start pwm signal
            self.channel_pwm = channel
            self.task_pwm = Task()
            self.counter_pwm = self.task_pwm.co_channels.add_co_pulse_chan_freq(f'{self.device}/ctr0',
                                                                                idle_state=constants.Level.LOW,
                                                                                initial_delay=0.0,
                                                                                freq=50.0,
                                                                                duty_cycle=pwm_min + (pwm_max-pwm_min)*speed/100.)
            self.counter_pwm.co_pulse_term = f'/{self.device}/{channel}'
            self.task_pwm.timing.cfg_implicit_timing(sample_mode=constants.AcquisitionType.CONTINUOUS)
            self.task_pwm.start()

    def stop_pwm(self):
        if self.task_pwm is not None:
            try:
                self.task_pwm.stop()
            except:
                pass
            finally:
                self.task_pwm = None
                
    
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
    
    ###########################################################################
    ###########################################################################
    ###
    ###  Counter Input Methods
    ###
    ###########################################################################
    ###########################################################################
    

    