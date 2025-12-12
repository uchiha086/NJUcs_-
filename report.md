# 实验三：软件实现与构建

##### 231220080 丁一鸣

### 1.根据UML图实现代码

代码实现的类主要是根据UML图中的类图中的具体划分实现，即分为Category、Transaction、StatisticService、UIService和SearchService进行，每个类中进行的操作也参考了类图中的操作。需要注意的是只实现了主体部分的内容，对于User的登录部分没有进行实现。

 <img src="C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112012306805.png" alt="image-20251112012306805" style="zoom: 67%;" />

 <img src="C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112012320624.png" alt="image-20251112012320624" style="zoom:67%;" />

 <img src="C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112012345772.png" alt="image-20251112012345772" style="zoom:67%;" />

  <img src="C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112012407307.png" alt="image-20251112012407307" style="zoom:67%;" />

  <img src="C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112012441722.png" alt="image-20251112012441722" style="zoom:67%;" />

代码main函数中实现了在终端中的简单的依托于字符的图形化，这个程序进行的流程是参考UML图中的活动图进行，分为记一笔、统计和查找三个主要功能，这里和UML图不一样的是增加了一个管理分类的模块专门进行分类的增删改查。

代码实现还有一点和类图不一样，在进行统计的时候不将统计数据单独存储，而是在调取时实时计算，这样简化了代码的实现，在数据量较小时基本不影响效率。

程序运行依赖和修改的数据直接存在了一个和程序同名的db文件中，但是不是通过现成的数据库管理实现的，而是直接通过读取文件，将其中的内容保存在vector中实现了类似的功能。

关于大模型的使用，在实验初期使用了Claude4.5模型，根据类图生成了几个类的大致框架，但是实现的代码过于复杂，每个功能之间的实现都是相对独立的，很难进行整合，然后我通过具体的需求简化了一些类的功能和内容，比如删去了存储统计数据的功能和用户登录的功能，简化了增加和删除的逻辑等。

还有一点用到大模型的是数据的存取部分，本身只是通过vector实现，每次进入程序的时候数据都是全新的，后来直接让大模型增加了一个数据的存取功能，可以将数据存在程序同名的db文件中进行数据库管理。在使用过程中使用了GPT5和Claude4.5，综合下来Claude4.5的代码能力更强一点。

### 2.代码的编译与运行结果

使用`g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o bookkeeping code.cpp`进行编译，编译后生成bookkeeping.exe即为可执行文件，程序运行中产生的数据存储在bookkeeping.db中。这里编译是在windows系统上进行的，在windows的终端中没有乱码出现。具体的运行结构如下：

首页

 ![image-20251112015336841](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112015336841.png)

输入数据进行功能的选择，即活动图中活动的选择，这里分别演示一下：

   ![image-20251112015409668](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112015409668.png)

 ![image-20251112015442270](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112015442270.png)

   ![image-20251112015524654](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112015524654.png)

 ![image-20251112015806059](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112015806059.png)

以上就是主要功能的演示，生成的数据存储在bookkeeping.db中：

 ![image-20251112015815307](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112015815307.png)

### 3.Git远程代码管理展示

 ![image-20251112021733152](C:\Users\user\AppData\Roaming\Typora\typora-user-images\image-20251112021733152.png)

将代码托管到自己的Github仓库中了