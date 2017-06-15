import os
import sys
import json
import hashlib
import subprocess
from gtts import gTTS
import speech_recognition as sr

CACHE_DIR = "./tts"

r = sr.Recognizer()

def init():
    if os.path.isdir(CACHE_DIR) == False:
        os.mkdir(CACHE_DIR)
    with sr.Microphone() as source:
        print("Calibrating ambient noise level")
        # listen for 1 second to calibrate the energy threshold for ambient noise levels
        r.adjust_for_ambient_noise(source)
        print("Done")

def play(path):
    subprocess.Popen(['mpg123', '-q', path]).wait()

def generateFilename(text):
    m = hashlib.md5()
    m.update(text)
    return CACHE_DIR + "/" + str(m.hexdigest()) + ".mp3"

def checkCacheFor(text):
    return os.path.isfile(generateFilename(text))

def downloadTTS(text):
    tts = gTTS(text=text, lang='it')
    tts.save(generateFilename(text))

def talk(text, useCache=True, saveCache=True):
    if useCache == False or checkCacheFor(text) == False:
        downloadTTS(text)
    play(generateFilename(text))

def listen():
    # obtain audio from the microphone
    with sr.Microphone() as source:
        print("Now listening")
        audio = r.listen(source)

    # recognize speech using Google Speech Recognition
    try:
        # for testing purposes, we're just using the default API key
        # to use another API key, use `r.recognize_google(audio, key="GOOGLE_SPEECH_RECOGNITION_API_KEY")`
        # instead of `r.recognize_google(audio)`
        text = r.recognize_google(audio, language='it-IT')
        return text
    except sr.UnknownValueError:
        print("Google Speech Recognition could not understand audio")
        return None
    except sr.RequestError as e:
        print("Could not request results from Google Speech Recognition service; {0}".format(e))
        return None
