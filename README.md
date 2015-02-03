# ffdb
Flat File Configuration Database for Embedded Projects

This is a simple key-value database that uses a flat file.  It was created for and used in small embedded projects for a configuration database.  It supports file locking for writes and
atomic updates.

# Usage
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

# Usage Tips
If your going to write a string, use the -s option to make sure it is written as a string


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






