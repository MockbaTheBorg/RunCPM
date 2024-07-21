print("RunCPM System Information v1.0.lua")
print("")
local version = BdosCall(251, 0)
local v1 = version // 16
local v2 = version % 16
local host = BdosCall(250, 0)
local shost = "Unknown"
if(host == 0) then shost = "Windows" end
if(host == 1) then shost = "Arduino" end
if(host == 2) then shost = "Posix" end
if(host == 3) then shost = "DOS" end
if(host == 4) then shost = "Teensy" end
if(host == 5) then shost = "ESP32" end
print("RunCPM Version "..v1.."."..v2.." running on "..shost)
local ccp = BdosCall(252, 0)
local sccp = "Unknown"
if(ccp == 0) then sccp = "DRI" end
if(ccp == 1) then sccp = "CCPZ" end
if(ccp == 2) then sccp = "ZCPR2" end
if(ccp == 3) then sccp = "ZCPR3" end
if(ccp == 4) then sccp = "Z80CCP" end
if(ccp > 15) then
	v1 = ccp // 16
	v2 = ccp % 16
	sccp = "Internal Version "..v1.."."..v2
end
print("CCP is "..sccp)
local ccpaddr = BdosCall(253, 0)
print("CCP loads at address "..string.upper(string.format("%04x",ccpaddr)).."h")
local bdos = RamRead(7)
bdos = bdos * 256
print("BDOS is at address "..string.upper(string.format("%04x",bdos)).."h")
local bios = RamRead(2)
bios = bios * 256
print("BIOS is at address "..string.upper(string.format("%04x",bios)).."h")
print((bdos-256).." bytes available for applications")
--