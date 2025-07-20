import sys
from enum import Enum

class Level(Enum):
    eTrace = "trace"
    eInfo = "info"
    eWarn = "warn"
    eError = "error"
    eFatal = "fatal"

color_reset = '\x1b[0m'
red_color = '\x1b[31m'
green_color = '\x1b[32m' 
cyan_color = '\x1b[36m' 

log_levels = {
    Level.eTrace: green_color,
    Level.eInfo: cyan_color,
    Level.eError: red_color,
}

def log(level: Level, message: str):
    level_string: str = str(level)
    level_string = level_string.removeprefix('Level.e')
    color = log_levels[level]

    print(f'[{color}{level_string}{color_reset}]: {message}')

def trace(message: str): log(Level.eTrace, message)
def info(message: str): log(Level.eInfo, message)
def error(message: str): log(Level.eError, message)

def fail(message: str):
    error(message)
    sys.exit(1)
