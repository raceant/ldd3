linux_ribeto.patch 为内核补丁，给linux-2.6.30.4打上补丁后就可在板子上运行，
	使用方法：将linux_ribeto.patch放在内核源码的同级目录下，然后进入目录，执行命令
	#patch -p1 < ../linux_ribeto.patch


lordweb_ribeto.patch为公司驱动补丁，给上面打好linux_ribeto.patch补丁的内核打上该补丁后即可		编译公司驱动。使用方法：将lordweb_ribeto.patch放在内核源码的同级目录下，然后在此目		录下执行  #patch -p0 < lordweb_ribeto.patch


ribeto_config 为内核配置文件，将其放在内核根目录下，并命名为“.config”即可


root_ribeto.tar.bz2为公司文件系统


zImage.bin为编译好的内核映像，可直接使用


root_ribeto为制作好的文件系统映像，可直接使用