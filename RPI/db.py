import os
import json

def loadDB(name):
    if existsDB(name) == False:
        return None
    try:
        with open(name + '.json', 'r') as f:
            return json.load(f)
    except:
        return None

def existsDB(name):
    return os.path.isfile(name + '.json')

def saveDB(name, data):
    with open(name + '.json', 'w') as f:
        json.dump(data, f)
