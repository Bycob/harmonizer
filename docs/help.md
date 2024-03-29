# Helpful stuff

## Install Jack

```bash
sudo apt install jackd2 libjack-jackd2-dev
```

Add realtime permissions to jack:
- go to file `/etc/security/limits.d/audio.conf`
- check that it contains the lines:
```
# Provided by the jackd package.
#
# Changes to this file will be preserved.
#
# If you want to enable/disable realtime permissions, run
#
#    dpkg-reconfigure -p high jackd

@audio   -  rtprio     95
@audio   -  memlock    unlimited
#@audio   -  nice      -19
```

Check that you belong to the "audio" group (Use `groups`)

If you don't:
```bash
sudo usermod -a -G audio [username]
```
and then logout/login.

## Start Jack

Check which devices you want to use as input/output
```bash
aplay -l
# or
cat /proc/asound/cards
```

Then use script to start jack with the required devices
```bash
# if you want to use device 2
scripts/start_jack.sh 2
jack_simple_client
```

Normally you should hear a sound

## Get Midi info & debug

```bash
aconnect -io
```

## Play midi from command line

```bash
sudo apt install wildmidi
wildmidi [filename.mid]
```

## Install FFTW 3

```bash
wget https://www.fftw.org/fftw-3.3.10.tar.gz
tar xvf fftw-3.3.10.tar.gz
cd fftw-3-3.10
./configure --enable-float
make
make install
```

if needed to clear the build folder

```bash
make distclean
```

