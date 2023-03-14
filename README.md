WakeByName
==

[![Build status](https://ci.appveyor.com/api/projects/status/github/CodeBeast357/WakeByName?svg=true)](https://ci.appveyor.com/project/CodeBeast357/wakebyname)
[![Coverage Status](https://coveralls.io/repos/github/CodeBeast357/WakeByName/badge.svg?branch=master)](https://coveralls.io/github/CodeBeast357/WakeByName?branch=master)

Overview
--
This command line tool elevates the operation [Wake-on-LAN](<https://en.wikipedia.org/wiki/Wake-on-lan>) using the usual network resolution protocols. It first resolves a list of [domain name](<https://en.wikipedia.org/wiki/Domain_name>)s from [DNS](<https://en.wikipedia.org/wiki/Domain_Name_System> "Domain Name System") lookups. Each resulting [IP address](<https://en.wikipedia.org/wiki/IP_address> "Internet Protocol address") is translated to a [MAC address](<https://en.wikipedia.org/wiki/MAC_address> "Media Access Control address") by the [ARP](<https://en.wikipedia.org/wiki/Address_Resolution_Protocol> "Address Resolution Protocol"). Once it has the MAC address, it builds up the magic packet and sends it on an [UDP](<https://en.wikipedia.org/wiki/User_Datagram_Protocol> "User Datagram Protocol") broadcast or multicast socket.

Background
--
The Wake-On-LAN mechanism uses the provided MAC address to compile and send a magic packet to trigger a [NIC](<https://en.wikipedia.org/wiki/Network_interface_controller> "Network Interface Controller") to power up.

The MAC address is usually unique and assigned by the manufacturer of the NIC. This situation is not always the case, as one could have a custom address.

By elevating the Wake-on-LAN mechanism with higher [OSI](<https://en.wikipedia.org/wiki/OSI_model> "Open Systems Interconnection") level protocols, this tool abstracts away the complexity of maintaining the usability of the base mechanism.

Features
--
* [`getopt`](<https://en.wikipedia.org/wiki/Getopt>)-style arguments
* List of domain names to trigger
* `n`: List of DNS servers to request (default: host NIC servers)
* `p`: Port number (default: 7)
* `s`: Flag for subnet range limitation
