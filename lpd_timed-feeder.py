# -*- coding: utf-8 -*-
"""
Created on Tue Feb  3 09:39:37 2026

Install notes
 - python -m pip install nidaqmx
 - python -m nidaqmx installdriver
 - pip install -U pyvisa-py
 - pip install -U zeroconf

@author: Carl Hall
"""
from time import sleep

from ni_daq import DAQ
daq = DAQ()

import logging
logging.basicConfig(format='%(asctime)s %(levelname)s - %(message)s', datefmt='%H:%M:%S', level=logging.INFO)
from logging import info

default_do_channels = ['port0/line0','port0/line1','port0/line2','port0/line3']



###############################################################################
#speed = 'Slow'      # Slow      150
#speed = 'Medium'    # Medium     165
speed = 'Fast'      # Fast      180

runtime = 10  # sec





###############################################################################
# run the cycle
info(f'Running particle distribution for {runtime} sec at {speed} speed')
if speed=='Slow':
    daq.write_do_array(channels=default_do_channels,values=[True,True,False,False])
    sleep(runtime)
    daq.write_do_array(channels=default_do_channels,values=[False,False,False,False])
    info('done!')

elif speed=='Medium':
    daq.write_do_array(channels=default_do_channels,values=[True,False,True,False])
    sleep(runtime)
    daq.write_do_array(channels=default_do_channels,values=[False,False,False,False])
    info('done!')

elif speed=='Fast':
    daq.write_do_array(channels=default_do_channels,values=[True,False,False,True,])
    sleep(runtime)
    daq.write_do_array(channels=default_do_channels,values=[False,False,False,False])
    info('done!')

else:
    raise IOError('unsupported speed')




#daq.write_do_array(channels=default_do_channels,values=[True,True,True,True])




