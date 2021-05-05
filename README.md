# ALCC: Migrating Congestion Control to the Application Layer

Application Layer Congestion Control (ALCC) framework, that allows any new CC protocol to run at the application layer on top of the underlying TCP stack, and drives it to deliver approximately the same performance, as it would achieve natively.

This implementation includes Copa and Verus integration into ALCC which can be tested and compared over a set of different channel traces. Following are the necessary instructions:

## Instructions
The directory called 'Applications' includes an easy-to-configure FTP server called bftpd. We have used bftpd for ALCC to download a large file for our experiments. Additionally, if you want to compare against native Verus or native Copa you will need to clone them into the 'Applications' folder.

#### Installations
Install mahimahi (See also http://mahimahi.mit.edu/)
````sh
$ git clone https://github.com/ravinet/mahimahi
$ cd mahimahi
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo sysctl -w net.ipv4.ip_forward=1
````

#### Python and Linux packages
````sh
$ sudo apt install python-pip
$ pip install numpy
$ pip install matplotlib
$ sudo apt-get install python-matplotlib
$ pip install pandas
$ sudo apt-get install tshark
$ sudo apt-get install makepp
$ sudo apt-get install g++ makepp libboost-dev libprotobuf-dev protobuf-compiler libjemalloc-dev iperf libboost-python-dev
$ sudo apt-get install build-essential autoconf libasio-dev libalglib-dev libboost-system-dev
````

#### Compile native verus and native copa

For Verus and Copa, we have cloned and modified slightly the native versions of these protocols to be able to compare. You just need to compile them. You can find the native repos inside the applications folder

Compile Copa

````sh
$ cd Applications/genericCC
$ makepp
````

For Verus
````sh
$ cd Applications/verus
$ ./bootstrap.sh
$ ./configure
$ make
````

#### Kernel Version Required
The ALCC kernel module requires kernel version 4.19.0. 
Make sure you upgrade/downgrade to kernel version 4.19.0 before proceeding. 


#### Set default TCP congestion control protocol
Before starting any experiments, make sure the default system TCP congestion control protocol is set as desired. For example; for cubic, do the following

```sh
$ sudo sysctl -w net.ipv4.tcp_congestion_control=cubic
```
#### Install ALCC Kernel Module
```sh
$ cd libalcc/netfilter
$ make clean
$ make
$ sudo insmod alcc_kernel.ko 
```

#### Generating Figure 4,5, and 6 (Verus vs. ALCC verus)

Before running the bftpd server, make sure you make changes to the following files.

go to to the bftpd folder and copy the path given by pwd
````sh
$ cd Applications/bftpd/
$ pwd
````

Next, open the bftpd.conf file, scroll down to FILE_AUTH and provide the full path you obtained earlier. + '/passwd'. You should also change the AUTO_CHDIR, and make it point to yout Desktop folder, usually /home/username/Desktop.

You will also have to add to the passwd file inside the bftpd folder the user by adding to the end of it:
YOUR_USERNAME * sudo /home/YOUR_USERNAME/Desktop

Note that, for the wget command to work, make sure the file you plan to download is placed in the same location defined in AUTO_CHDIR (as we mentioned earlier in your Desktop).

For ALCC Verus follow the following steps:
```sh
$ cd libalcc/verus/lib/alglib/src/
$ gcc -c *.cpp
```

Compile bftpd server for Alcc verus
```sh
$ cd Applications
$ cd bftpd
$ cp Makefile_verus Makefile
$ make clean
$ make
```

open a new termain window.
Make sure you choose the underlying default TCP congestion control protocol first For-example: to choose CUBIC do the following:
````sh
$ sudo sysctl net.ipv4.tcp_congestion_control=cubic
````

You can confirm the chosen TCP CC protocol as follows:
````sh
$ sysctl net.ipv4.tcp_congestion_control
````

Start the bftpd server
```sh
$ cd Applications/bftpd/
$ sudo ./bftpd -D -c bftpd.conf
```

Navigate to the Artifact folder, and edit the run.py file.
Make sure you edit lines 12 - 15, and change them to the correct linux user, the user password, and the machine IP address (Do not set it to localhost or 127.0.0.1). This is an example of these parameters:

SER_NAME= 'cuda'
PASSWORD = 'cuda'
IP_ADDRESS = '10.224.41.106'
FILE_TO_DOWNLOAD = 'jellyLarge.m4v'

Notice how we also have a FILE_TO_DOWNLOAD, this should be placed on your desktop. Make sure you choose a large file of size > 1GB. You can download a 30 MB file from https://sample-videos.com/index.php#sample-mp4-video, and then concatenate multiple instances of that file into a single file as:

````sh
$ cat sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 > sampleVideo_Large.mp4
$ cat sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 > sampleVideo.mp4 
````

remove the requirement for a password when using sudo by:
````sh
$ sudo visudo
````
add to the end of the file the following, where YOUR_USERNAME is the name of the linux user:
YOUR_USERNAME ALL=(ALL) NOPASSWD: ALL

exit by ctrl+x and then select y and enter


next run the testing script
````sh
$ ./run_verus.sh
````

Once done running all scenarios, do the following:
````sh
$ cd Results
$ python plotverus.py CityDrive 20 300
$ python plotverus.py Corniche 20 300
$ python plotverus.py highwayGold 20 300
$ python plotverus.py rapidGold 20 300
````

You can view the results inside the figures folder

#### Generating Figure 8,9, and 10 (Copa vs. ALCC Copa)

Assuming that you have properly configured the bftpd application from the setup before. 
Now we need to compile it for ALCC Copa. Go to Applications/bftpd and do the following:

```sh
$ cd Applications
$ cd bftpd
$ cp Makefile_copa Makefile
$ make clean
$ make
```

open a new termain window.
Make sure you choose the underlying default TCP congestion control protocol first For-example: to choose CUBIC do the following:
````sh
$ sudo sysctl net.ipv4.tcp_congestion_control=cubic
````
You can confirm the chosen TCP CC protocol as follows:
````sh
$ sysctl net.ipv4.tcp_congestion_control
````

Run the bftpd ALCC copa server as:
````sh
$ sudo ./bftpd -D -c bftpd.conf
````

Go to the other terminal and navigate to the Artifacts folder:
````sh
$ cd ALCC/Artifacts
$ ./run_copa.sh
````

Once done running all scenarios, do the following:
````sh
$ cd Results
$ python plotcopa.py CampusWalk 20 300
$ python plotcopa.py Highway 20 300
$ python plotcopa.py cellularGold 20 300
$ python plotcopa.py Corniche 20 300
````

You can view the results inside the figures folder