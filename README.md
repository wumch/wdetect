# wdetect
php extension for detecting numbers and digital info from weixin/wechat mp-platform's data-screenshot 

wdetect 是一个php扩展，功能主要是：

从微信数据截图上找到那个 漏斗形的图表 并读取里面包含的转化率数值信息；
在图片的指定区域内扫描、定位数字；
从图片上指定的矩形框里读取数值；
判断图片上指定的矩形框里包含的文本的宽度(像素数) 与 给定的文本 是否相近(用于粗略判断 图上文本内容跟指定文本内容是否相同，下面安装的版本里被阉了)
wdetect 依赖opencv2、boost>=1.41(filesystem/preprocessor/lexical_cast等)、staging(c/c++脚手架，私人物品)，上述第四个功能依赖 ImageMagick。

CentO上安装：
先安装依赖：

安装opencv2，要下载89M的源码，这里有：192.168.100.141:/home/wumengchun/opencv-2.4.9.zip (http://jaist.dl.sourceforge.net/project/opencvlibrary/opencv-unix/2.4.9/opencv-2.4.9.zip)
安装boost：sudo yum install boost141-devel.x86_64 boost141-filesystem.x86_64 -y
（wdetect、staging代码svn地址：http://dev.xundameng.com/weiboyi-api/Plugins/ImageRecognition/trunk）

然后进wdetect源码的目录，修改config.m4，设置 CXX=g++44  后再

/usr/local/php/bin/phpize

./configure --enable-wdetect --with-staging=/opt/release/staging --with-php-config=/usr/local/php/bin/php-config

make && sudo make install

修改 /usr/local/php/etc/php.ini 设置 enable_dl=On

就可以用了(建议看看 /usr/local/php/bin/php -r 'error_reporting(-1); dl("wdetect.so"); var_dump(class_exists("WDetecter"));' 对不对)。



测试：进wdetect源码目录，执行

/usr/local/php/bin/php test.php   # 用 ./data/16.jpg 这个图片做测试

/usr/local/php/bin/php test.php ./data  # 用 ./data 下的所有图片文件做测试，输出每次测试的详细结果和统计结果

192.168.100.26:/home/wumengchun/wdetect 已经装好了，ssh账号和密码都是 evaluation。

 

安装 cmake28、gcc44 前可能要加软件源：

可以从192.168.100.141复制这些文件 /etc/yum.repos.d/epel.repo    /etc/yum.repos.d/epel-testing.repo    /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL

 

装完boost141可能要：

for lib in ls /usr/lib64/libboost_*so.5; do test ! -e ${lib/so.5/so} && sudo ln -s $lib ${lib/so.5/so} ; done

 

centos上安装opencv2：

先把cmake升级到2.8.1x，然后 cmake28（不能cmake）
记得把CC/CXX设置为gcc4.4版：env CC=/usr/bin/gcc44 CXX=/usr/bin/g++44  cmake28 -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr -D BUILD_NEW_PYTHON_SUPPORT=NO .. 
make前先解决依赖：
sudo yum install gtk+-devel.x86_64 gimp-devel.x86_64 gimp-devel-tools.x86_64 gimp-help-browser.x86_64 zlib-devel.x86_64 libtiff-devel.x86_64 libjpeg-devel.x86_64 libpng-devel.x86_64 gstreamer-devel.x86_64 libavc1394-devel.x86_64 libraw1394-devel.x86_64 libdc1394-devel.x86_64 jasper-devel.x86_64 jasper-utils.x86_84 swig python libtool nasm.x86_84
