# ALCC: Migrating Congestion Control to the Application Layer

Application Layer Congestion Control (ALCC) framework, that allows any new CC protocol to run at the application layer on top of the underlying TCP stack, and drives it to deliver approximately the same performance, as it would achieve natively.
This implementation includes Copa and Verus integration into ALCC which can be tested and compared over a set of different channel traces. The evaluation can be carried out in 2 ways. Following are the necessary instructions:
## Instructions for using the ALCC vitual-Box image
To test the functionality of ALCC, immediately, we provide a virtual box image that includes everything pre-installed and pre-configured. You can download the image from the link below: 
Link:[ALCC virtual box image]

After you set up the virtual box, navigate to the proper directory to run the protocols. Do the following;
####  For native verus and ALCC-verus
open a terminal window
````sh
$ cd ALCC/Artifacts
$ ./run_verus.py
````
####  For native Copa and ALCC-Copa
open a terminal window
````sh
$ cd ALCC/ Artifacts
$ ./run_copa.py
````
####  For ALCC-Copa atop TCP flavors
>**Note:** If you already have runned for native copa and ALCC-Copa atop cubic, i.e., the previous steps. Then remove cubic from algo list in file 'run_copa_tcp_variants.py' in line 15. Also, commnent out the section which requires a re-run for native Copa in line 12.

Open a terminal window
````sh
$ cd ALCC/Artifacts
$ python3 run_copa_tcp_variants.py
````
#### For ALCC-Verus atop TCP flavors
>**Note:** If you already have runned for native verus and ALCC-verus atop cubic, i.e., the previous steps. Then remove cubic from tcp_flavour list in line 15 of file 'run_verus_tcp_variants.py'. Also, commnent out the section which requires a re-run for native Verus (i.e, line 12 in 'run_verus_tcp_variants.py' )

Open a terminal window
````sh
$ cd ALCC/Artifacts
$ python3 run_verus_tcp_variants.py
````
####  For Verus and ALCC-Verus with losses
Open a terminal window
````sh
$ cd ALCC/Artifacts
$ python3 run_verus_losses.py
````
Once done running all scenarios, you can now plot each protocol

# Plotting Results
Open a terminal window
````sh
$ cd ALCC/Artifacts
$ cd Results
````
#### For Verus plots
###### Generating Figure 4,5, and 6 (Verus vs. ALCC-Verus)
Open a terminal window
````` sh
$ cd ALCC/Artifacts
$ cd Results
$ python plotverus.py CityDrive 20 300
$ python plotverus.py Corniche 20 300
$ python plotverus.py highwayGold 20 300
$ python plotverus.py rapidGold 20 300
`````
###### Generating Figure 13 (Verus vs. ALCC-Verus)
Open a terminal window
````` sh
$ cd ALCC/Artifacts
$ cd Results
$ python plotverus.py cellularGold 300
`````

>**Note**: Where 20 is the number of runs we have, and 300 is the total run time of each trace.

##### For Copa Plots
###### Generating Figure 8,9, and 10 (Copa vs. ALCC Copa)

Open a terminal window
````sh
$ cd ALCC/Artifacts
$ cd Results
$ python plotcopa.py CampusWalk 20 300
$ python plotcopa.py Highway 20 300
$ python plotcopa.py cellularGold 20 300
$ python plotcopa.py Corniche 20 300
````
>**Note**: Where 20 is the number of runs we have, and 300 is the total run time of each trace.

##### For ALCC-Verus and ALCC-Copa over different TCP variants
###### Generating Figure 11 and 12
Open a terminal window
````sh
$ cd ALCC/Artifacts
$ cd Results
$ python plot_verus_tcp_flavors.py CampusWalk 300 20
$ python plot_verus_tcp_flavors.py CampusWalk 300 20
$ python plot_copa_tcp_flavors.py cellularGold 20
$ python plot_copa_tcp_flavors.py highwayGold 20
````
# Instructions for using Linux machine
All our experiements are conducted on a linux machine with the following hardware and Software installed.
UBUNTU 2.04.2 
Memory: 62.8 GiB
Processor: Intel Xeon  CPU e5-1620 v4 @ 3.5 GHzx8
g++, gcc: 9.3.0
Libboost 1.71-dev
Python  3.8.2

## Installations
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
#### Install mahimahi 
(See also http://mahimahi.mit.edu/)
````sh
$ git clone https://github.com/ravinet/mahimahi
$ cd mahimahi
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo sysctl -w net.ipv4.ip_forward=1
````


#### Compile native verus and native copa

For Verus and Copa, we have cloned and modified slightly the native versions of these protocols to be able to compare. You just need to compile them. You can find the native repos inside the applications folder

#### For Copa

````sh
$ cd ALCC
$ cd Applications/genericCC
$ makepp
````

#### For Verus
````sh
$ cd ALCC
$ cd Applications/verus
$ ./bootstrap.sh
$ ./configure
$ make
````
#### Install ALCC Kernel Module
```sh
$ cd ALCC
$ cd libalcc/netfilter
$ make clean
$ make
$ sudo insmod alcc_kernel.ko 
```
# Running Experiments

Before running any of the experiments, make sure you make changes to the following files.
Go to to the bftpd folder and copy the path given by pwd
````sh
$ cd Applications/bftpd/
$ pwd
````
Copy the path shown after $ pwd
Next, open the bftpd.conf file, scroll down to FILE_AUTH and provide the full path you obtained earlier. + '/passwd'. ForExample 'FILE_AUTH="/home/YOUR_USERNAME/Desktop/ALCC/Applications/bftpd/passwd"' .You should also change the AUTO_CHDIR, and make it point to your Desktop folder, usually /home/username/Desktop.
You will also have to add to the passwd file inside the bftpd folder the user by adding to the end of it:
YOUR_USERNAME * sudo /home/YOUR_USERNAME/Desktop

>**Note that**, for the wget command to work, make sure the file you plan to download is placed in the same location defined in AUTO_CHDIR (as we mentioned earlier in your Desktop).

Navigate to the Artifact folder, and edit the run.py file.
Make sure you edit lines 12 - 15, and change them to the correct linux user, the user password, and the machine IP address (Do not set it to localhost or 127.0.0.1). This is an example of these parameters:

USER_NAME= 'alcc'
PASSWORD = 'password'
IP_ADDRESS = '10.224.41.106'
FILE_TO_DOWNLOAD = 'sampleVideo.mp4'

>**Notice that** how we also have a FILE_TO_DOWNLOAD. Follow the following section to get a file to download

#### Placing a large file on Desktop
 Make sure you choose a large file of size > 1GB placed on your desktop. You can download a 30 MB file from [here], and then concatenate multiple instances of that file into a single file as:

````sh
$ cat sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 sampleVideo.mp4 > sampleVideo_Large.mp4
$ cat sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 sampleVideo_Large.mp4 > sampleVideo.mp4 
````
#### Remove the requirement for a password when using sudo by:
````sh
$ sudo visudo
````
Add to the end of the file the following, where YOUR_USERNAME is the name of the linux user:
YOUR_USERNAME ALL=(ALL) NOPASSWD: ALL
Exit by ctrl+x and then select y and enter

####  For native verus and ALCC-verus
For ALCC Verus follow the following steps:
```sh
$ cd ALCC
$ cd libalcc/verus/lib/alglib/src/
$ gcc -c *.cpp
```
open a terminal window
````sh
$ cd ALCC/Artifacts
$ ./run_verus.py
````
####  For native Copa and ALCC-Copa
open a terminal window
````sh
$ cd ALCC/ Artifacts
$ ./run_copa.py
````
####  For ALCC-Copa atop TCP flavors
>**Note:** If you already have runned for native copa and ALCC-Copa atop cubic, i.e., the previous steps. Then remove cubic from algo list in file 'run_copa_tcp_variants.py' in line 15. Also, commnent out the section which requires a re-run for native Copa in line 12.

Open a terminal window
````sh
$ cd ALCC/Artifacts
$ python3 run_copa_tcp_variants.py
````
#### For ALCC-Verus atop TCP flavors
>**Note:** If you already have runned for native verus and ALCC-verus atop cubic, i.e., the previous steps. Then remove cubic from tcp_flavour list in line 15 of file 'run_verus_tcp_variants.py'. Also, commnent out the section which requires a re-run for native Verus (i.e, line 12 in 'run_verus_tcp_variants.py' )

Open a terminal window
````sh
$ cd ALCC/Artifacts
$ python3 run_verus_tcp_variants.py
````
####  For Verus and ALCC-Verus with losses
Open a terminal window
````sh
$ cd ALCC/Artifacts
$ python3 run_verus_losses.py
````
Once done running all scenarios, you can now plot each protocol by following the steps explained in "Plotting Results" Section above.


[here]: <https://sample-videos.com/index.php#sample-mp4-video>
[ALCC virtual box image]: <https://drive.google.com/drive/folders/1QTdeoMXDx_r6YfpL8yRENkla4HtRN33V?usp=sharing>
