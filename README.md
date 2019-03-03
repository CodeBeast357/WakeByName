WakeByName
==

[![Build status](https://ci.appveyor.com/api/projects/status/github/CodeBeast357/WakeByName?svg=true)](https://ci.appveyor.com/project/CodeBeast357/wakebyname)
[![Coverage Status](https://coveralls.io/repos/github/CodeBeast357/WakeByName/badge.svg?branch=master)](https://coveralls.io/github/CodeBeast357/WakeByName?branch=master)

Overview
--
This command line tool elevates the [Wake-on-LAN](<https://en.wikipedia.org/wiki/Wake-on-lan>) mechanism by using the usual network resolution protocols. It firstly resolves a list of [domain name](<https://en.wikipedia.org/wiki/Domain_name>)s by the use of [DNS](<https://en.wikipedia.org/wiki/Domain_Name_System> "Domain Name System") lookups. The resulting [IP address](<https://en.wikipedia.org/wiki/IP_address> "Internet Protocol address")es are then each translated to a [MAC address](<https://en.wikipedia.org/wiki/MAC_address> "Media Access Control address") by the [ARP](<https://en.wikipedia.org/wiki/Address_Resolution_Protocol> "Address Resolution Protocol"). Once it has the MAC address, it builds up the magic packet and sends it on an [UDP](<https://en.wikipedia.org/wiki/User_Datagram_Protocol> "User Datagram Protocol") broadcast or multicast socket.

Background
--
The Wake-On-LAN mechanism uses the [MAC address](<https://en.wikipedia.org/wiki/MAC_address> "Media Access Control address") you provide to compile and send a magic packet in order to trigger a [NIC](<https://en.wikipedia.org/wiki/Network_interface_controller> "Network Interface Controller") to power up.

The MAC address is usually unique and assigned by the manufacturer of the NIC. This is not always the case. To make matters worse, it can be changed to another custom address.

By elevating the Wake-on-LAN mechanism with higher [OSI](<https://en.wikipedia.org/wiki/OSI_model> "Open Systems Interconnection") level protocols, it abstracts away the complexity of maintaining the usability of the base mechanism.

Features
--
* [`getopt`](<https://en.wikipedia.org/wiki/Getopt>)-style arguments
* List of domain names to trigger
* `n`: List of DNS servers to request (default: host NIC servers)
* `p`: Port number (default: 7)
