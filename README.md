# ffdb
Flat File Configuration Database for Embedded Projects

This is a simple key-value database that uses a flat file.  It was created for and used in small embedded projects for a configuration database.  It supports file locking for writes and
atomic updates.  I primarily use it in embedded linux applications called from shell scripts, but it can be used in a variety of ways.  It does compile and fully functional in windows, but we primarily use that for simulation.

Pull requests are welcome. 

# Usage
```
usage: ./ffdb [-h] [-v] [-d database] [-t temp_file] [-l lock_file] key [value]
    where -h prints this message.
          -x output xml
          -s force string write (not auto detect).
          -v set verbose mode.
          -m multiple keys (read only).
          -a return all keys
          -q remove quotes on output.
          -d sets database file [default /data/cfg/config.db]
          -t sets temporary file [default /data/cfg/ffdb.tmp]
          -l sets lock file [default /tmp/ffdblock.tmp]
```

You will likely want to change the defaults for database and temporary file in the source code before compiling.

# Usage Tips
If a string is going to write a string, use the -s option to make sure it is written as a string


# Examples

Examples using the included sample database config.lua

./ffdb -d config.lua SESSION_ID
"test_HD"

./ffdb -d config.lua -q SESSION_ID
test_HD

./ffdb -d config.lua -t /tmp/ttt.txt SESSION_ID "first session"
./ffdb -d config.lua SESSION_ID
"first session"

./ffdb -d config.lua -x -m ENABLE_HD ENABLE_CIF AUDIO_ENCODER
<database>
<keys>
<AUDIO_ENCODER s="AAC-LC"/>
<ENABLE_HD ul="1"/>
<ENABLE_CIF ul="1"/>
</keys>
</database>

./ffdb -d config.lua -m ENABLE_HD ENABLE_CIF AUDIO_ENCODER
AUDIO_ENCODER="AAC-LC"
ENABLE_HD=1
ENABLE_CIF=1

#Shell Examples

/bin/session_audio -sid $(/bin/ffdb -q -d /data/cfg/config.lua SESSION_ID) -ac -sid $(/bin/ffdb -q -d /data/cfg/config.lua AUDIO_ENCODER)

This would turn into this command with our sample database:

/bin/session_audio -sid test_HD -ac AAC-LC

#Debugging
You can turn on verbose mode to see how the command is processed if your experiencing any weirdness.




#Enhancements
Currently this software cannot add keys into a database.  It has been used primarily where there would be a default database stored in ROM and copied over on "reset to default" 
and then modified from there.   Some may want to add this feature.

There is no caching, so every transaction at least opens, reads and closes the file.   This can be expensive if your reading database values one by one, especially in slow flash filesystems.
There is a read multiple feature that can really help, but it might be hard to manage for some on the script side.

#Others Stuff
I have a mini-redis like implementation that operates over UDP sockets using the same database (at the same time as this software if needed) with caching if there is enough interest in this software.












