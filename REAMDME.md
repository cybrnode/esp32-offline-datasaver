# Offline data save
This code is written to save offline data in esp8266 incase nework is unavailable and and resync the data when network is available again. It uses SPIFFS to save data and can cater larger amount of data.

## Algorithm
```
    Read data from sensor
    save in new file
    If netwrk is available
        read a already saed file
        send data on network
            if network POST was successful
                delete the file
```

## Current issues
1. Writes on Flas are limited this things still requires testing a conclusion.
2. char * mishandeling issues occur when an attempt is made to read the already saved file.