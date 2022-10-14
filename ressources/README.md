# Setup dialog channel numbers

This document lists the used (system) channel numbers and their meaning.

## Common buttons on all pages

| Number | Description |
| ------ | ----------- |
| 412 | OK: Save changes and close setup |
| 413 | Cancel: Forget all changes and close setup |

## Dialog page Controller (5002)

| Number | Description |
| ------ | ----------- |
| 122 | FQDN of NetLinx or the IP-address |
| 144 | Network port of NetLinx. Default 1319 |
| 41 | Channel number of the panel |
| 199 | Panel type / Name of the panel (NDP name) |
| 2020* | FTP user name |
| 2021* | FTP password |
| 25 | TP4 file name |
| 2023* | Listbox for file names from FTP |
| 2030* | FTP button download |
| 2031* | FTP passive mode |

\* _Numbers are not official setup functions and may violate with official numbers!_

## Dialog page Logging (5001)

All the following numbers are no official channels numbers and my violate with official functions!

| Number | Description |
| ------ | ----------- |
| 2000 | Checkbox Info |
| 2001 | Checkbox Warning |
| 2002 | Checkbox Error |
| 2003 | Checkbox Trace |
| 2004 | Checkbox Debug |
| 2005 | Checkbox Protocol |
| 2006 | Checkbox all |
| 2007 | Checkbox Profiling |
| 2008 | Checkbox Long format |
| 2009 | Input line log file |
| 2010 | Button reset |
| 2011 | Button file open |

## Dialog page SIP (5003)

| Number | Description |
| ------ | ----------- |
| 418 | SIP proxy name / IP |
| 419 | SIP network port |
| 420 | SIP STUN server |
| 421 | SIP domain server (usually the same as proxy) |
| 422 | SIP User name |
| 423 | SIP password |
| 2060* | Checkbox IPv4 |
| 2061* | Checkbox IPv6 |
| 416 | Checkbox enabled |
| 2062* | Use internal phone |

\* _Numbers are not official setup functions and may violate with official numbers!_

## Dialog page Sound (5005)

| Number | Description |
| ------ | ----------- |
| 1143 | File name of system sound |
| 2024 | List box for system sounds (5 rows) |
| 404 | File name of single beep (button hit) |
| 2025 | List box of single beep (5 rows) |
| 405 | File name of double beep (button miss) |
| 2026 | List box of double beep (5 rows) |
| 17 | Enable system sound |
| 9 | Bargraph master volume (button 333) |
| 6 | Bargraph gain (button 327) |
| 2050* | Play system sound |
| 2051* | Play single beep |
| 2052* | Play double beep |
| 159 | Play test sound |

\* _Numbers are not official setup functions and may violate with official numbers!_

## Dialog page View (5004)

All the following numbers are no official channels numbers and my violate with official functions!

| Number | Description |
| ------ | ----------- |
| 2070 | Scale to fit |
| 2071 | Show banner on startup |
| 2072 | Suppress toolbar |
| 2073 | Force toolbar visible |
| 2074 | Lock rotation |
