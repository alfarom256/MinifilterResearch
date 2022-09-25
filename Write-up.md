## 0.0 - Intro
### 0.1 Abstract
This research project served to help me learn more about file system minifilter drivers and how a malicious actor may leverage a vulnerable driver to patch callbacks for minifilters. Along the way, I'd discovered previous research by Aviad Shamriz which helped me immensely in my endeavor.

https://aviadshamriz.medium.com/part-1-fs-minifilter-hooking-7e743b042a9d

As this article goes very in-depth into the mechanics of file system minifilter hooking with another loaded driver, I will focus on my research methods which led me to develop a PoC leveraging Dell's vulnerable "dbutil" driver to perform the same actions from user-mode, and some things I learned along the way.

Alongside this I will present (mildly redacted) technical details of exploits I've previously found in production anti-malware products.

### 0.2 Acknowledgements
Thank you to Avid and MZakocs for your work which helped make this possible.
https://aviadshamriz.medium.com/part-1-fs-minifilter-hooking-7e743b042a9d
https://github.com/mzakocs/CVE-2021-21551-POC

Shoutout to the vxug community and my friends for inspiration and guidance:
* ch3rn0byl
* s4r1n
* cb
* Jonas

### 0.3 Setup
My testing and development setup was original Windows 10 1809, though after a lovely VMWare crash which led me to lose my VM, I have since reperformed my research on the Microsoft-provided Windows 11 Enterprise Hyper-V evaluation image.

## 1.0 - What is a file system mini filter
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/about-file-system-filter-drivers

https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts

https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/how-file-system-filter-drivers-are-similar-to-device-drivers

https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/how-file-system-filter-drivers-are-different-from-device-drivers

https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/storage-device-stacks--storage-volumes--and-file-system-stacks

File system minifilters are drivers which are used to inspect, log, modify, or prevent file system I/O operations. The filter manager driver (FltMgr.sys) effectively "sits in-between" the I/O Manager and the File System Driver, and is responsible for registration of file system minifilter drivers, and the invocation of their pre and post-operation callbacks. 

>A minifilter driver attaches to the file system stack indirectly, by registering with _FltMgr_ for the I/O operations that the minifilter driver chooses to filter.
*https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts*

FltMgr also maintains a list of volumes attached to the system, and is responsible for storing and invoking callbacks on a per-volume basis.

### 1.1 - Core concepts and APIs

#### Altitude
As previously mentioned, minifilters "sit in-between" the I/O manager and the filesystem driver. One of the fundamental questions and concepts which arose from this "proxying" behavior is: 
How do I know where in the "stack" my driver sits? 
What path does an IRP take from the I/O manager to the filesystem driver?

The minifilter's Altitude describes it's load order. For example, a minifilter with an altitude of "1000" will be loaded into the I/O stack before a minifilter with an altitude of "30100."

```
 ┌──────────────────────┐
 │                      │
 │     I/O Manager      │            ┌───────────────────┐
 │                      │            │  Minifilter0:     │
 └───────────┬──────────┘     ┌──────►  Altitude 42000   │
             │                │      │                   │
             │                │      └───────────────────┘
             │                │                
 ┌───────────▼──────────┐     │      ┌───────────────────┐
 │                      ◄─────┘      │  Minifilter1:     │
 │        FLTMGR        ◄────────────►  Altitude 30100   │
 │                      ◄─────┐      │                   │
 └───────────┬──────────┘     │      └───────────────────┘
             │                │                
             │                │      ┌───────────────────┐
             │                │      │  Minifilter2:     │
             │                └──────►  Altitude 30000   │
 ┌───────────▼──────────┐            │                   │
 │                      │            └───────────────────┘
 │         NTFS         │
 │                      │
 └───────────┬──────────┘
             │
             │
             ▼                          
			...
```
Simplified version of: https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts

#### Frame
Frames describe a range of Altitudes for mini-filters.


#### FltRegisterFilter

The first step to registering a file system minifilter is to, well, invoke the `FltRegisterFilter` method.

```
NTSTATUS FLTAPI FltRegisterFilter( 
	[in] PDRIVER_OBJECT Driver, 
	[in] const FLT_REGISTRATION *Registration, 
	[out] PFLT_FILTER *RetFilter 
);
```
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nf-fltkernel-fltregisterfilter






### 1.2 - Writing a minifilter
To aid in research and testing, I created a very simple mini filter driver to get the PID of any given process which inevitably calls `NtCreateFile` on the file `C:\Users\<USER>\test.txt`, and log it via `DbgPrint`.
