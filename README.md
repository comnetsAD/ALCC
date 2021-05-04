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
#### Kernel Version Required
The ALCC kernel module requires kernel version 4.19.0. 
Make sure you upgrade/downgrade to kernel version 4.19.0 before proceeding. 


##### Step 1:
###### Set default TCP congestion control protocol
Before starting any experiments, make sure the default system TCP congestion control protocol is set as desired. For example; for cubic, do the following

```sh
$ sysctl -w net.ipv4.tcp_congestion_control=cubic
```
###### Install ALCC Kernel Module
```sh
$ cd libalcc/netfilter
$ make clean
$ make
$ sudo insmod alcc_kernel.ko 
```
###### Start BFTPD server
Before running the bftpd server, make sure you make changes to the following files.

go to
````sh
$ Applications/bftpd/bftpd.conf
````
open the bftpd.conf file, scroll down to FILE_AUTH and provide the proper full path. As well as the AUTO_CHDIR, which is where the FTP server is starting from.

Next, go to 
````sh
$ Applications/bftpd/passwd
````
open the passwd file and provide the proper path.
Note that, for the wget command to work, make sure the file you plan to download is placed in the same location defined in AUTO_CHDIR.

For ALCC Verus follow the following steps:
```sh
$ cd libalcc/verus/lib/alglib/src/
$ gcc -c *.cpp
```
```sh
$ cd Applications
$ cd bftpd
$ cp Makefile_verus Makefile
$ make clean
$ make
```
For ALCC Copa follow the following path

```sh
$ cd Applications
$ cd bftpd
$ cp Makefile_copa Makefile
$ make clean
$ make
```
Open a new terminal and start the bftpd server

```sh
$ Applications/bftpd/
$ sudo ./bftpd -D -c bftpd.conf
```

###### Start the runs for Native/ALCC protocols

Open a new terminal window.
Make sure you choose the underlying default TCP congestion control protocol first For-example: to choose CUBIC do the following:
````sh
$ sudo sysctl net.ipv4.tcp_congestion_control=cubic
````
You can confirm the chosen TCP CC protocol as follows:
````sh
$ sysctl net.ipv4.tcp_congestion_control
````
The Channel Traces used in our work are all placed in a folder called channelTraces inside the Eval subdirectory.
The Eval subdirectory also includes 2 important scripts that  we used to automate the runs over all channel traces. 
Follow the following Instructions

In the run.py file, you need to set the correct linux user name, password, ip address of the machine, as well as the name of the file to be downloaded. The file should be of large size > 20MB.

example:
USER_NAME= 'cuda'
PASSWORD = 'cuda'
IP_ADDRESS = '10.224.41.106'
FILE_TO_DOWNLOAD = 'jellyLarge.m4v'


In the run.sh file, set the parameters as desired. (The default settings are used for the results shown in our paper). 
Note that, for ALCC Verus and ALCC copa, the bftpd Makefiles should be chosen accordingly before proceeding. 
Also in the run.py file in Eval, line 78 and 106, the ip address in 'wget' command need to be updated to the ip address of your local machine.

```sh
$ cd Eval
$ run.sh
````
The results are collected in the Results folder. 

###### For running Native protocols Verus and Copa

For Verus and Copa, clone the follwing git repositories in the Applications subdirectory

````sh
$ cd Applications/
$ git clone https://github.com/yzaki/verus.git
$ git clone https://github.com/venkatarun95/genericCC.git

````
Follow the build instructions of both Verus and Copa provided in the git repositories. Forexample for verus do the following;
````sh
$ cd verus
$ sudo apt-get install build-essential autoconf libasio-dev libalglib-dev libboost-system-dev
$ ./bootstrap.sh
$ ./configure
$ make
````

For plotting the Results, we have provided some helping Scripts namely plotverus.py and plotcopa.py in the Results folder.