#!/usr/bin/env python3

import os
import sys
import subprocess
import tempfile
import shutil
from enum import Enum

color_reset = '\x1b[0m'
red_color = '\x1b[31m'
green_color = '\x1b[32m' 
cyan_color = '\x1b[36m' 

log_levels = {
    "trace": green_color,
    "info": cyan_color,
    "error": red_color,
}

def log(level: str, message: str):
    color = log_levels[level]
    print(f'[{color}{level}{color_reset}]: {message}')

def trace(message: str): log('trace', message)
def info(message: str): log('info', message)
def error(message: str): log('error', message)

def run():
    pass

if __name__ == '__main__':
    run()
