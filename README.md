在本任务中，你将在任务二静态路由的基础上，为路由器添加动态路由功能。同任务二一样，任务三的网络拓扑与代码框架均已给出。


#### 4.3.1 任务要求 <a name="subsubsec4.3.1"></a>

请基于给定的拓扑及代码框架

1. 将动态路由代码补充完整，实现RIP路由协议。
2. 阅读代码，画出动态路由的代码流程图。

#### 4.3.2 任务拓扑<a name="subsubsec4.3.2"></a>

任务三网络拓扑如下，其包含一台客户端（client）主机，三台需要你实现RIP协议的路由器（Router1, Router2, 和Router3），两台服务器（分别是server1和server2）主机，三台主机都连接于路由器，具体的IP和端口设置如下图所示。

<img src="计网实验四.assets/topo3-16983026016822.png" alt="topo3" style="zoom:50%;" />

#### 4.3.3 任务流程<a name="subsubsec4.3.3"></a>

**1、**进入虚拟机并定位到具体的实验代码目录：

```
/home/ubuntu/workspace/lab3
```

**2、**进入对应的实验目录，并运行以下命令以配置运行环境、启动Mininet模拟器和POX控制器（POX控制器和Mininet需要分别运行在两个终端中）：

```bash
cd ~/workspace/lab3/
./config.sh
./run_pox.sh
./run_mininet.sh
```

执行`./config.sh` ，你将能看到以下输出：

```
ubuntu@lab4:~/workspace/lab3$ ./config.sh 
running develop
running egg_info
writing requirements to pwospf.egg-info/requires.txt
writing pwospf.egg-info/PKG-INFO
writing top-level names to pwospf.egg-info/top_level.txt
writing dependency_links to pwospf.egg-info/dependency_links.txt
reading manifest file 'pwospf.egg-info/SOURCES.txt'
writing manifest file 'pwospf.egg-info/SOURCES.txt'
running build_ext
Creating /usr/local/lib/python2.7/dist-packages/pwospf.egg-link (link to .)
pwospf 0.0.0 is already the active version in easy-install.pth

Installed /home/ubuntu/workspace/lab3/pox_module
# 省略后续输出
```

运行POX控制器时，你应该能看到如下输出：

```
ubuntu@lab4:~/workspace/lab3$ ./run_pox.sh 
POX 0.5.0 (eel) / Copyright 2011-2014 James McCauley, et al.
server1 192.168.2.200
server2 172.24.3.30
client 10.0.1.100
router1-eth1 10.0.1.1
router1-eth2 10.0.2.1
router1-eth3 10.0.3.1
router2-eth1 10.0.2.2
router2-eth2 192.168.2.2
router2-eth3 192.168.3.1
router3-eth1 10.0.3.2
router3-eth2 172.24.3.2
router3-eth3 192.168.3.2
INFO:.home.ubuntu.workspace.lab3.pox_module.pwospf.srhandler:created server
SRServerListener listening on 8888
INFO:core:POX 0.5.0 (eel) is up.
```

运行Mininet模拟器时，你应该能看到如下输出：

```
ubuntu@lab4:~/workspace/lab3$ ./run_mininet.sh 
[sudo] password for ubuntu: 
*** Shutting down stale SimpleHTTPServers  
*** Shutting down stale webservers  
server1 192.168.2.200
server2 172.24.3.30
client 10.0.1.100
router1-eth1 10.0.1.1
router1-eth2 10.0.2.1
router1-eth3 10.0.3.1
router2-eth1 10.0.2.2
router2-eth2 192.168.2.2
router2-eth3 192.168.3.1
router3-eth1 10.0.3.2
router3-eth2 172.24.3.2
router3-eth3 192.168.3.2
*** Successfully loaded ip settings for hosts
 {'server1': '192.168.2.200', 'server2': '172.24.3.30', 'router3-eth3': '192.168.3.2', 'router3-eth1': '10.0.3.2', 'router3-eth2': '172.24.3.2', 'client': '10.0.1.100', 'router2-eth3': '192.168.3.1', 'router2-eth2': '192.168.2.2', 'router2-eth1': '10.0.2.2', 'router1-eth2': '10.0.2.1', 'router1-eth3': '10.0.3.1', 'router1-eth1': '10.0.1.1'}
*** Creating network
*** Creating network
*** Adding controller
Unable to contact the remote controller at 127.0.0.1:6653
Connecting to remote controller at 127.0.0.1:6633
*** Adding hosts:
client server1 server2 
*** Adding switches:
router1 router2 router3 
*** Adding links:
(client, router1) (router2, router1) (router3, router1) (router3, router2) (server1, router2) (server2, router3) 
*** Configuring hosts
client server1 server2 
*** Starting controller
c0 
*** Starting 3 switches
router1 router2 router3 ...
*** setting default gateway of host server1
server1 192.168.2.2
*** setting default gateway of host server2
server2 172.24.3.2
*** setting default gateway of host client
client 10.0.1.1
*** Starting SimpleHTTPServer on host server1 
*** Starting SimpleHTTPServer on host server2 
*** Starting CLI:
```

**3、**现在已启动了模拟的网络拓扑，本任务提供了一个已实现RIP的路由器可执行文件solution供参考，请在三个终端中分别执行下述的三个的三个命令，以分别运行三个路由器。

```
./solution -t 300 -s 127.0.0.1 -p 8888 -v router1
```

```
./solution -t 300 -s 127.0.0.1 -p 8888 -v router2
```

```
./solution -t 300 -s 127.0.0.1 -p 8888 -v router3
```

==运行后，可能1个或者多个终端显示 ` *warning* Routing table empty `；或者可能出现终端意外退出。==可以通过 ctrl + d 关闭 mininet，ctrl + c 关闭 solution(sr)，保留 pox，然后再依次重新运行 mininet，以及 solution(sr)。

对任一路由器，你将看到下列内容，打印的内容中显示了路由表各端口和转发表的情况

```
ubuntu@lab4:~/workspace/lab3$ ./solution -t 300 -s 127.0.0.1 -p 8888 -v router1
Using VNS sr stub code revised 2009-10-14 (rev 0.20)
Client ubuntu connecting to Server 127.0.0.1:8888
Requesting topology 300
successfully authenticated as ubuntu
Router interfaces:
eth3	HWaddr86:5f:a9:3d:c9:85
	inet addr 10.0.3.1
	inet mask 255.255.255.252
eth2	HWaddra2:b2:40:bf:95:c6
	inet addr 10.0.2.1
	inet mask 255.255.255.252
eth1	HWaddrfa:4e:39:6b:cd:00
	inet addr 10.0.1.1
	inet mask 255.255.255.0
  <---------- Router Table ---------->
Destination	Gateway		Mask		Iface	Metric	Update_Time
10.0.3.0	0.0.0.0	255.255.255.252	eth3	0	14:44:23
10.0.2.0	0.0.0.0	255.255.255.252	eth2	0	14:44:23
10.0.1.0	0.0.0.0	255.255.255.0	eth1	0	14:44:23
 <-- Ready to process packets --> 
```

**4、**在提供的参考实现中(solution)，当有两个及以上路由器运行时，路由器之间将每隔五秒钟自动交换路由信息。每五秒钟，你将看到每个路由器中更新后的路由信息，也即路由器中路由表的内容（下为router1的路由表内容）。

```
  <---------- Router Table ---------->
Destination	Gateway		Mask		Iface	Metric	Update_Time
10.0.3.0	0.0.0.0	255.255.255.252	eth3	0	14:44:53
10.0.2.0	0.0.0.0	255.255.255.252	eth2	0	14:44:53
10.0.1.0	0.0.0.0	255.255.255.0	eth1	0	14:44:53
192.168.3.0	10.0.2.2	255.255.255.252	eth2	1	14:44:52
192.168.2.0	10.0.2.2	255.255.255.0	eth2	1	14:44:52
172.24.3.0	10.0.3.2	255.255.255.0	eth3	1	14:44:50
```

**5、**当路由表收敛后，你可以在Mininet中使用traceroute命令查看client到server1的路由信息，你将能看到如下输出：

```
mininet> client traceroute 192.168.2.200
traceroute to 192.168.2.200 (192.168.2.200), 30 hops max, 60 byte packets
 1  10.0.1.1 (10.0.1.1)  63.712 ms  63.589 ms  63.627 ms
 2  10.0.2.2 (10.0.2.2)  901.800 ms  831.979 ms  901.836 ms
 3  192.168.2.200 (192.168.2.200)  1019.533 ms  1019.655 ms  1019.596 ms
```

**6、**现在我们手动断开router1和router2之间的链路，再次查看路由表的收敛情况。在Mininet中使用下述命令。

```
mininet> link router1 router2 down
```

你将能观察到路由器的路由表在几分钟后收敛，并且router1的路由表没有了到router2的路由

```
  <---------- Router Table ---------->
Destination	Gateway		Mask		Iface	Metric	Update_Time
10.0.3.0	0.0.0.0	255.255.255.252	eth3	0	14:46:58
10.0.1.0	0.0.0.0	255.255.255.0	eth1	0	14:46:58
192.168.3.0	10.0.3.2	255.255.255.252	eth3	1	14:46:55
192.168.2.0	10.0.3.2	255.255.255.0	eth3	2	14:46:55
172.24.3.0	10.0.3.2	255.255.255.0	eth3	1	14:46:55
```

**7、**再次在Mininet中使用traceroute，此时你将发现client到server1的路由信息已经发生了更新。

```
mininet> client traceroute server1
traceroute to 192.168.2.200 (192.168.2.200), 30 hops max, 60 byte packets
 1  10.0.1.1 (10.0.1.1)  74.457 ms  74.413 ms  74.404 ms
 2  10.0.3.2 (10.0.3.2)  147.585 ms  150.291 ms  213.970 ms
 3  192.168.3.1 (192.168.3.1)  271.000 ms  326.139 ms  394.015 ms
 4  192.168.2.200 (192.168.2.200)  394.040 ms  394.027 ms  394.019 ms
```

**8、**若恢复在第6步中断开的链路后，再次使用traceroute查看client到server1的路由信息，可发现其又恢复到步骤5中的状态。

```
mininet> link router1 router2 up
mininet> client traceroute server1
traceroute to 192.168.2.200 (192.168.2.200), 30 hops max, 60 byte packets
 1  10.0.1.1 (10.0.1.1)  91.427 ms  91.435 ms  36.478 ms
 2  10.0.2.2 (10.0.2.2)  302.904 ms  351.201 ms  351.218 ms
 3  192.168.2.200 (192.168.2.200)  399.408 ms  399.498 ms  442.537 ms
```

#### 4.3.4 代码概览<a name="subsubsec4.3.4"></a>

本任务涉及的代码在目录 `/home/ubuntu/workspace/lab3/router`下，在开始本任务前，请在确保任务二代码可正确运行后，将在任务二中补全后的sr_router.c文件拷贝到任务三/lab3/router目录下。 你可以执行以下命令以编译你的代码：

```bash
cd ~/workspace/lab3/router
sudo make 
```

你需要在三个终端中分别执行下述的三个命令，以分别运行三个路由器。

```
./sr -t 300 -s 127.0.0.1 -p 8888 -v router1
```

```
./sr -t 300 -s 127.0.0.1 -p 8888 -v router2
```

```
./sr -t 300 -s 127.0.0.1 -p 8888 -v router3
```

本任务中需要补全的代码均在sr_rt.c文件中，空缺的代码块将以如下形式呈现：

```cpp
/* Lab4-Task3 TODO */
需要你实现的代码  
/* End TODO */
```

为了便于你理解本任务代码，现对sr_rt.c中主要的方法做简要介绍：

```cpp
int sr_load_rt(struct sr_instance* sr,const char* filename)
```

路由器通过这个函数从现有的文件中读取路由表，sr参数为指向路由器实例的指针，filename参数为指向带有路由表的文件路径。

```cpp
void sr_add_rt_entry(struct sr_instance* sr, struct in_addr dest,struct in_addr gw, struct in_addr mask, uint32_t metric, char* if_name)
```

路由器使用这个函数添加路由表的路由条目，sr参数为指向路由器实例的指针，dest参数为路由条目的目的IP地址，gw参数为路由条目的网关，mask参数为路由条目的掩码，metric参数为到该路由地址所需要的跳数，if_name参数表示转发到该路由所经过的端口名。

```cpp
void sr_print_routing_table(struct sr_instance* sr)
```

路由器通过这个函数打印路由表，可以通过该函数观察路由表的收敛情况。

```cpp
void sr_print_routing_entry(struct sr_rt* entry)
```

上一个函数通过调用这个函数来打印路由表中的具体条目。entry参数为执行某个条目的指针。

```cpp
void *sr_rip_timeout(void *sr_ptr)
```

路由器通过调用这个函数，定时查看路由表中是否存在过期条目，是否存在有端口离线或端口上线的情况发生，如果存在这些情况，则给邻近的交换机发布RIP路由更新消息，更新临近交换机的路由表。

```cpp
void send_rip_request(struct sr_instance *sr)
```

路由器通过这个函数发送RIP的请求，调用这个函数时，会一次性向所有路由器端口发送RIP请求包。

```cpp
void send_rip_response(struct sr_instance *sr)
```

路由器通过这个函数发送RIP回复，调用这个函数时，会一次性向所有的路由发送自身的路由表。

```cpp
void update_route_table(struct sr_instance *sr, uint8_t *packet, unsigned int len, char *interface)
```

路由器通过该函数接受其他路由的回复，收到其他路由回复后，与自身的路由表做比较，如果路由表发生改变，则路由器自身调用路由回复函数，向相邻的路由器发布新的路由表。

<!-- 为方便你理解实验三代码的执行流程，我们为你提供了代码执行的流程图。-->

<!-- <img src="计网实验四.assets/流程图RIP.png" alt="流程图RIP" style="zoom: 50%;" /> -->

#### 4.3.5 涉及的网络协议<a name="subsubsec4.3.5"></a>

本任务使用RIPv2，请阅读[RFC2453 RIP Version 2](https://datatracker.ietf.org/doc/html/rfc2453)了解更多RIPv2细节。请注意，RIPv2使用多播将路由表发送给邻近路由器，为了简化代码实现，本任务中使用广播发送路由表。


<!-- 

```
struct sr_rip_pkt {
    uint8_t command;
    uint8_t version;
    uint16_t unused;
    struct entry{
        uint16_t afi; /* Address Family Identifier */
        uint16_t tag; /*Route Tag */
        uint32_t address; /* IP Address */
        uint32_t mask; /* Subnet Mask */
        uint32_t next_hop; /* Next Hop */
        uint32_t metric; /* Metric */
        } entries[MAX_NUM_ENTRIES]; /* #define MAX_NUM_ENTRIES 25 */
    } __attribute__ ((packed)) ;
    typedef struct sr_rip_pkt sr_rip_pkt_t; 
```

-->

#### 4.3.6 测试样例<a name="subsubsec4.3.6"></a>

如果你的代码工作正常，则以下所有操作都应该有效：

- 在给定的网络拓扑中能正确计算路由表。
- 当网络拓扑发生变化时，网络中路由能正确收敛。

我们为你准备了15个测试样例，在实现完你的方案后，你可以根据这些测试样例评测代码的正确性。当你提交你的实验代码后，我们也会根据这些测试样例来评测你的代码。

你可以在Mininet中执行以下命令以验证测试用例：

```
mininet> source testcases.sh
```

如果你通过了所有测试用例，你将看到如下输出：

```
mininet> source testcases.sh
Testcase1:  Pass
nohup: appending output to 'nohup.out'
Testcase2:  Pass
nohup: appending output to 'nohup.out'
Testcase3:  Pass
Info: router1-eth2 down
Info: Sleep 10 seconds, wait for routing table converging
Testcase4:  Pass
Testcase5:  Pass
Testcase6:  Pass
Info: router2-eth3 down
Info: Sleep 10 seconds, wait for routing table converging
Testcase7:  Pass
Testcase8:  Pass
Testcase9:  Pass
Info: router2-eth3 up
Info: router1-eth2 up
Info: Sleep 10 seconds, wait for routing table converging
Testcase10:  Pass
Testcase11:  Pass
Testcase12:  Pass
Info: router1-eth1 down
Info: Sleep 10 seconds, wait for routing table converging
Testcase13:  Pass
Testcase14:  Pass
Info: router1-eth1 up
Info: Sleep 10 seconds, wait for routing table converging
Testcase15:  Pass
```

**测试用例的具体设置如下：**

测试样例1：
Client Ping往拓扑中所有的IP地址，并traceroute到 server1。

测试样例2：
Server1 Ping往拓扑中所有的IP地址，并traceroute到server2。

测试样例3：
Server2 Ping往拓扑中所有的IP地址，并traceroute到client。

**进行到这一步之后，断开router1和router2之间的链路，然后测试以下测试样例**

测试样例4：
Client Ping往拓扑中所有的IP地址，并traceroute到server1。

测试样例5：
Server1 Ping往拓扑中所有的IP地址，并traceroute到server2。

测试样例6：
Server2 Ping往拓扑中所有的IP地址并traceroute到client。

**进行到这一步后，断开router2和router3之间的链路，测试以下测试样例**

测试样例7：
Client Ping往拓扑中所有的IP地址，并traceroute到server2。

测试样例8：
Server1 Ping往拓扑中所有的IP地址。

测试样例9：
Server2 Ping往拓扑中所有的IP地址，并traceroute 到client。

**然后恢复我们刚才断开的2个链路，测试以下测试样例**

测试样例10：
与测试用例1相同。

测试样例11：
与测试用例2相同。

测试样例12：
与测试用例3相同。

**然后阻塞 router1 和 client 之间的链路，测试以下测试样例**

测试样例13：
Server1 Ping往拓扑中所有的IP地址，并traceroute到 server2。

测试样例14：
Server2 Ping往拓扑中所有的IP地址，并traceroute到router1-eth3。

**最后，接收router1和client之间的链路，测试以下测试样例**

测试样例15：
与测试用例1相同。
