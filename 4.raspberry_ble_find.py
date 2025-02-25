# 이 코드를 실행하기 전에 다음을 설치하세요:
# sudo apt-get install python3-pip libglib2.0-dev
# sudo pip3 install bluepy

from bluepy.btle import Scanner, DefaultDelegate

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
    
    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev:
            print(f"새 장치 발견: {dev.addr} ({dev.addrType})")
        elif isNewData:
            print(f"장치 새 데이터: {dev.addr}")

scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(10.0)  # 10초 동안 스캔

print("\n발견된 장치:")
for dev in devices:
    print(f"장치 {dev.addr} ({dev.addrType}), RSSI={dev.rssi} dB")
    for (adtype, desc, value) in dev.getScanData():
        print(f"  {desc}: {value}")