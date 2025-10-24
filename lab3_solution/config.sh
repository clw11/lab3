#!/bin/bash

cd pox_module
sudo python setup.py develop

pkill -9 solution
pkill -9 sr

